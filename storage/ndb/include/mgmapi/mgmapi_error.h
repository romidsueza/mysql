/*
   Copyright (C) 2003 MySQL AB
    All rights reserved. Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef MGMAPI_ERROR_H
#define MGMAPI_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif
  /**
   *    Error codes
   */
  enum ndb_mgm_error {
    /** Not an error */
    NDB_MGM_NO_ERROR = 0,

    /* Request for service errors */
    /** Supplied connectstring is illegal */
    NDB_MGM_ILLEGAL_CONNECT_STRING = 1001,
    /** Supplied NdbMgmHandle is illegal */
    NDB_MGM_ILLEGAL_SERVER_HANDLE = 1005,
    /** Illegal reply from server */
    NDB_MGM_ILLEGAL_SERVER_REPLY = 1006,
    /** Illegal number of nodes */
    NDB_MGM_ILLEGAL_NUMBER_OF_NODES = 1007,
    /** Illegal node status */
    NDB_MGM_ILLEGAL_NODE_STATUS = 1008,
    /** Memory allocation error */
    NDB_MGM_OUT_OF_MEMORY = 1009,
    /** Management server not connected */
    NDB_MGM_SERVER_NOT_CONNECTED = 1010,
    /** Could not connect to socker */
    NDB_MGM_COULD_NOT_CONNECT_TO_SOCKET = 1011,
    /** Could not bind local address */
    NDB_MGM_BIND_ADDRESS = 1012,
    
    /* Alloc node id failures */
    /** Generic error, retry may succeed */
    NDB_MGM_ALLOCID_ERROR = 1101,
    /** Non retriable error */
    NDB_MGM_ALLOCID_CONFIG_MISMATCH = 1102,

    /* Service errors - Start/Stop Node or System */
    /** Start failed */
    NDB_MGM_START_FAILED = 2001,
    /** Stop failed */
    NDB_MGM_STOP_FAILED = 2002,
    /** Restart failed */
    NDB_MGM_RESTART_FAILED = 2003,

    /* Service errors - Backup */
    /** Unable to start backup */
    NDB_MGM_COULD_NOT_START_BACKUP = 3001,
    /** Unable to abort backup */
    NDB_MGM_COULD_NOT_ABORT_BACKUP = 3002,

    /* Service errors - Single User Mode */
    /** Unable to enter single user mode */
    NDB_MGM_COULD_NOT_ENTER_SINGLE_USER_MODE = 4001,
    /** Unable to exit single user mode */
    NDB_MGM_COULD_NOT_EXIT_SINGLE_USER_MODE = 4002,

    /* Usage errors */
    /** Usage error */
    NDB_MGM_USAGE_ERROR = 5001
  };
#ifndef DOXYGEN_SHOULD_SKIP_INTERNAL
  struct Ndb_Mgm_Error_Msg {
    enum ndb_mgm_error  code;
    const char *        msg;
  };
  extern const struct Ndb_Mgm_Error_Msg ndb_mgm_error_msgs[];
  extern const int ndb_mgm_noOfErrorMsgs;
#endif

#ifdef __cplusplus
}
#endif

#endif
