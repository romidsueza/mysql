/*
  Copyright (c) 2015, 2023, Oracle and/or its affiliates.

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

#ifndef ABSTRACT_CONNECTION_PROVIDER_INCLUDED
#define ABSTRACT_CONNECTION_PROVIDER_INCLUDED

#include "i_connection_provider.h"
#include "base/i_connection_factory.h"
#include "i_callable.h"

namespace Mysql{
namespace Tools{
namespace Dump{

class Abstract_connection_provider : public I_connection_provider
{
protected:
  Abstract_connection_provider(
    Mysql::Tools::Base::I_connection_factory* connection_factory);

  virtual Mysql::Tools::Base::Mysql_query_runner* create_new_runner(
    Mysql::I_callable<bool, const Mysql::Tools::Base::Message_data&>*
    message_handler);

private:
  Mysql::Tools::Base::I_connection_factory* m_connection_factory;

  class Message_handler_wrapper
  {
  public:
    Message_handler_wrapper(
      Mysql::I_callable<bool, const Mysql::Tools::Base::Message_data&>*
      message_handler);

    int64 pass_message(const Mysql::Tools::Base::Message_data& message);

  private:
    Mysql::I_callable<bool, const Mysql::Tools::Base::Message_data&>*
      m_message_handler;
  };
};

}
}
}

#endif
