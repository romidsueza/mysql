/* Copyright (c) 2012, 2023, Oracle and/or its affiliates.

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

#ifndef PFS_STAGE_PROVIDER_H
#define PFS_STAGE_PROVIDER_H

/**
  @file include/pfs_stage_provider.h
  Performance schema instrumentation (declarations).
*/

#ifdef HAVE_PSI_STAGE_INTERFACE
#ifdef MYSQL_SERVER
#ifndef EMBEDDED_LIBRARY
#ifndef MYSQL_DYNAMIC_PLUGIN

#include "mysql/psi/psi.h"

#define PSI_STAGE_CALL(M) pfs_ ## M ## _v1

C_MODE_START

void pfs_register_stage_v1(const char *category,
                           PSI_stage_info_v1 **info_array,
                           int count);

PSI_stage_progress* pfs_start_stage_v1(PSI_stage_key key, const char *src_file, int src_line);
PSI_stage_progress* pfs_get_current_stage_progress_v1();

void pfs_end_stage_v1();

C_MODE_END

#endif /* MYSQL_DYNAMIC_PLUGIN */
#endif /* EMBEDDED_LIBRARY */
#endif /* MYSQL_SERVER */
#endif /* HAVE_PSI_STAGE_INTERFACE */

#endif

