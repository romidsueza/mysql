/*
   Copyright (c) 2011, 2023, Oracle and/or its affiliates.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef NDB_THD_H
#define NDB_THD_H

#include <mysql/plugin.h>

extern handlerton* ndbcluster_hton;

/* Get Thd_ndb pointer from THD */
static inline
class Thd_ndb*
thd_get_thd_ndb(THD* thd)
{
  return (class Thd_ndb *) thd_get_ha_data(thd, ndbcluster_hton);
}
  
/* Backward compatibility alias for 'thd_get_thd_ndb'  */
static inline
class Thd_ndb*
get_thd_ndb(THD* thd)
{
  return thd_get_thd_ndb(thd);
}


/* Set Thd_ndb pointer for THD */
static inline
void
thd_set_thd_ndb(THD *thd, class Thd_ndb *thd_ndb)
{
  thd_set_ha_data(thd, ndbcluster_hton, thd_ndb);
}


/* Make sure THD has a Thd_ndb struct assigned */
class Ndb* check_ndb_in_thd(THD* thd, bool validate_ndb= false);


/* Print thd's list of warnings to error log */
void
thd_print_warning_list(THD* thd, const char* prefix);

/*
  Determine if THD is applying binlog. ie. either marked as
  slave thread or being in "pseudo slave mode"
*/
bool
applying_binlog(const THD* thd);

#endif
