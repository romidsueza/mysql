/* Copyright (c) 2016, 2023, Oracle and/or its affiliates.

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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#ifndef MYSQL_BUFFER_H
#define MYSQL_BUFFER_H

#include "keyring_memory.h"
#include "i_serialized_object.h"

namespace keyring
{

class Buffer : public ISerialized_object
{
public:
  Buffer() : data(NULL)
  {
    mark_as_empty();
  }
  Buffer(size_t memory_size) : data(NULL)
  {
    reserve(memory_size);
  }
  ~Buffer()
  {
    if(data != NULL)
      delete[] data;
  }

  inline void free();
  my_bool get_next_key(IKey **key);
  my_bool has_next_key();
  void reserve(size_t memory_size);

  uchar *data;
  size_t size;
  size_t position;
private:
  Buffer(const Buffer&);
  Buffer& operator=(const Buffer&);

  inline void mark_as_empty()
  {
    size= position= 0;
  }
};

} //namespace keyring

#endif //MYSQL_BUFFER_H
