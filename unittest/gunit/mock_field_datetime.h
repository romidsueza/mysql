/* Copyright (c) 2011, 2023, Oracle and/or its affiliates.

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

#ifndef MOCK_FIELD_DATETIME_H
#define MOCK_FIELD_DATETIME_H

#include "field.h"

class Mock_field_datetime : public Field_datetime
{
  void initialize()
  {
    ptr= buffer;
    memset(buffer, 0, PACK_LENGTH);
  }

public:
  uchar buffer[PACK_LENGTH];
  Mock_field_datetime() : Field_datetime(false, "") { initialize(); }

};

#endif // MOCK_FIELD_DATETIME_H
