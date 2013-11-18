/*
   Copyright (c) 2000-2003 MySQL AB, 2008 Sun Microsystems, Inc.
   Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <portlib/NdbTick.h>

/*
  These functions are shared with ndb_restore so that the creating of
  tables through ndb_restore is syncronized correctly with the mysqld's

  The lock/unlock functions use the BACKUP_SEQUENCE row in SYSTAB_0

  retry_time == 0 means no retry
  retry_time <  0 means infinite retries
  retry_time >  0 means retries for max 'retry_time' seconds
*/
static NdbTransaction *
ndbcluster_global_schema_lock_ext(THD *thd, Ndb *ndb, NdbError &ndb_error,
                                  int retry_time= 10)
{
  ndb->setDatabaseName("sys");
  ndb->setDatabaseSchemaName("def");
  NdbDictionary::Dictionary *dict= ndb->getDictionary();
  Ndb_table_guard ndbtab_g(dict, "SYSTAB_0");
  const NdbDictionary::Table *ndbtab= NULL;
  NdbOperation *op;
  NdbTransaction *trans= NULL;
  int retry_sleep= 50; /* 50 milliseconds, transaction */
  NDB_TICKS start;

  if (retry_time > 0)
  {
    start = NdbTick_getCurrentTicks();
  }
  while (1)
  {
    if (!ndbtab)
    {
      if (!(ndbtab= ndbtab_g.get_table()))
      {
        if (dict->getNdbError().status == NdbError::TemporaryError)
          goto retry;
        ndb_error= dict->getNdbError();
        goto error_handler;
      }
    }

    trans= ndb->startTransaction();
    if (trans == NULL)
    {
      ndb_error= ndb->getNdbError();
      goto error_handler;
    }

    op= trans->getNdbOperation(ndbtab);
    op->readTuple(NdbOperation::LM_Exclusive);
    op->equal("SYSKEY_0", NDB_BACKUP_SEQUENCE);

    if (trans->execute(NdbTransaction::NoCommit) == 0)
      break;

    if (trans->getNdbError().status != NdbError::TemporaryError)
      goto error_handler;
    else if (thd->killed)
      goto error_handler;
  retry:
    if (retry_time == 0)
      goto error_handler;
    if (retry_time > 0)
    {
      const NDB_TICKS now = NdbTick_getCurrentTicks();
      if (NdbTick_Elapsed(start,now).seconds() > (Uint64)retry_time)
        goto error_handler;
    }
    if (trans)
    {
      ndb->closeTransaction(trans);
      trans= NULL;
    }
    do_retry_sleep(retry_sleep);
  }
  return trans;

 error_handler:
  if (trans)
  {
    ndb_error= trans->getNdbError();
    ndb->closeTransaction(trans);
  }
  return NULL;
}

static int
ndbcluster_global_schema_unlock_ext(Ndb *ndb, NdbTransaction *trans,
                                    NdbError &ndb_error)
{
  if (trans->execute(NdbTransaction::Commit))
  {
    ndb_error= trans->getNdbError();
    ndb->closeTransaction(trans);
    return -1;
  }
  ndb->closeTransaction(trans);
  return 0;
}
