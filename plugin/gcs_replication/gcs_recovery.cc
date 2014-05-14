/* Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "gcs_recovery.h"
#include "gcs_recovery_message.h"
#include <gcs_protocol_factory.h>
#include "gcs_member_info.h"
#include "handlers/certification_handler.h"
#include "gcs_message.h"
#include <applier_interfaces.h>
#include <mysqld_thd_manager.h>  // Global_THD_manager
#include <rpl_info_factory.h>
#include <rpl_slave.h>
#include <debug_sync.h>

/** Default user for donor connection*/
static char DEFAULT_USER[]= "root";
/** Default password for donor connection*/
static char DEFAULT_PASSWORD[]= "";
/** The number of queued transactions below which we declare the node online */
static uint RECOVERY_TRANSACTION_THRESHOLD= 0;

static void *launch_handler_thread(void* arg)
{
  Recovery_module *handler= (Recovery_module*) arg;
  handler->recovery_thread_handle();
  return 0;
}

Recovery_module::Recovery_module(Applier_module_interface *applier,
                                 GCS::Protocol *protocol)
  :communication_proto(protocol), applier_module(applier), view_id(0),
  selected_donor(NULL), donor_connection_retry_count(0),
  recovery_running(false), donor_transfer_finished(false),
  connected_to_donor(false), donor_connection_interface(),
  stop_wait_timeout(LONG_TIMEOUT), max_connection_attempts_to_donors(-1)
{

  (void) strncpy(donor_connection_user, DEFAULT_USER, strlen(DEFAULT_USER)+1);
  (void) strncpy(donor_connection_password,
                 DEFAULT_PASSWORD, strlen(DEFAULT_PASSWORD)+1);

#ifdef HAVE_PSI_INTERFACE
  PSI_mutex_info recovery_mutexes[]=
  {
    { &recovery_mutex_key, "recovery_LOCK", 0},
    { &run_mutex_key, "recovery_run_LOCK", 0},
    { &donor_selection_mutex_key, "donor_selection_LOCK", 0}
  };

  PSI_cond_info recovery_conds[]=
  {
    { &run_cond_key, "recovery_run_COND", 0},
    { &recovery_cond_key, "recovery_COND", 0}
  };

  register_gcs_psi_keys(recovery_mutexes, 3, recovery_conds, 2);
#endif /* HAVE_PSI_INTERFACE */

  mysql_mutex_init(run_mutex_key, &run_lock, MY_MUTEX_INIT_FAST);
  mysql_cond_init(run_cond_key, &run_cond, 0);
  mysql_mutex_init(recovery_mutex_key, &recovery_lock, MY_MUTEX_INIT_FAST);
  mysql_cond_init(recovery_cond_key, &recovery_condition, 0);
  mysql_mutex_init(donor_selection_mutex_key,
                   &donor_selection_lock,
                   MY_MUTEX_INIT_FAST);

  //Set the locks for waiting on the gcs_replication
  set_recovery_wait_structures(&recovery_condition, &recovery_lock);
}

Recovery_module::~Recovery_module()
{
  mysql_mutex_destroy(&run_lock);
  mysql_cond_destroy(&run_cond);
}

int
Recovery_module::start_recovery(const string& group_name,
                                ulonglong rec_view_id,
                                Member_set& cluster_members)
{
  DBUG_ENTER("Recovery_module::initialize_recovery_thd");

  mysql_mutex_lock(&run_lock);

  this->group_name= group_name;
  this->view_id= rec_view_id;
  this->member_set= cluster_members;

  if(check_recovery_thread_status())
  {
    log_message(MY_ERROR_LEVEL,
                "[Recovery:] A previous recovery session is still running."
                "Please stop the plugin and wait for it to stop.");
    DBUG_RETURN(1);
  }

  //reset the recovery aborted status here to avoid concurrency
  recovery_aborted= false;

  //Set the retry count to be the max number of possible donors
  if(max_connection_attempts_to_donors == -1)
  {
    max_connection_attempts_to_donors= cluster_members.size() -1;
  }

#ifdef HAVE_PSI_INTERFACE
  PSI_thread_info threads[]= {
    { &key_thread_recovery,
      "gcs-recovery-module", PSI_FLAG_GLOBAL
    }
  };
  mysql_thread_register("gcs-recovery-module", threads, 1);
#endif

  if ((mysql_thread_create(key_thread_recovery,
                           &recovery_pthd,
                           get_connection_attrib(),
                           launch_handler_thread,
                           (void*)this)))
  {
    DBUG_RETURN(1);
  }

  while (!recovery_running)
  {
    DBUG_PRINT("sleep",("Waiting for applier thread to start"));
    mysql_cond_wait(&run_cond, &run_lock);
  }

  mysql_mutex_unlock(&run_lock);

  DBUG_RETURN(0);
}

