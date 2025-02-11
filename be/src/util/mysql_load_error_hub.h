// This file is made available under Elastic License 2.0.
// This file is based on code available under the Apache license here:
//   https://github.com/apache/incubator-doris/blob/master/be/src/util/mysql_load_error_hub.h

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

#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <utility>

#include "gen_cpp/InternalService_types.h"
#include "util/load_error_hub.h"

#ifndef __StarRocksMysql
#define __StarRocksMysql void
#endif

namespace starrocks {

// For now every load job has its own mysql connection,
// and we use short connection to avoid to many connections.
// we write to mysql in a batch of data, not every data error msg,

class MysqlLoadErrorHub : public LoadErrorHub {
public:
    struct MysqlInfo {
        std::string host;
        int32_t port;
        std::string user;
        std::string passwd;
        std::string db;
        std::string table;

        MysqlInfo(std::string h, int32_t p, std::string u, std::string pwd, std::string d, std::string t)
                : host(std::move(h)),
                  port(p),
                  user(std::move(u)),
                  passwd(std::move(pwd)),
                  db(std::move(d)),
                  table(std::move(t)) {}

        MysqlInfo(const TMysqlErrorHubInfo& t_info)
                : host(t_info.host),
                  port(t_info.port),
                  user(t_info.user),
                  passwd(t_info.passwd),
                  db(t_info.db),
                  table(t_info.table) {}
    };

    MysqlLoadErrorHub(const TMysqlErrorHubInfo& info);

    ~MysqlLoadErrorHub() override;

    Status prepare() override;

    Status export_error(const ErrorMsg& error_msg) override;

    Status close() override;

    std::string debug_string() const override;

private:
    Status open_mysql_conn(__StarRocksMysql** my_conn);

    Status write_mysql();

    Status gen_sql(__StarRocksMysql* my_conn, const LoadErrorHub::ErrorMsg& error_msg, std::stringstream* sql_stream);

    Status error_status(const std::string& prefix, __StarRocksMysql* my_conn);

    MysqlInfo _info;

    // the number in a write batch.
    static const int32_t EXPORTER_THRESHOLD = 100;
    static const int32_t EXPORTER_MAX_ERROR_NUM = 50;

    // the max size of one line
    static const int32_t EXPORTER_MAX_LINE_SIZE = 500;

    std::mutex _mtx;
    std::queue<ErrorMsg> _error_msgs;
    int32_t _total_error_num = 0;

    // should at least (line_length * 2 + 1) long
    std::array<char, 2 * EXPORTER_MAX_LINE_SIZE + 1> _escape_buff;

}; // end class MysqlLoadErrorHub

} // end namespace starrocks
