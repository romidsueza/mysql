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

#include "hash_to_buffer_serializer.h"

namespace keyring
{
  my_bool Hash_to_buffer_serializer::store_key_in_buffer(const IKey* key,
                                                         Buffer *buffer)
  {
    if (buffer->size < buffer->position + key->get_key_pod_size())
      return TRUE;
    key->store_in_buffer(buffer->data, &(buffer->position));
    return FALSE;
  }

  my_bool Hash_to_buffer_serializer::store_keys_in_buffer(HASH *keys_hash,
                                                          Buffer *buffer)
  {
    for (uint i= 0 ; i < keys_hash->records ; ++i)
    {
      if(store_key_in_buffer(reinterpret_cast<const IKey *>(my_hash_element(keys_hash, i)),
                             buffer))
        return TRUE;
    }
    return FALSE;
  }

  ISerialized_object* Hash_to_buffer_serializer::serialize(HASH *keys_hash,
                                                           IKey *key,
                                                           const Key_operation operation)
  {
    size_t memory_needed_for_buffer_after_operation= memory_needed_for_buffer;

    switch(operation)
    {
      case STORE_KEY: memory_needed_for_buffer_after_operation += key->get_key_pod_size();
                      break;
      case REMOVE_KEY: memory_needed_for_buffer_after_operation -= key->get_key_pod_size();
                       break;
      case NONE:
      case ROTATE:
        break;
    }

    Buffer *buffer= new Buffer(memory_needed_for_buffer_after_operation);
    buffer->set_key_operation(operation);
    if (store_keys_in_buffer(keys_hash, buffer))
    {
      delete buffer;
      return NULL;
    }
    return buffer;
  }
} //namespace keyring
