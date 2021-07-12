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

// brpc - A framework to host and access services throughout Baidu.

#ifndef BRPC_TEST_BRPC_TEST_UTIL_H
#define BRPC_TEST_BRPC_TEST_UTIL_H

namespace brpc {
namespace test {

// Test cases that related to network/port will break if the ports
// they are using are occupied by other processes.
// FindUnusedTcpPort offers an portable way to get an unused, random
// port to make such unittest case more table.
// Returns:
//    true if success and *port contains the random port.
//    false if fail
bool FindUnusedTcpPort(int* port);

}
}

#endif
