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
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */


/* This struct includes all reserved words and functions */

#ifndef _lex_symbol_h
#define _lex_symbol_h

enum SYM_GROUP
{
  SG_KEYWORDS=          1 << 0, // SQL keywords and reserved words
  SG_FUNCTIONS=         1 << 1, // very special native SQL functions
  SG_HINTABLE_KEYWORDS= 1 << 2, // SQL keywords that accept optimizer hints
  SG_HINTS=             1 << 3, // optimizer hint parser keywords

  /* All tokens of the main parser: */
  SG_MAIN_PARSER=       SG_KEYWORDS | SG_HINTABLE_KEYWORDS | SG_FUNCTIONS
};

struct SYMBOL {
  const char *name;
  const unsigned int length;
  const unsigned int tok;
  int group; //< group mask, see SYM_GROUP enum for bits
};

struct LEX_SYMBOL
{
  const SYMBOL *symbol;
  char   *str;
  unsigned int length;
};

#endif /* _lex_symbol_h */
