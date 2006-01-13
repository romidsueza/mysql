/* Copyright (C) 2000-2003 MySQL AB

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Typedefs for long names
typedef NdbDictionary::Object NDBOBJ;
typedef NdbDictionary::Column NDBCOL;
typedef NdbDictionary::Table NDBTAB;
typedef NdbDictionary::Index  NDBINDEX;
typedef NdbDictionary::Dictionary  NDBDICT;
typedef NdbDictionary::Event  NDBEVENT;

#define IS_TMP_PREFIX(A) (is_prefix(A, tmp_file_prefix) || is_prefix(A, "@0023sql"))

extern ulong ndb_extra_logging;

#ifdef HAVE_NDB_BINLOG

#define INJECTOR_EVENT_LEN 200

enum SCHEMA_OP_TYPE
{
  SOT_DROP_TABLE,
  SOT_CREATE_TABLE,
  SOT_RENAME_TABLE,
  SOT_ALTER_TABLE,
  SOT_DROP_DB,
  SOT_CREATE_DB,
  SOT_ALTER_DB,
  SOT_CLEAR_SLOCK
};

const uint max_ndb_nodes= 64; /* multiple of 32 */

extern pthread_t ndb_binlog_thread;
extern pthread_mutex_t injector_mutex;
extern pthread_cond_t  injector_cond;

static const char *ha_ndb_ext=".ndb";
static const char share_prefix[]= "./";

extern unsigned char g_node_id_map[max_ndb_nodes];
extern handlerton ndbcluster_hton;
extern pthread_t ndb_util_thread;
extern pthread_mutex_t LOCK_ndb_util_thread;
extern pthread_cond_t COND_ndb_util_thread;
extern int ndbcluster_util_inited;
extern pthread_mutex_t ndbcluster_mutex;
extern HASH ndbcluster_open_tables;
extern Ndb_cluster_connection* g_ndb_cluster_connection;
extern long ndb_number_of_storage_nodes;

/*
  Initialize the binlog part of the ndb handlerton
*/
void ndbcluster_binlog_init_handlerton();
/*
  Initialize the binlog part of the NDB_SHARE
*/
void ndbcluster_binlog_init_share(NDB_SHARE *share, TABLE *table);

int ndbcluster_create_binlog_setup(Ndb *ndb, const char *key,
                                   const char *db,
                                   const char *table_name,
                                   NDB_SHARE *share);
int ndbcluster_create_event(Ndb *ndb, const NDBTAB *table,
                            const char *event_name, NDB_SHARE *share);
int ndbcluster_create_event_ops(NDB_SHARE *share,
                                const NDBTAB *ndbtab,
                                const char *event_name);
int ndbcluster_log_schema_op(THD *thd, NDB_SHARE *share,
                             const char *query, int query_length,
                             const char *db, const char *table_name,
                             uint32 ndb_table_id,
                             uint32 ndb_table_version,
                             enum SCHEMA_OP_TYPE type);
int ndbcluster_handle_drop_table(Ndb *ndb, const char *event_name,
                                 NDB_SHARE *share);
void ndb_rep_event_name(String *event_name,
                        const char *db, const char *tbl);

int ndbcluster_binlog_start();
pthread_handler_t ndb_binlog_thread_func(void *arg);

/*
  table cluster_replication.apply_status
*/
void ndbcluster_setup_binlog_table_shares(THD *thd);
extern NDB_SHARE *apply_status_share;
extern NDB_SHARE *schema_share;

extern THD *injector_thd;
extern int ndb_binlog_thread_running;

bool
ndbcluster_show_status_binlog(THD* thd, stat_print_fn *stat_print,
                              enum ha_stat_type stat_type);

/*
  prototypes for ndb handler utility function also needed by
  the ndb binlog code
*/
int ndbcluster_find_all_files(THD *thd);
void ndb_unpack_record(TABLE *table, NdbValue *value,
                       MY_BITMAP *defined, byte *buf);

NDB_SHARE *ndbcluster_get_share(const char *key,
                                TABLE *table,
                                bool create_if_not_exists,
                                bool have_lock);
NDB_SHARE *ndbcluster_get_share(NDB_SHARE *share);
void ndbcluster_free_share(NDB_SHARE **share, bool have_lock);
void ndbcluster_real_free_share(NDB_SHARE **share);
int handle_trailing_share(NDB_SHARE *share);
inline NDB_SHARE *get_share(const char *key,
                            TABLE *table,
                            bool create_if_not_exists= TRUE,
                            bool have_lock= FALSE)
{
  return ndbcluster_get_share(key, table, create_if_not_exists, have_lock);
}

inline NDB_SHARE *get_share(NDB_SHARE *share)
{
  return ndbcluster_get_share(share);
}

inline void free_share(NDB_SHARE **share, bool have_lock= FALSE)
{
  ndbcluster_free_share(share, have_lock);
}

inline void real_free_share(NDB_SHARE **share)
{
  ndbcluster_real_free_share(share);
}

inline
Thd_ndb *
get_thd_ndb(THD *thd) { return (Thd_ndb *) thd->ha_data[ndbcluster_hton.slot]; }

inline
void
set_thd_ndb(THD *thd, Thd_ndb *thd_ndb) { thd->ha_data[ndbcluster_hton.slot]= thd_ndb; }

Ndb* check_ndb_in_thd(THD* thd);


#endif /* HAVE_NDB_BINLOG */
