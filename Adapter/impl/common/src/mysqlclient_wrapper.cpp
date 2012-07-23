/*
 Copyright (c) 2012, Oracle and/or its affiliates. All rights
 reserved.
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; version 2 of
 the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301  USA
 */

#include <mysql.h>

#include <node.h>
#include "js_wrapper_macros.h"
#include "NativeCFunctionCall.h"

using namespace v8;

Handle<Value> mysql_init_wrapper(const Arguments &args) {
  HandleScope scope;
  
  REQUIRE_ARGS_LENGTH(0);
  
  NativeCFunctionCall_1_<MYSQL *, MYSQL *> ncall(args);
  ncall.arg0 = 0;  // override 
  ncall.function = & mysql_init;
  ncall.run();
  
  return scope.Close(ncall.jsReturnVal());
}


Handle<Value> mysql_close_wrapper(const Arguments &args) {
  HandleScope scope;
  
  REQUIRE_ARGS_LENGTH(0);
  
  NativeCVoidFunctionCall_1_<MYSQL *> ncall(args);
  ncall.function = & mysql_close;
  ncall.run();
  
  return scope.Close(ncall.jsReturnVal());
}


void mysqlclient_initOnLoad(Handle<Object> target) {
  DEFINE_JS_FUNCTION(target, "mysql_init", mysql_init_wrapper);
  DEFINE_JS_FUNCTION(target, "mysql_close", mysql_close_wrapper);
}

NODE_MODULE(mysqlclient, mysqlclient_initOnLoad)



  
  