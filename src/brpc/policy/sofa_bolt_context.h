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


#ifndef BRPC_SOFA_BOLT_CONTEXT_H
#define BRPC_SOFA_BOLT_CONTEXT_H

#include <stdint.h>                             // int64_t
#include <memory>
#include <string>                               // std::string
#include <utility>
#include "brpc/kvmap.h"
#include "brpc/rpc_context.h"                   // RpcContext
#include "brpc/policy/sofa_bolt_defines.h"

namespace brpc {
namespace policy {

// Special context for sofa bolt to handle protocol specific options.
class SofaBoltContext : public RpcContext {
public:
    friend class SofaBoltHeartBeatRequestContextMaker;
    friend class SofaBoltRequestContextMaker;
    friend class SofaBoltOneWayRequestContextMaker;
    friend class SofaBoltContextPrivateAccessor;
public:
    virtual ~SofaBoltContext() {}

    SofaBoltProtocolVersion GetRequestProtocolVersion() const {
        return _request_protocol_version; 
    }

    bool RequestCrc32CheckEnabled() const {
        return (_request_options & SOFA_BOLT_CRC_CHECK) > 0;
    }

    const std::string& GetRequestServiceName() const {
        return _request_service_name;
    }

    const std::string& GetRequestServiceUniqueId() const {
        return _request_service_unique_id;
    }

    const std::string& GetRequestServiceVersion() const {
        return _request_service_version;
    }

    bool HasResponseHeaderMap() const {
        return _response_header_kv.operator bool();
    }

    const KVMap& GetResponseHeaderMap() const {
        return *(_response_header_kv.get());
    }

    bool IsOneWayRequest() const {
        return _request_header_type == SOFA_BOLT_ONEWAY;
    }

    SofaBoltCommandCodeType GetRequestCmdCode() const {
        return _request_cmd_code;
    }

    SofaBoltHeaderType GetRequestHeaderType() const {
        return _request_header_type;
    }

    bool IsHeartBeatRequest() const {
        return _request_cmd_code == SOFA_BOLT_CMD_HEARTBEAT;
    }

    const std::string GetResponseClassName() const {
        return _response_class_name;
    }
public:
    void SetRequestProtocolVersion(SofaBoltProtocolVersion version) {
        _request_protocol_version = version;
    }

    void MarkAsOneWayRequest() {
        _request_header_type = SOFA_BOLT_ONEWAY;
    }

    void MarkAsHeartBeatRequest() {
        _request_cmd_code = SOFA_BOLT_CMD_HEARTBEAT;
    }

    void RequestEnableCrc32Check() {
        _request_options |= SOFA_BOLT_CRC_CHECK;
    }

    void SetRequestServiceName(const std::string& service_name) {
        _request_service_name = service_name;
    }

    void SetRequestServiceName(std::string&& service_name) {
        _request_service_name = service_name;
    }

    void SetRequestServiceUniqueId(const std::string& unique_id) {
        _request_service_unique_id = unique_id;
    }

    void SetRequestServiceUniqueId(std::string&& unique_id) {
        _request_service_unique_id = unique_id;
    }

    void SetRequestServiceVersion(const std::string& service_version) {
        _request_service_version = service_version;
    }

    void SetRequestServiceVersion(std::string&& service_version) {
        _request_service_version = service_version;
    }
public:
    void AddResponseHeader(const std::string& key, const std::string& value) {
        if (!HasResponseHeaderMap()) {
            _response_header_kv.reset(new KVMap);
        }
        _response_header_kv->Set(key, value);
    }

    void SetResponseClassName(const std::string& class_name) {
        _response_class_name = class_name;
    }

    void SetResponseClassName(std::string&& class_name) {
       _response_class_name = class_name;
    }

    SofaBoltResponseStatus GetResponseStatusCode() const {
        return _response_status_code;
    }
private:
    // Make constructor private so that SofaBoltContext can only be created from friend derieved classes.
    SofaBoltContext() {
        _request_protocol_version = SOFA_BOLT_V1;
        _request_cmd_code = SOFA_BOLT_CMD_REQUEST;
        _request_header_type = SOFA_BOLT_REQUEST;
        _request_options = 0;
    }

