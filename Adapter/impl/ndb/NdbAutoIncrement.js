/*
 Copyright (c) 2013, Oracle and/or its affiliates. All rights
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

"use strict";

var adapter         = require(path.join(build_dir, "ndb_adapter.node")).ndb,
    stats_module    = require(path.join(api_dir,"stats.js")),
    QueuedAsyncCall = require(path.join(spi_dir,"common","QueuedAsyncCall.js")).QueuedAsyncCall,
    stats           = stats_module.getWriter("spi","ndb","autoincrement"),
    udebug          = unified_debug.getLogger("NdbAutoIncrement.js");


/** NdbAutoIncrementCache 
    Wraps an Ndb object which manages (and caches) auto-inc values for a table.
    The API is that first you call prefetch(n) indicating how many values you
    want. Then you call getValue() once for each desired value.
*/

function NdbAutoIncrementCache(table) {
  udebug.log("New cache for table", table.name);
  this.table = table;
  this.impl = table.ndb_auto_inc;
  this.execQueue = [];
}

NdbAutoIncrementCache.prototype = {
  table         : null,
  impl          : null,
  execQueue     : null,
  batch_size    : 1
}

NdbAutoIncrementCache.prototype.close = function() {
  this.impl.close();
};

NdbAutoIncrementCache.prototype.prefetch = function(n) {
  this.batch_size += n;
};

NdbAutoIncrementCache.prototype.getValue = function(callback) {
  var cache = this;
  var apiCall = new QueuedAsyncCall(this.execQueue, callback);
  udebug.log("NdbAutoIncrementCache getValue table:", this.table.name, 
             "queue:", this.execQueue.length, "batch:", this.batch_size);
  apiCall.description = "AutoIncrementCache getValue";
  apiCall.batch_size = this.batch_size;
  apiCall.run = function() {
    adapter.impl.getAutoIncrementValue(cache.impl, cache.table, this.batch_size,
                                       this.callback);
    cache.batch_size--;
  }
  apiCall.enqueue();
};


function getAutoIncCacheForTable(table) {
  if(table.ndb_auto_inc) {
    table.autoIncrementCache = new NdbAutoIncrementCache(table);
    delete table.ndb_auto_inc;
  }
}


/** NdbAutoIncrementHandler 
    This provides a service to NdbTransactionHandler.execute() 
    which, given a list of operations, may need to populate them with 
    auto-inc values before executing.
*/

function NdbAutoIncrementHandler(operations) {
  var i, op;
  this.autoinc_op_list = [];
  for(i = 0 ; i < operations.length ; i++) { 
    op = operations[i];
    if(typeof op.tableHandler.autoIncColumnNumber === 'number' && 
       op.opcode === 'insert') {
      this.values_needed++;    
      this.autoinc_op_list.push(op);
      op.tableHandler.dbTable.autoIncrementCache.prefetch(1);
    }
  }
  udebug.log("New Handler",this.autoinc_op_list.length,"/",operations.length);
}

NdbAutoIncrementHandler.prototype = {
  values_needed   : 0,
  autoinc_op_list : null,
  final_callback  : null,
  errors          : 0
}


function makeOperationCallback(handler, op) {
  return function(err, value) {
    udebug.log("getValue operation callback value:", value, "qlen:",
               op.tableHandler.dbTable.autoIncrementCache.execQueue.length);
    handler.values_needed--;
    if(value > 0) {
      op.tableHandler.setAutoincrement(op.values, value);
    }
    else {
      handler.erorrs++;
    }
    if(handler.values_needed === 0) {
      handler.dispatchFinalCallback();
    }
  }
}


NdbAutoIncrementHandler.prototype.dispatchFinalCallback = function() {
  this.final_callback(this.errors, this);
};


NdbAutoIncrementHandler.prototype.getAllValues = function(callback) {
  var i, op, cache;
  udebug.log("getAllValues");
  this.final_callback = callback;
  for(i = 0 ; i < this.autoinc_op_list.length ; i++) {
    op = this.autoinc_op_list[i];
    cache = op.tableHandler.dbTable.autoIncrementCache;
    cache.getValue(makeOperationCallback(this, op));
  }
};
  

function getAutoIncrementHandler(operationList) {
  var handler = new NdbAutoIncrementHandler(operationList);
  if(handler.values_needed === 0) {
    handler = null;
  }
  return handler;
}


exports.getCacheForTable = getAutoIncCacheForTable;
exports.getHandler = getAutoIncrementHandler;

