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


#ifndef BRPC_SOFA_BOLT_HEADER_ACCESSOR_H
#define BRPC_SOFA_BOLT_HEADER_ACCESSOR_H
#include "butil/iobuf.h"
#include "butil/logging.h"
#include "butil/binary_printer.h"
#include "butil/sys_byteorder.h"
#include "butil/type_traits.h"
#include "brpc/policy/sofa_bolt_defines.h"

namespace brpc {
namespace policy {

#define ENABLE_IF_VERSION(TemplateType, SofaBoltVersion, ReturnType) \
            typename butil::enable_if<butil::is_same<TemplateType, HeaderType>::value && \
                    HeaderTrait::version == SofaBoltVersion, ReturnType>::type

#define ENABLE_IF_TYPE(TemplateType, SofaBoltHeaderType, ReturnType) \
            typename butil::enable_if<butil::is_same<TemplateType, HeaderType>::value && \
                    HeaderTrait::type == SofaBoltHeaderType, ReturnType>::type

#define ENABLE_IF_VERSION_AND_TYPE(TemplateType, SofaBoltVersion, SofaBoltHeaderType, ReturnType) \
            typename butil::enable_if<butil::is_same<TemplateType, HeaderType>::value && \
                    HeaderTrait::version == SofaBoltVersion && \
                    HeaderTrait::type == SofaBoltHeaderType, ReturnType>::type

template<typename SofaBoltHeader>
class SofaBoltHeaderReadAccessor {
public:
    typedef SofaBoltHeader HeaderType;
    typedef SofaBoltHeaderTrait<HeaderType> HeaderTrait;

    SofaBoltHeaderReadAccessor(SofaBoltHeader* header, bool need_network_to_host_reorder = true) : _header(header) {
        if (need_network_to_host_reorder) {
            HeaderNetworkOrderToHost(header); 
        } 
    }

    const SofaBoltHeader* GetHeader() const {
        return _header;
    }

    SofaBoltProtocolVersion GetProtocol() const {
        return static_cast<SofaBoltProtocolVersion>(_header->proto);
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, bool) CheckVer1IfApplicable() const {
        return true;
    }

    // 1. The ver2 value should be as same as proto value for sofa-bolt v1 and v2. 
    // 2. From v2, proto will remain as ver1 and new version will use an newer ver1 value.
    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, bool) CheckVer1IfApplicable() const {
        return _header->proto == _header->ver1;
    }

    // Will never be called for V1 protocol.
    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, uint8_t) GetVer1IfApplicable() const {
        return 0;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, uint8_t) GetVer1IfApplicable() const {
        return _header->ver1;
    }

    uint8_t GetHeaderType() const {
        return _header->type;
    }

    bool CheckHeaderType(SofaBoltHeaderType type) const {
        return _header->type == type;
    }

    bool CheckCmdCode(SofaBoltCommandCodeType cmd) const {
        return _header->cmd_code == cmd;
    }

    uint16_t GetCmdCode() const {
        return _header->cmd_code;
    }

    // Ver2 is not used, make sure it is set to 0.
    bool CheckVer2() const {
        return _header->ver2 == 0;
    }

    uint32_t GetRequestId() const {
        return _header->request_id;
    }

    bool CheckCodec(SofaBoltCodecType codec) const {
        return _header->codec == codec;
    }

