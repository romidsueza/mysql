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
-- Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

--
-- View: x$wait_classes_global_by_latency
-- 
-- Lists the top wait classes by total latency, ignoring idle (this may be very large).
--
-- mysql> SELECT * FROM x$wait_classes_global_by_latency;
-- +-------------------+---------+----------------+-------------+----------------+--------------+
-- | event_class       | total   | total_latency  | min_latency | avg_latency    | max_latency  |
-- +-------------------+---------+----------------+-------------+----------------+--------------+
-- | wait/io/file      |   29468 | 27100905420290 |           0 | 919672370.7170 | 350700491310 |
-- | wait/io/table     |  224924 |   719670285750 |      116870 |   3199615.3623 | 208579012460 |
-- | wait/synch/mutex  | 1532036 |   118515948070 |       56550 |     77358.4616 |   2590408470 |
-- | wait/io/socket    |    1193 |    10677541030 |           0 |   8950160.1257 |    287760330 |
-- | wait/lock/table   |    6972 |     3674766030 |      109330 |    527074.8752 |      8855730 |
-- | wait/synch/rwlock |   13646 |     1579833580 |       37700 |    115772.6499 |     28293850 |
-- +-------------------+---------+----------------+-------------+----------------+--------------+
--

CREATE OR REPLACE
  ALGORITHM = TEMPTABLE
  DEFINER = 'mysql.sys'@'localhost'
  SQL SECURITY INVOKER 
VIEW x$wait_classes_global_by_latency (
  event_class,
  total,
  total_latency,
  min_latency,
  avg_latency,
  max_latency
) AS
SELECT SUBSTRING_INDEX(event_name,'/', 3) AS event_class, 
       SUM(COUNT_STAR) AS total,
       SUM(sum_timer_wait) AS total_latency,
       MIN(min_timer_wait) AS min_latency,
       IFNULL(SUM(sum_timer_wait) / NULLIF(SUM(COUNT_STAR), 0), 0) AS avg_latency,
       MAX(max_timer_wait) AS max_latency
  FROM performance_schema.events_waits_summary_global_by_event_name
 WHERE sum_timer_wait > 0
   AND event_name != 'idle'
 GROUP BY SUBSTRING_INDEX(event_name,'/', 3) 
 ORDER BY SUM(sum_timer_wait) DESC;