int
Recovery_module::check_recovery_thread_status()
{
  //if some of the threads are running
  if (donor_connection_interface.is_io_thread_running() ||
      donor_connection_interface.is_sql_thread_running())
  {
    return terminate_recovery_slave_threads();
  }
  return 0;
}

int
Recovery_module::stop_recovery()
{
  DBUG_ENTER("Recovery_module::stop_recovery_thread");

  mysql_mutex_lock(&run_lock);

  if (!recovery_running)
  {
    mysql_mutex_unlock(&run_lock);
    DBUG_RETURN(0);
  }

  recovery_aborted= true;

  while (recovery_running)
  {
    DBUG_PRINT("loop", ("killing gcs recovery thread"));

    mysql_mutex_lock(&recovery_thd->LOCK_thd_data);
    /*
      Error codes from pthread_kill are:
      EINVAL: invalid signal number (can't happen)
      ESRCH: thread already killed (can happen, should be ignored)
    */
    int err __attribute__((unused))= pthread_kill(recovery_thd->real_id,
                                                  SIGUSR1);
    DBUG_ASSERT(err != EINVAL);
    recovery_thd->awake(THD::NOT_KILLED);
    mysql_mutex_unlock(&recovery_thd->LOCK_thd_data);

    //Break the wait for the applier suspension
    applier_module->interrupt_applier_suspension_wait();
    //Break the wait for view change event
    mysql_mutex_lock(&recovery_lock);
    mysql_cond_broadcast(&recovery_condition);
    mysql_mutex_unlock(&recovery_lock);

    /*
      There is a small chance that thread might miss the first
      alarm. To protect against it, resend the signal until it reacts
    */
    struct timespec abstime;
    set_timespec(abstime,2);
#ifndef DBUG_OFF
    int error=
#endif
    mysql_cond_timedwait(&run_cond, &run_lock, &abstime);
    if (stop_wait_timeout >= 2)
    {
      stop_wait_timeout= stop_wait_timeout - 2;
    }
    else if (recovery_running) // quit waiting
    {
      mysql_mutex_unlock(&run_lock);
      DBUG_RETURN(1);
    }
    DBUG_ASSERT(error == ETIMEDOUT || error == 0);
  }

  DBUG_ASSERT(!recovery_running);

  mysql_mutex_unlock(&run_lock);

  DBUG_RETURN(0);
}

