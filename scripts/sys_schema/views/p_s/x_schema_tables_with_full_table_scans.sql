-- Copyright (c) 2014, 2023, Oracle and/or its affiliates.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License, version 2.0,
-- as published by the Free Software Foundation.
--
-- This program is also distributed with certain software (including
-- but not limited to OpenSSL) that is licensed under separate terms,
-- as designated in a particular file or component or in included license
-- documentation.  The authors of MySQL hereby grant you an additional
-- permission to link the program and your derivative works with the
-- separately licensed software that they have included with MySQL.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License, version 2.0, for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

--
-- View: x$schema_tables_with_full_table_scans
--
-- Find tables that are being accessed by full table scans
-- ordering by the number of rows scanned descending.
--
-- mysql> select * from x$schema_tables_with_full_table_scans limit 5;
-- +--------------------+------------------------------+-------------------+----------------+
-- | object_schema      | object_name                  | rows_full_scanned | latency        |
-- +--------------------+------------------------------+-------------------+----------------+
-- | mem30__instruments | fsstatistics                 |          10207042 | 13098927688488 |
-- | mem30__instruments | preparedstatementapidata     |            436428 |   973274338980 |
-- | mem30__instruments | mysqlprocessactivity         |            411702 |   282072434940 |
-- | mem30__instruments | querycachequeriesincachedata |            374011 |   767152380564 |
-- | mem30__instruments | rowaccessesdata              |            322321 |  1547594778456 |
-- +--------------------+------------------------------+-------------------+----------------+
--

CREATE OR REPLACE
  ALGORITHM = MERGE
  DEFINER = 'mysql.sys'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$schema_tables_with_full_table_scans (
  object_schema,
  object_name,
  rows_full_scanned,
  latency
) AS
SELECT object_schema, 
       object_name,
       count_read AS rows_full_scanned,
       sum_timer_wait AS latency
  FROM performance_schema.table_io_waits_summary_by_index_usage 
 WHERE index_name IS NULL
   AND count_read > 0
 ORDER BY count_read DESC;
