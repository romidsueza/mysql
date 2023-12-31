/* Copyright (c) 2014, 2023, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */


#include "observer_server_actions.h"
#include "observer_trans.h"

int group_replication_reset_master_logs(Binlog_transmit_param *param)
{
  register_server_reset_master();
  return 0;
}

int group_replication_transmit_start(Binlog_transmit_param *param,
                                     const char *log_file, my_off_t log_pos)
{
  return 0;
}

int group_replication_transmit_stop(Binlog_transmit_param *param)
{
  return 0;
}

int group_replication_reserve_header(Binlog_transmit_param *param,
                                     unsigned char *header,
                                     unsigned long size,
                                     unsigned long *len)
{
  return 0;
}

int group_replication_before_send_event(Binlog_transmit_param *param,
                                        unsigned char *packet,
                                        unsigned long len,
                                        const char *log_file,
                                        my_off_t log_pos)
{
  return 0;
}

int group_replication_after_send_event(Binlog_transmit_param *param,
                                       const char *event_buf,
                                       unsigned long len,
                                       const char *skipped_log_file,
                                       my_off_t skipped_log_pos)
{
  return 0;
}

Binlog_transmit_observer binlog_transmit_observer = {
  sizeof(Binlog_transmit_observer),

  group_replication_transmit_start,     // transmit_start,
  group_replication_transmit_stop,      // transmit_stop,
  group_replication_reserve_header,     // reserve_header,
  group_replication_before_send_event,  // before_send_event,
  group_replication_after_send_event,   // after_send_event,
  group_replication_reset_master_logs   // reset_master
};