int
Recovery_module::update_recovery_process(Member_set& left,
                                         Member_set& cluster_members)
{
  DBUG_ENTER("Recovery_module::update_recovery_process");

  int error= 0;

  if (recovery_running)
  {

    string node_id = communication_proto->get_client_uuid();
    string donor_id= selected_donor.get_uuid();

    if (GCS::is_member_in_set(node_id, left))
    {
      stop_recovery();
    }
    else
    {
      /*
        Lock to avoid concurrency between this code that handles failover and
        the establish_donor_connection method. We either:
        1) lock first and see that the method as not run yet, updating the list
           of cluster members that will be used there.
        2) lock after the method executed, and if the selected donor is leaving
           we stop the connection thread and select a new one.
      */
      mysql_mutex_lock(&donor_selection_lock);

      //if some node left, reset the counter as potential failed members left
      if((int)left.size() > 1)
      {
        donor_connection_retry_count= 0;
        rejected_donors.clear();
      }

      /*
       Set the new members for donor selection in case the donor still being
       selected under the normal thread flow, or for future selection in case of
       failover.
      */
      member_set= cluster_members;

      /*
       It makes sense to cut our connect to the donor if:
       1) The donor has left the building
       2) We are already connected to him.
      */
      if (GCS::is_member_in_set(donor_id,left) && connected_to_donor)
      {
        /*
         The donor transfer flag is not lock protected on the recovery thread so
         we have the scenarios.
         1) The flag is true and we do nothing
         2) The flag is false and remains false so we restart the connection, and
         that new connection will deliver the rest of the data
         3) The flag turns true while we are restarting the connection. In this
         case we will probably create a new connection that won't be needed and
         will be terminated the instant the lock is freed.
        */
        if(!donor_transfer_finished)
        {
          log_message(MY_INFORMATION_LEVEL,
                      "[Recovery:] Killing the current recovery connection as the "
                      "donor %s left.", donor_id.c_str());
          if(donor_failover())
          {
            /*
             We can't failover, nothing to do, better exit the group.
             There is yet a possibility that the donor transfer terminated in
             the meanwhile, rendering the error unimportant.
            */
            if(!donor_transfer_finished)
            {
              log_message(MY_ERROR_LEVEL,
                      "[Recovery:] Failover to another donor failed, rendering "
                      "recovery impossible."
                      "The node will now leave to cluster");
              mysql_mutex_unlock(&donor_selection_lock);
              communication_proto->leave(group_name);
              DBUG_RETURN(error);
            }
            else
            {
              log_message(MY_WARNING_LEVEL,
                          "[Recovery:] Failover to another donor failed, but "
                          "recovery already received all the data.");
            }
          }
        }
        //else do nothing
      }
      mysql_mutex_unlock(&donor_selection_lock);
    } //member not on left
  } // recovery_running

  DBUG_RETURN(error);
}

int Recovery_module::donor_failover()
{
  if(donor_connection_interface.is_io_thread_running())
  {
    //Stop only the first one
    int thread_mask= SLAVE_IO;
    int error= 0;
    if ((error= donor_connection_interface.stop_threads(false, thread_mask)))
    {
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] Can't kill the current recovery process. "
                  "Recovery will shutdown.");
      return error;
    }
    return(establish_donor_connection(true));
  }
  return 0;
}

int
Recovery_module::recovery_thread_handle()
{
  int error= 0;
  donor_transfer_finished= false;

  set_recovery_thread_context();
  Certifier_interface *certifier=
    applier_module->get_certification_handler()->get_certifier();

  mysql_mutex_lock(&run_lock);
  recovery_running= true;
  mysql_cond_broadcast(&run_cond);
  mysql_mutex_unlock(&run_lock);

  //a new recovery round is starting, clean the status
  rejected_donors.clear();

  //wait for the appliers suspension
  if(!recovery_aborted &&
     applier_module->wait_for_applier_complete_suspension(&recovery_aborted))
  {
    log_message(MY_ERROR_LEVEL,
                "[Recovery:] Can't evaluate the applier module execution status."
                " Recovery will shutdown to avoid data corruption.");
    goto cleanup;
  }

  reset_retrieved_seq_number();

  if (!recovery_aborted)
  {
    //if the connection to the donor failed, abort recovery
    if((error = establish_donor_connection()))
    {
      goto cleanup;
    }
  }

  mysql_mutex_lock(&recovery_lock);
  while (get_retrieved_seq_number() == -1 && !recovery_aborted)
  {
    mysql_cond_wait(&recovery_condition, &recovery_lock);
  }
  mysql_mutex_unlock(&recovery_lock);

  //if there is no certifier, just ignore the info
  if (certifier != NULL)
  {
    certifier->set_certification_info(get_retrieved_cert_db(),
                                      get_retrieved_seq_number());
  }

  donor_transfer_finished= true;
  connected_to_donor= false;

  /**
    If recovery fails or is aborted, it never makes sense to awake the applier,
    as that would lead to the certification and execution of transactions on
    the wrong context.
  */
  if (!recovery_aborted)
    applier_module->awake_applier_module();

  wait_for_applier_module_recovery();

cleanup:

  //if finished, declare the node online
  if (!recovery_aborted && !error)
    notify_cluster_recovery_end();

  terminate_recovery_slave_threads();

  mysql_mutex_lock(&run_lock);
  recovery_running= false;
  mysql_cond_broadcast(&run_cond);
  mysql_mutex_unlock(&run_lock);

  /*
   If recovery failed, it's no use to continue in the group as the node cannot
   take an active part in the cluster, so it leaves.

   This code can only be invoked after recovery being declared as terminated as
   otherwise it would deadlock with the method waiting for the last view, and
   last view waiting for this thread to die.
  */
  if(error)
    communication_proto->leave(group_name);

  clean_recovery_thread_context();

  return 0;
}