    uint8_t GetCodec() const {
        return _header->codec;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, uint8_t) GetOptions() const {
        return 0;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, uint8_t) GetOptions() const {
        return _header->options;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, bool) HasCrcCheckOption() const {
        return false;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, bool) HasCrcCheckOption() const {
        return (_header->options  & SOFA_BOLT_CRC_CHECK) > 0;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, void) SetEnableCrcCheckIfApplicable() {}

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, void) SetEnableCrcCheckIfApplicable() {
        _header->options |=  SOFA_BOLT_CRC_CHECK;
    }

    uint16_t GetClassLen() const {
        return _header->class_len;
    }

    uint16_t GetHeaderLen() const {
        return _header->header_len;
    }

    uint32_t GetContentLen() const {
        return _header->content_len;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_TYPE(Q, SOFA_BOLT_RESPONSE, uint16_t) GetResponseStatus() const {
        return _header->resp_status;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_TYPE(Q, SOFA_BOLT_RESPONSE, bool)
    CheckResponseStatus(SofaBoltResponseStatus status) const {
        return _header->resp_status == status;
    }
private:
    template<typename Q = HeaderType>
    ENABLE_IF_TYPE(Q, SOFA_BOLT_RESPONSE, void)
    HeaderNetworkOrderToHost(Q* header) {
        header->cmd_code = butil::NetToHost16(header->cmd_code);
        header->request_id = butil::NetToHost32(header->request_id);
        header->resp_status = butil::NetToHost16(header->resp_status);
        header->class_len = butil::NetToHost16(header->class_len);
        header->header_len = butil::NetToHost16(header->header_len);
        header->content_len = butil::NetToHost32(header->content_len);
    }
private:
    const SofaBoltHeader* _header;
};

template<typename SofaBoltHeader>
class SofaBoltHeaderWriteAccessor {
public:
    typedef SofaBoltHeader HeaderType;
    typedef SofaBoltHeaderTrait<HeaderType> HeaderTrait;

    SofaBoltHeaderWriteAccessor(SofaBoltHeader* header) : _header(header) {
        if (HeaderTrait::type == SOFA_BOLT_REQUEST) {
            memset(_header, 0, sizeof(SofaBoltHeader));
        }
    }

    const SofaBoltHeader* GetHeader() const {
        return _header;
    }

    void SetProtocol(SofaBoltProtocolVersion version) {
        _header->proto = version;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, void) SetVer1IfApplicable(SofaBoltProtocolVersion /** ignored for V1 **/) {}

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, void) SetVer1IfApplicable(SofaBoltProtocolVersion version) {
        _header->ver1 = version;
    }

    void SetHeaderType(SofaBoltHeaderType type) {
        _header->type = type;
    }

    void SetCmdCode(SofaBoltCommandCodeType cmd) {
        _header->cmd_code = cmd;
    }

    void SetRequestId(uint32_t request_id) {
        _header->request_id = request_id;
    }

    void SetCodec(SofaBoltCodecType codec) {
        _header->codec = codec;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, bool) IsCrc32CheckEnabledIfApplicable() {
        return false;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, bool) IsCrc32CheckEnabledIfApplicable() {
        return (_header->options &  SOFA_BOLT_CRC_CHECK) > 0;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V1, void) SetEnableCrcCheckIfApplicable() {}

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION(Q, SOFA_BOLT_V2, void) SetEnableCrcCheckIfApplicable() {
        _header->options |=  SOFA_BOLT_CRC_CHECK;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_TYPE(Q, SOFA_BOLT_REQUEST, void) SetTimeout(uint32_t timeout_ms) {
        _header->timeout = timeout_ms;
    }

    void SetClassLen(uint16_t len) {
        _header->class_len = len;
    } 

    void SetHeaderLen(uint16_t len) {
        _header->header_len = len;
    } 

    void SetContentLen(uint32_t len) {
        _header->content_len = len;
    }

    template<typename Q = HeaderType>
    ENABLE_IF_TYPE(Q, SOFA_BOLT_REQUEST, void) HeaderHostOrderToNetwork() {
        _header->cmd_code = butil::HostToNet16(_header->cmd_code);
        _header->request_id = butil::HostToNet32(_header->request_id);
        _header->timeout = butil::HostToNet32(_header->timeout);
        _header->class_len = butil::HostToNet16(_header->class_len);
        _header->header_len = butil::HostToNet16(_header->header_len);
        _header->content_len = butil::HostToNet32(_header->content_len);
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION_AND_TYPE(Q, SOFA_BOLT_V1, SOFA_BOLT_REQUEST, void) PackToIOBuf(butil::IOBuf* out) {
        butil::IOBufAppender appender;
        appender.push_back(_header->proto); 
        appender.push_back(_header->type);
        appender.append(&_header->cmd_code, 2);
        appender.push_back(_header->ver2);
        appender.append(&_header->request_id, 4);
        appender.push_back(_header->codec);
        appender.append(&_header->timeout, 4);
        appender.append(&_header->class_len, 2);
        appender.append(&_header->header_len, 2);
        appender.append(&_header->content_len, 4);
        appender.move_to(*out);
    }

    template<typename Q = HeaderType>
    ENABLE_IF_VERSION_AND_TYPE(Q, SOFA_BOLT_V2, SOFA_BOLT_REQUEST, void) PackToIOBuf(butil::IOBuf* out) {
        butil::IOBufAppender appender;
        appender.push_back(_header->proto); 
        appender.push_back(_header->ver1);
        appender.push_back(_header->type);
        appender.append(&_header->cmd_code, 2);
        appender.push_back(_header->ver2);
        appender.append(&_header->request_id, 4);
        appender.push_back(_header->codec);
        appender.push_back(_header->options);
        appender.append(&_header->timeout, 4);
        appender.append(&_header->class_len, 2);
        appender.append(&_header->header_len, 2);
        appender.append(&_header->content_len, 4);
        appender.move_to(*out);
    }
private:
    SofaBoltHeader* _header;
};

#undef ENABLE_IF_VERSION_AND_TYPE
#undef ENABLE_IF_TYPE
#undef ENABLE_IF_VERSION

}
}
#endif
