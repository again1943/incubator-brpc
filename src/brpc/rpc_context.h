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


#ifndef BRPC_RPC_CONTEXT_H
#define BRPC_RPC_CONTEXT_H

namespace brpc {

// Derive this class to custom rpc context for different protocols.
//
// Background:
//
// For some non-standard protocols, which may have obscure protocol settings.
// If we directly put the code that manipulating those settings into the rpc
// controller, that will cause much more chaos and the code will not be easy
// to understand.
//
// This RpcContext can be derived by different protocol context, encapsulating
// their protocol only data, options and settings. The ownership of the context
// is transfered to Controller once the context is set/put into the Controller.
class RpcContext {
public:
    virtual ~RpcContext() {}
};

}
#endif
