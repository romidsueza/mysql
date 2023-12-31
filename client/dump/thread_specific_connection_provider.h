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

#ifndef THREAD_SPECIFIC_CONNECTION_PROVIDER_INCLUDED
#define THREAD_SPECIFIC_CONNECTION_PROVIDER_INCLUDED

#include "abstract_connection_provider.h"
#include "thread_specific_ptr.h"
#include "base/mutex.h"
#include <vector>

namespace Mysql{
namespace Tools{
namespace Dump{

class Thread_specific_connection_provider : public Abstract_connection_provider
{
public:
  Thread_specific_connection_provider(
    Mysql::Tools::Base::I_connection_factory* connection_factory);
  ~Thread_specific_connection_provider();

  virtual Mysql::Tools::Base::Mysql_query_runner* get_runner(
    Mysql::I_callable<bool, const Mysql::Tools::Base::Message_data&>*
    message_handler);

private:
  my_boost::thread_specific_ptr<Mysql::Tools::Base::Mysql_query_runner>
    m_runner;

  std::vector<Mysql::Tools::Base::Mysql_query_runner*> m_runners_created;
  my_boost::mutex m_runners_created_lock;
};

}
}
}

#endif
