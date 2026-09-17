// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include "adbc_connection_pool.hpp"
#include "adbc_util.hpp"

namespace duckdb {
namespace adbc {
void AdbcScanFunction(ClientContext &context, TableFunctionInput &input, DataChunk &output);
unique_ptr<FunctionData> AdbcScanBindFunction(ClientContext &context,
                                              TableFunctionBindInput &input,
                                              vector<LogicalType> &return_types,
                                              vector<Identifier> &names);

// A factory class that holds the ADBC connection state and produces
// ArrowArrayStreamWrapper instances
class AdbcArrowStreamFactory {
public:
    // Create an ephemeral connection (i.e., read_adbc(...) is called directly)
    AdbcArrowStreamFactory(const string &uri, const string &query_text);
    // Use a connection from the catalog's pool (i.e., SELECT * FROM <adbc>)
    AdbcArrowStreamFactory(unique_ptr<AdbcPooledConnection> connection, const string &query_text);
    // same type of constructor, but allows specification of the table name -Marios
    AdbcArrowStreamFactory(unique_ptr<AdbcPooledConnection> conn, const string &query_text, string t_name);
    AdbcStatement *GetStatement();
    void ResetStatement();
    // using the table name field
    // and with projection columns passed
    // creates a new SQL statement
    void SetStatementProjection(const vector<string> &columns);
    // telling if the sql statement
    // being passed to the attach'd DB has column projection
    // or not (meaning we've nontrivially called SetStatementProjection)
    // this is controlled by table_name.empty()
    bool IsProjectPushdown();

private:
    unique_ptr<AdbcPooledConnection> connection;
    string query_text;
    Handle<Private::AdbcStatement> statement;
    string table_name;
};

// A wrapper class to take ownership of the factory object (and the
// corresponding ADBC state) during the scan
class AdbcArrowScanFunctionData : public ArrowScanFunctionData {
public:
    // Pass the factory and the factory function that creates an ArrowArrayStream
    AdbcArrowScanFunctionData(ClientContext &context, unique_ptr<AdbcArrowStreamFactory> factory);
    // reaches into adbc_arrow_stream_factory and exposes if we're pushing
    // the columnar projection down to the attached DB
    bool IsProjectPushdown();

private:
    unique_ptr<AdbcArrowStreamFactory> adbc_arrow_stream_factory;

public:
    optional_idx cardinality;
};

} // namespace adbc
} // namespace duckdb
