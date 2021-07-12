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

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#include "brpc_test_util.h"

namespace brpc {
namespace test {

// Test cases that related to network/port will break if the ports
// they are using are occupied by other processes.
// FindUnusedTcpPort offers an portable way to get an unused, random
// port to make such unittest case more table.

bool FindUnusedTcpPort(int* port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return false;
  }

  int enable_socket_reuse = 1;
  if (setsockopt(
    fd, SOL_SOCKET, SO_REUSEADDR, &enable_socket_reuse, sizeof(int)) < 0) {
    close(fd);
    return false;
  } 

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(fd, (struct sockaddr*)(&addr), sizeof(addr)) < 0) {
    close(fd);
    return false;
  }

  socklen_t len = sizeof(addr);
  if (getsockname(fd, (struct sockaddr*)(&addr), &len) < 0) {
    close(fd);
    return false;
  }

  *port = ntohs(addr.sin_port);
  return true;
}

}
}

