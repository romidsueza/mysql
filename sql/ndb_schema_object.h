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

#ifndef NDB_SCHEMA_OBJECT_H
#define NDB_SCHEMA_OBJECT_H

/*
  Used for communication between the SQL thread performing
  a schema operation and the schema disctribution thread.

  The SQL thread creates one NDB_SCHEMA_OBJECT in the hash and
  when the schema distribution thread has received new events it will
  update the entry with slock info from the other nodes in the
  cluster, finally the slock bitmap will empty and the SQL thread
  knows that the schema operation has completed and will delete
  the entry.
*/


#include <my_thread.h> // my_bitmap.h
#include <my_bitmap.h>

struct NDB_SCHEMA_OBJECT {
  native_mutex_t mutex;
  char *key;
  size_t key_length;
  uint use_count;
  MY_BITMAP slock_bitmap;
  uint32 slock[256/32]; // 256 bits for lock status of table
  uint32 table_id;
  uint32 table_version;
};

NDB_SCHEMA_OBJECT *ndb_get_schema_object(const char *key,
                                         bool create_if_not_exists);

void ndb_free_schema_object(NDB_SCHEMA_OBJECT **ndb_schema_object);

#endif