    void SetResponseStatusCode(SofaBoltResponseStatus status) {
        _response_status_code = status;
    }
private:
    /***********************Request Settings Goes Here***********************/
    // So far, only CRC32 option(value 0x1) is used in SOFA_BOLT_V2. 
    //So if user code set protocol to SOFA_BOLT_V1, options field will be ignored.
    uint8_t _request_options;
    SofaBoltProtocolVersion _request_protocol_version;
    // For client side, either SOFA_BOLT_CMD_REQUEST or SOFA_BOLT_CMD_ONEWAY
    SofaBoltCommandCodeType _request_cmd_code;
    // For client side, either SOFA_BOLT_REQUEST or SOFA_BOLT_CMD_HEARTBEAT
    SofaBoltHeaderType _request_header_type;
    // Service name for remote serivce, this name may not as same as the protobuf
    // generated service name. User code must manually set the service name if necessary.
    std::string _request_service_name;
    // The service version, if not set, default is "1.0"
    std::string _request_service_version;
    // The service version, if not set, default is empty
    std::string _request_service_unique_id;
    /**********************Response Setttings Goes Here************************/
    // Sofa bolt response header map. We keep it as a pointer because it's not always that a sofa 
    // bolt server responds a header map.
    std::unique_ptr<KVMap>   _response_header_kv;
    // Sofa bolt response class name
    std::string _response_class_name;
    // Response status code
    SofaBoltResponseStatus _response_status_code;
};

class SofaBoltRequestContextMaker : public SofaBoltContext {
private:
    SofaBoltRequestContextMaker() {}
    using SofaBoltContext::MarkAsOneWayRequest;
    using SofaBoltContext::MarkAsHeartBeatRequest;
    using SofaBoltContext::AddResponseHeader;
public:
    static SofaBoltRequestContextMaker* create() {
        return new SofaBoltRequestContextMaker();
    }
    SofaBoltContext* GetContext() {
        return this;
    }
    virtual ~SofaBoltRequestContextMaker() {}
};

class SofaBoltHeartBeatRequestContextMaker : public SofaBoltContext {
private:
    SofaBoltHeartBeatRequestContextMaker() {}
    using SofaBoltContext::SetRequestServiceName;
    using SofaBoltContext::SetRequestServiceUniqueId;
    using SofaBoltContext::SetRequestServiceVersion;
    using SofaBoltContext::AddResponseHeader;
    using SofaBoltContext::MarkAsOneWayRequest;
public:
    static SofaBoltHeartBeatRequestContextMaker* create() {
        return new SofaBoltHeartBeatRequestContextMaker();
    }
    SofaBoltContext* GetContext() {
        return this;
    }
    virtual ~SofaBoltHeartBeatRequestContextMaker() {}
};

// Due to BRPC's current mechanism, oneway requests are not supported.
class SofaBoltOneWayRequestContextMaker : public SofaBoltContext {
private:
    SofaBoltOneWayRequestContextMaker() {};
    using SofaBoltContext::AddResponseHeader;
    using SofaBoltContext::MarkAsHeartBeatRequest;
public:
    static SofaBoltOneWayRequestContextMaker* create() {
        return new SofaBoltOneWayRequestContextMaker();
    }

    SofaBoltContext* GetContext() {
        return this;
    }
    virtual ~SofaBoltOneWayRequestContextMaker() {}
};

class SofaBoltContextPrivateAccessor {
public:
    SofaBoltContextPrivateAccessor(SofaBoltContext& context) : _reference(context) {}
    SofaBoltContextPrivateAccessor(SofaBoltContext* context) : _reference(*context) {}

    void AddResponseHeader(const std::string& key, const std::string& value) {
        _reference.AddResponseHeader(key, value);
    }

    void SetResponseClassName(const std::string& class_name) {
        _reference.SetResponseClassName(class_name);
    }

    void SetResponseClassName(std::string&& class_name) {
        _reference.SetResponseClassName(std::move(class_name));
    }

    void SetResponseStatusCode(SofaBoltResponseStatus status) {
        _reference.SetResponseStatusCode(status);
    }

    void SetResponseStatusCode(uint16_t status) {
        _reference.SetResponseStatusCode(static_cast<SofaBoltResponseStatus>(status));
    }
private:
    SofaBoltContext& _reference;
};

}
}
#endif
