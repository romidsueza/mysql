# Copyright (c) 2016, 2023, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

INCLUDE(CheckCSourceRuns)
INCLUDE(CheckCXXSourceRuns)

SET(code "
  int main (int argc, char **argv)
  {
    double n[21] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 1, 1, 1, 1, 1, 1, 1,1 };
    double m= 0, s= 0;
    int i;
    for(i= 0; i < 21; i++)
    {
      double m_kminusone= m;
      m= m_kminusone + (n[i] - m_kminusone) / (double) (i + 2);
      s= s + (n[i] - m_kminusone) * (n[i] - m);
    }
    /*
      s should now be either 5e 74 d1 45 17 5d 14 40 or
      40 14 5d 17 45 d1 74 5e, depending on endianness. If the floating point
      operations are optimized with fused multiply-add instructions, the least
      significant byte is 5d instead of 5e.
    */
    return (*(unsigned char*)(&s) == 0x5e ||
            *((unsigned char*)(&s) + 7) == 0x5e);
  }"
)

SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
SET(CMAKE_REQUIRED_FLAGS
  "${CMAKE_REQUIRED_FLAGS} -O3"
)

IF(CMAKE_COMPILER_IS_GNUCC)
  CHECK_C_SOURCE_RUNS("${code}" HAVE_C_FLOATING_POINT_FUSED_MADD)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX)
  CHECK_CXX_SOURCE_RUNS("${code}" HAVE_CXX_FLOATING_POINT_FUSED_MADD)
ENDIF()

SET(CMAKE_REQUIRED_FLAGS
  "${CMAKE_REQUIRED_FLAGS} -ffp-contract=off"
)

IF(CMAKE_COMPILER_IS_GNUCC)
  CHECK_C_SOURCE_COMPILES("${code}" HAVE_C_FP_CONTRACT_FLAG)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX)
  CHECK_CXX_SOURCE_COMPILES("${code}" HAVE_CXX_FP_CONTRACT_FLAG)
ENDIF()

SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
