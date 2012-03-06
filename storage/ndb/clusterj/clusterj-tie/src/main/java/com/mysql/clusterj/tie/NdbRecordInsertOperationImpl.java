/*
 *  Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */

package com.mysql.clusterj.tie;

import com.mysql.clusterj.core.store.Table;

public class NdbRecordInsertOperationImpl extends NdbRecordOperationImpl {

    public NdbRecordInsertOperationImpl(ClusterTransactionImpl clusterTransaction, Table storeTable) {
        super(clusterTransaction, storeTable);
        this.ndbRecordValues = clusterTransaction.getCachedNdbRecordImpl(storeTable);
        this.ndbRecordKeys = ndbRecordValues;
        this.valueBufferSize = ndbRecordValues.getBufferSize();
        this.numberOfColumns = ndbRecordValues.getNumberOfColumns();
        this.blobs = new NdbRecordBlobImpl[this.numberOfColumns];
        resetMask();
    }

    public void beginDefinition() {
        // allocate a buffer for the operation data
        valueBuffer = ndbRecordValues.newBuffer();
        keyBuffer = valueBuffer;
    }

    public void endDefinition() {
        ndbOperation = insert(clusterTransaction);
    }

    @Override
    public String toString() {
        return " insert " + tableName;
    }

}