void
Recovery_module::set_recovery_thread_context()
{
  my_thread_init();
  recovery_thd= new THD;
  recovery_thd->thread_stack= (char*) &recovery_thd;
  recovery_thd->store_globals();
  init_thr_lock();

  Global_THD_manager::get_instance()->add_thd(recovery_thd);
}

void
Recovery_module::clean_recovery_thread_context()
{
  recovery_thd->release_resources();
  THD_CHECK_SENTRY(recovery_thd);
  Global_THD_manager::get_instance()->remove_thd(recovery_thd);

  delete recovery_thd;

  my_thread_end();
}

bool Recovery_module::select_donor()
{
  bool no_available_donors= false;
  bool clean_run= rejected_donors.empty();
  while(!no_available_donors)
  {
    std::set<Member>::iterator set_iterator= member_set.begin();
    //select the first online node
    while(set_iterator != member_set.end()){
      Member member = *set_iterator;
      //is online and it's not me and didn't error out before
      string m_uuid=  member.get_uuid();

      bool is_online= member.get_recovery_status() == GCS::MEMBER_ONLINE;
      bool not_self= m_uuid.compare(communication_proto->get_client_uuid());
      bool not_rejected= std::find(rejected_donors.begin(),
                                   rejected_donors.end(),
                                   m_uuid) == rejected_donors.end();

      if ( is_online && not_self && not_rejected )
      {
        selected_donor= *set_iterator;
        return no_available_donors;
      }
      ++set_iterator;
    }

    //no donor was found
    if(!clean_run)
    {
      //There were donors that threw an error before, try again with those
      rejected_donors.clear();
      clean_run= true;
    }
    else
    {
      //no more donor to try with, just report an error
      no_available_donors= true;
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] No suitable donor found, recovery aborting.");
      break;
    }
  }
  return no_available_donors;
}

int Recovery_module::establish_donor_connection(bool failover)
{
  int error= -1;
  connected_to_donor= false;

  while (error != 0 && !recovery_aborted)
  {
    if(!failover)
      mysql_mutex_lock(&donor_selection_lock);

    if((error= select_donor())) //select a donor
    {
      if(!failover)
        mysql_mutex_unlock(&donor_selection_lock);
      return error; //no available donors, abort
    }

 #ifndef DBUG_OFF
    DBUG_EXECUTE_IF("recovery_thread_wait",
                  {
                    const char act[]= "now wait_for signal.recovery_continue";
                    DBUG_ASSERT(!debug_sync_set_action(current_thd, STRING_WITH_LEN(act)));
                  });
#endif // DBUG_OFF

    if(!failover)
    {
      if ((error= initialize_donor_connection()))
      {
        log_message(MY_ERROR_LEVEL,
                "[Recovery:] Error when configuring the connection to the donor.");
      }
    }
    else
    {
      initialize_connection_parameters();
    }

    if (!error && !recovery_aborted)
    {
      error= start_recovery_donor_threads(failover);
    }

    if (error)
    {
      if(donor_connection_retry_count == max_connection_attempts_to_donors)
      {
        log_message(MY_ERROR_LEVEL,
                    "[Recovery:] Maximum number of retries when trying to "
                    "connect to a donor reached. Aborting recovery.");
        if(!failover)
          mysql_mutex_unlock(&donor_selection_lock);
        return error; // max number of retries reached, abort
      }
      else
      {
        donor_connection_retry_count++;
        rejected_donors.push_back(selected_donor.get_uuid());
        log_message(MY_INFORMATION_LEVEL,
                    "[Recovery:] Retrying connection with another donor. "
                    "Attempt %d/%d",
                    donor_connection_retry_count,
                    max_connection_attempts_to_donors);
      }
    }
    else
    {
      connected_to_donor = true;
    }
    if(!failover)
      mysql_mutex_unlock(&donor_selection_lock);
  }

  return error;
}

