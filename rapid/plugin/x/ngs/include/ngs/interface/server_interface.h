/*
 * Copyright (c) 2015, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef NGS_SERVER_INTERFACE_H_
#define NGS_SERVER_INTERFACE_H_

#include "ngs/protocol_authentication.h"
#include "ngs_common/smart_ptr.h"


namespace ngs {

class Client_interface;
class Server;
class Session_interface;
class Scheduler_dynamic;
class Protocol_encoder;
class Protocol_config;
class Mutex;

class Server_interface {
public:
  virtual ~Server_interface() {}


  virtual void get_authentication_mechanisms(std::vector<std::string> &auth_mech, Client_interface &client) = 0;

  virtual ngs::shared_ptr<Scheduler_dynamic> get_worker_scheduler() const = 0;
  virtual Authentication_handler_ptr         get_auth_handler(const std::string &name, Session_interface *session) = 0;
  virtual ngs::shared_ptr<Protocol_config>   get_config() const = 0;
  virtual Mutex &get_client_exit_mutex() = 0;

  virtual Ssl_context *ssl_context() const = 0;

  virtual ngs::shared_ptr<Session_interface> create_session(
      Client_interface &client,
      Protocol_encoder &proto,
      int session_id) = 0;

  virtual bool is_running() = 0;

  virtual void on_client_closed(const Client_interface &client) = 0;
  virtual void restart_client_supervision_timer() = 0;
};

} // namespace ngs

#endif // NGS_SERVER_INTERFACE_H_
