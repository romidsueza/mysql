/* Copyright (c) 2000, 2023, Oracle and/or its affiliates.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA */

/* read record through position and fix key-position */
/* As mi_rsame but supply a position */

#include "myisamdef.h"


	/*
	** If inx >= 0 update index pointer
	** Returns one of the following values:
	**  0 = Ok.
	** HA_ERR_KEY_NOT_FOUND = Row is deleted
	** HA_ERR_END_OF_FILE   = End of file
	*/

int mi_rsame_with_pos(MI_INFO *info, uchar *record, int inx, my_off_t filepos)
{
  DBUG_ENTER("mi_rsame_with_pos");
  DBUG_PRINT("enter",("index: %d  filepos: %ld", inx, (long) filepos));

  if (inx < -1 ||
      (inx >= 0 && ! mi_is_key_active(info->s->state.key_map, inx)))
  {
    set_my_errno(HA_ERR_WRONG_INDEX);
    DBUG_RETURN(HA_ERR_WRONG_INDEX);
  }

  info->update&= (HA_STATE_CHANGED | HA_STATE_ROW_CHANGED);
  if ((*info->s->read_rnd)(info,record,filepos,0))
  {
    if (my_errno() == HA_ERR_RECORD_DELETED)
      set_my_errno(HA_ERR_KEY_NOT_FOUND);
    DBUG_RETURN(my_errno());
  }
  info->lastpos=filepos;
  info->lastinx=inx;
  if (inx >= 0)
  {
    info->lastkey_length=_mi_make_key(info,(uint) inx,info->lastkey,record,
				      info->lastpos);
    info->update|=HA_STATE_KEY_CHANGED;		/* Don't use indexposition */
  }
  DBUG_RETURN(0);
} /* mi_rsame_pos */