int Recovery_module::initialize_donor_connection(){

  DBUG_ENTER("Recovery_module::initialize_connection_parameters");

  int error= 0;

  char relay_log_name[]= "gcs_recovery";
  char relay_log_info_name[]= "gcs_recovery_relay_log.info";

  error= donor_connection_interface.initialize_repositories(relay_log_name,
                                                            relay_log_info_name);
  if(error)
  {
    if (error == REPLICATION_THREAD_REPOSITORY_CREATION_ERROR)
    {
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] Failed to setup the donor connection metadata "
                  "containers.");

    }
    if (error == REPLICATION_THREAD_MI_INIT_ERROR)
    {
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] Failed to setup the donor connection (mi)"
                  " metadata container.");
    }
    if (error == REPLICATION_THREAD_RLI_INIT_ERROR)
    {
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] Failed to setup the donor connection (relay log) "
                  "metadata container.");
    }
    DBUG_RETURN(error);
  }

  initialize_connection_parameters();

  donor_connection_interface.initialize_view_id_until_condition(view_id);

  DBUG_RETURN(error);
}

void Recovery_module::initialize_connection_parameters()
{
  const string hostname= selected_donor.get_hostname();
  uint port= selected_donor.get_port();

  donor_connection_interface.initialize_connection_parameters(&hostname,
          port, donor_connection_user, donor_connection_password, NULL, 1);

  log_message(MY_INFORMATION_LEVEL,
          "[Recovery:] Establishing connection to donor %s at %s@%s"
          " port: %d.", selected_donor.get_uuid().c_str(),
          donor_connection_user, hostname.c_str(), port);
}


int Recovery_module::start_recovery_donor_threads(bool failover)
{
  DBUG_ENTER("Recovery_module::initialize_sql_thread");

  int error= 0;
  /*
   On new connection both threads should be started
   On failover connections, only the IO thread should be started.
   On why we cannot use init_thread_mask for this:
     During failover the running SQL thread, can process a View_change event
     and stop.
     In that scenario if we reached this code we could restart the SQL thread
     making it apply events that are past the view change, events that are also
     queued in the applier module's queue.
  */
  int thread_mask= (failover) ? SLAVE_IO : SLAVE_SQL | SLAVE_IO;

  error= donor_connection_interface.start_replication_threads(thread_mask, true);
  if (error)
  {
    if (error == REPLICATION_THREAD_START_ERROR)
    {
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] Error on the recovery's IO/SQL thread "
                  "initialization");
    }
    if (error == REPLICATION_THREAD_START_NO_INFO_ERROR)
    {
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] No information available when starting the SQL "
                  "thread due to an error on the relay log initialization");
    }
    if(error == REPLICATION_THREAD_START_IO_NOT_CONNECTED)
    {
      log_message(MY_ERROR_LEVEL,
                  "[Recovery:] There was an error when connecting to the donor "
                  "server. Check the node connection credentials.");
    }
  }

  DBUG_RETURN(error);
}

int Recovery_module::terminate_recovery_slave_threads()
{
  DBUG_ENTER("Recovery_module::terminate_slave_threads");

  log_message(MY_INFORMATION_LEVEL,
              "[Recovery:] Terminating existing donor connection.");
  /*
   Lock to avoid concurrent donor failover attempts when we are already
   terminating the threads.
  */
  mysql_mutex_lock(&donor_selection_lock);

  int error= 0;

  if((error= donor_connection_interface.stop_threads(false)))
  {
    log_message(MY_ERROR_LEVEL,
            "[Recovery:] Error when stopping the recovery's slave thread");
  }
  else
  {
    donor_connection_interface.clean_thread_repositories();
  }

  // TODO: purge the relay logs

  mysql_mutex_unlock(&donor_selection_lock);
  DBUG_RETURN(error);
}

void Recovery_module::wait_for_applier_module_recovery()
{

  bool applier_monitoring= true;
  while (!recovery_aborted && applier_monitoring)
  {
    ulong queue_size = applier_module->get_message_queue_size();
    if(queue_size <= RECOVERY_TRANSACTION_THRESHOLD)
    {
      applier_monitoring= false;
    }
    else
    {
      usleep(100 * queue_size);
    }
  }
}

void Recovery_module::notify_cluster_recovery_end()
{
  Recovery_message *recovery_msg
    = new Recovery_message(RECOVERY_END_MESSAGE,
                           &(communication_proto->get_client_uuid()));

  GCS::Message msg(recovery_msg, GCS::MSG_GCS_CLIENT);
  communication_proto->broadcast(msg);
}
