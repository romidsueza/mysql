/*
   Copyright (c) 2006, 2014, Oracle and/or its affiliates. All rights reserved.

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

#ifndef DYNARR256_HPP
#define DYNARR256_HPP

#include "Pool.hpp"
#include <NdbMutex.h>

#define JAM_FILE_ID 299


class DynArr256;
struct DA256Page;

class DynArr256Pool
{
  friend class DynArr256;
public:
  DynArr256Pool();
  
  void init(Uint32 type_id, const Pool_context& pc);
  void init(NdbMutex*, Uint32 type_id, const Pool_context& pc);

  Uint32 getUsed()   { return m_used; }  // # entries currently seized
  Uint32 getUsedHi() { return m_usedHi;} // high water mark for getUsed()

protected:
  Uint32 m_type_id;
  Uint32 m_first_free;
  Uint32 m_last_free;
  Pool_context m_ctx;
  struct DA256Page* m_memroot;
  NdbMutex * m_mutex;

  Uint32 m_used;
  Uint32 m_usedHi;
private:
  Uint32 seize();
  void release(Uint32);
};

class DynArr256
{
public:
  struct Head
  {
#if defined VM_TRACE || defined ERROR_INSERT
    Head() { m_ptr_i = RNIL; m_sz = 0; m_high_pos = 0; }
#else
    Head() { m_ptr_i = RNIL; m_sz = 0;}
#endif

    Uint32 m_ptr_i;
    Uint32 m_sz;
#if defined VM_TRACE || defined ERROR_INSERT
    Uint32 m_high_pos;
#endif

    bool isEmpty() const { return m_sz == 0;}
  };
  
  DynArr256(DynArr256Pool & pool, Head& head) : 
    m_head(head), m_pool(pool){}
  
  Uint32* set(Uint32 pos);
  Uint32* get(Uint32 pos) const ;
  Uint32* get_dirty(Uint32 pos) const ;

  struct ReleaseIterator
  {
    Uint32 m_sz;
    Uint32 m_pos;
    Uint32 m_ptr_i[5];
  };
  
  void init(ReleaseIterator&);
  /**
   * return 0 - done
   *        1 - data (in retptr)
   *        2 - nodata
   */
  Uint32 release(ReleaseIterator&, Uint32* retptr);
  Uint32 trim(Uint32 trim_pos, ReleaseIterator&);
  Uint32 truncate(Uint32 trunc_pos, ReleaseIterator&, Uint32* retptr);
protected:
  Head & m_head;
  DynArr256Pool & m_pool;
  
  bool expand(Uint32 pos);
  void handle_invalid_ptr(Uint32 pos, Uint32 ptrI, Uint32 p0);
};

inline
Uint32 DynArr256::release(ReleaseIterator& iter, Uint32* retptr)
{
  return truncate(0, iter, retptr);
}

inline
Uint32 DynArr256::trim(Uint32 pos, ReleaseIterator& iter)
{
  return truncate(pos, iter, NULL);
}

inline
Uint32 * DynArr256::get(Uint32 pos) const
{
#if defined VM_TRACE || defined ERROR_INSERT
  // In debug this function will abort if used
  // with pos not already mapped by call to set.
  // Use get_dirty if return NULL is wanted instead.
  require((m_head.m_sz > 0) && (pos <= m_head.m_high_pos));
#endif
  return get_dirty(pos);
}

#undef JAM_FILE_ID

#endif
