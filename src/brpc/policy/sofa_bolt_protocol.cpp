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


#include <limits>
#include <unordered_map>
#include <google/protobuf/descriptor.h>         // MethodDescriptor
#include <google/protobuf/message.h>            // Message
#include <gflags/gflags.h>
#include <sys/types.h>
#include "brpc/errno.pb.h"
#include "brpc/parse_result.h"
#include "butil/binary_printer.h"
#include "butil/crc32c.h"
#include "butil/endpoint.h"
#include "butil/fast_rand.h"
#include "butil/logging.h"                       // LOG()
#include "butil/time.h"
#include "butil/iobuf.h"                         // butil::IOBuf
#include "butil/sys_byteorder.h"
#include "butil/type_traits.h"
#include "brpc/controller.h"               // Controller
#include "brpc/details/controller_private_accessor.h"
#include "brpc/socket.h"                   // Socket
#include "brpc/server.h"                   // Server
#include "brpc/details/server_private_accessor.h"
#include "brpc/span.h"
#include "brpc/compress.h"                 // ParseFromCompressedData
#include "brpc/options.pb.h"               // ServiceOptions
#include "brpc/policy/most_common_message.h"
#include "butil/containers/flat_map.h"
#include "butil/iobuf.h"
#include "brpc/policy/hasher.h"
#include "brpc/policy/sofa_bolt_defines.h"
#include "brpc/policy/sofa_bolt_protocol.h"
#include "brpc/policy/sofa_bolt_header_accessor.h"
#include "brpc/policy/sofa_bolt_context.h"

namespace brpc {
namespace policy {

#define SOFA_BOLT_HEADER_SIZE_CHECK(header, size)  \
        BAIDU_CASSERT(sizeof(header) == size, header##_size_must_be_##size##_bytes)
SOFA_BOLT_HEADER_SIZE_CHECK(SofaBoltResponseHeaderV1Packed, 20);
SOFA_BOLT_HEADER_SIZE_CHECK(SofaBoltResponseHeaderV2Packed, 22);
#undef SOFA_BOLT_HEADER_SIZE_CHECK

const std::unordered_map<int, const char*> g_sofa_bolt_status_message {
    {SOFA_BOLT_RESPONSE_STATUS_SUCCESS, "success"},
    {SOFA_BOLT_RESPONSE_STATUS_ERROR, "error"},
    {SOFA_BOLT_RESPONSE_STATUS_SERVER_EXCEPTION, "server exception"},
    {SOFA_BOLT_RESPONSE_STATUS_UNKNOWN, "unknown"},
    {SOFA_BOLT_RESPONSE_STATUS_SERVER_THREADPOOL_BUSY, "server threadpool busy"},
    {SOFA_BOLT_RESPONSE_STATUS_ERROR_COMM, "communication error"},
    {SOFA_BOLT_RESPONSE_STATUS_NO_PROCESSOR, "no processor find"},
    {SOFA_BOLT_RESPONSE_STATUS_TIMEOUT, "timeout"},
    {SOFA_BOLT_RESPONSE_STATUS_CLIENT_SEND_ERROR, "client send error"},
    {SOFA_BOLT_RESPONSE_STATUS_CODEC_EXCEPTION, "exception in encode or decode"},
    {SOFA_BOLT_RESPONSE_STATUS_CONNECTION_CLOSED,  "connection closed"},
    {SOFA_BOLT_RESPONSE_STATUS_SERVER_SERIAL_EXCEPTION, "server serialize exception"},
    {SOFA_BOLT_RESPONSE_STATUS_SERVER_DESERIAL_EXCEPTION, "server deserialize exception"},
};

const static uint32_t crc32_table[256] = {
            0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
            0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
            0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
            0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
            0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
            0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
            0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
            0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
            0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
            0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
            0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
            0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
            0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
            0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
            0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
            0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
            0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
            0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
            0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
            0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
            0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
            0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
            0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
            0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
            0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
            0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
            0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
            0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
            0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
            0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
            0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
            0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

// java.util.zip.CRC32 compatible CRC32 implementation to match the sofa bolt java server  crc check.
class JavaCompatibleCRC32 {
public:
    JavaCompatibleCRC32() : _crc32(0xffffffff) {}

    void update(const uint8_t* input,  size_t size) {
        for (size_t p = 0; p < size; p++) {
            _crc32 = (_crc32 >> 8) ^ crc32_table[(_crc32 ^ input[p]) & 0xff];
        }
    }

    void update(const butil::IOBuf& input) {
        butil::IOBufBytesIterator it(input);
        size_t bytes_left = input.size();
        const void* data = NULL;
        size_t size = 0;
        while (bytes_left > 0 && it.forward_one_block(&data, &size)) {
            size_t digest_bytes = std::min(bytes_left, size);
            const uint8_t* ptr = static_cast<const uint8_t*>(data);
            update(ptr, digest_bytes);
            bytes_left -= digest_bytes;
        }
    }

    uint32_t checksum() {
        return _crc32 ^ 0xffffffff;
    }

private:
    uint32_t _crc32;
};

template<typename UnpackedResponseHeader>
ParseResult ParseSofaBoltMessageImpl(butil::IOBuf* source,  Socket* socket) {
    typedef SofaBoltHeaderTrait<UnpackedResponseHeader> TraitType;
    size_t packed_response_header_size = sizeof(typename TraitType::PackedType);
    if (source->size() < packed_response_header_size) {
        return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
    }

    uint8_t options = 0;
    uint16_t class_len, header_len;
    uint32_t content_len;

    if (TraitType::version == SOFA_BOLT_V2) {
        source->copy_to(&options, sizeof(uint8_t), offsetof(SofaBoltResponseHeaderV2Packed, options));
    }
    source->copy_to(&class_len, sizeof(uint16_t), offsetof(typename TraitType::PackedType, class_len));
    source->copy_to(&header_len, sizeof(uint16_t), offsetof(typename TraitType::PackedType, header_len));
    source->copy_to(&content_len, sizeof(uint32_t), offsetof(typename TraitType::PackedType, content_len));

    class_len = butil::NetToHost16(class_len);
    header_len = butil::NetToHost16(header_len);
    content_len = butil::NetToHost32(content_len);
    size_t payload_len = packed_response_header_size + class_len + header_len + content_len;
    size_t total_len = payload_len;

    if ((options & SOFA_BOLT_CRC_CHECK) > 0) {
        total_len += sizeof(uint32_t); 
    }

    if (source->size() < total_len) {
        return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
    }

    MostCommonMessage* msg = MostCommonMessage::Get();
    source->cutn(&msg->meta, packed_response_header_size);
    source->cutn(&msg->payload, total_len - packed_response_header_size);
    return MakeMessage(msg);
}

// Parse sofa bolt message.
ParseResult ParseSofaBoltMessage(butil::IOBuf* source, Socket* socket, bool read_eof, const void *arg) { 
    uint8_t protocol;
    size_t size = sizeof(uint8_t);
    size_t n = source->copy_to(&protocol, size);
    if (n < size) {
        return MakeParseError(PARSE_ERROR_NOT_ENOUGH_DATA);
    }

    switch(protocol) {
        case SOFA_BOLT_V1:
            return ParseSofaBoltMessageImpl<SofaBoltResponseHeaderV1>(source, socket);
        case SOFA_BOLT_V2:
            return ParseSofaBoltMessageImpl<SofaBoltResponseHeaderV2>(source, socket);
        default:
            return MakeParseError(PARSE_ERROR_ABSOLUTELY_WRONG);
    }
}

#define EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, field, offset) \
        source->copy_to(&(header->field), sizeof(header->field), offset)

template<typename UnpackedResponseHeader>
void ExtractSofaHeader(const butil::IOBuf* source, UnpackedResponseHeader* header);

template<>
void ExtractSofaHeader<SofaBoltResponseHeaderV1>(const butil::IOBuf* source, SofaBoltResponseHeaderV1* header) {
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, proto, offsetof(SofaBoltResponseHeaderV1Packed, proto)); 
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, type, offsetof(SofaBoltResponseHeaderV1Packed, type)); 
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, cmd_code, offsetof(SofaBoltResponseHeaderV1Packed, cmd_code));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, ver2, offsetof(SofaBoltResponseHeaderV1Packed, ver2));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, request_id, offsetof(SofaBoltResponseHeaderV1Packed, request_id));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, codec, offsetof(SofaBoltResponseHeaderV1Packed, codec)); 
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, resp_status, offsetof(SofaBoltResponseHeaderV1Packed, resp_status));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, class_len, offsetof(SofaBoltResponseHeaderV1Packed, class_len));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, header_len, offsetof(SofaBoltResponseHeaderV1Packed, header_len));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, content_len, offsetof(SofaBoltResponseHeaderV1Packed, content_len));
}

template<>
void ExtractSofaHeader<SofaBoltResponseHeaderV2>(const butil::IOBuf* source, SofaBoltResponseHeaderV2* header) {
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, proto, offsetof(SofaBoltResponseHeaderV2Packed, proto));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, ver1, offsetof(SofaBoltResponseHeaderV2Packed, ver1));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, type, offsetof(SofaBoltResponseHeaderV2Packed, type)); 
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, cmd_code, offsetof(SofaBoltResponseHeaderV2Packed, cmd_code));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, ver2, offsetof(SofaBoltResponseHeaderV2Packed, ver2));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, request_id, offsetof(SofaBoltResponseHeaderV2Packed, request_id));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, codec, offsetof(SofaBoltResponseHeaderV2Packed, codec));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, options, offsetof(SofaBoltResponseHeaderV2Packed, options));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, resp_status, offsetof(SofaBoltResponseHeaderV2Packed, resp_status));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, class_len, offsetof(SofaBoltResponseHeaderV2Packed, class_len));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, header_len, offsetof(SofaBoltResponseHeaderV2Packed, header_len));
    EXTRACT_SOFA_BOLT_HEADER_FIELD(source, header, content_len, offsetof(SofaBoltResponseHeaderV2Packed, content_len));
}

#undef EXTRACT_SOFA_BOLT_HEADER_FIELD

template<typename SofaBoltHeaderAccessor>
bool CheckSofaBoltResponseHeader(const SofaBoltHeaderAccessor& accessor, Controller* cntl) {
    // We don't check header.proto because we've done the check before calling CheckSofaBoltHeader.
    if (!accessor.CheckVer1IfApplicable()) {
        cntl->SetFailed(ERESPONSE, "Response header proto %d not match ver1 %d",
            accessor.GetProtocol(),  static_cast<int>(accessor.GetVer1IfApplicable()));
        return false;
    }

    if (!accessor.CheckHeaderType(SOFA_BOLT_RESPONSE)) {
        cntl->SetFailed(ERESPONSE, "Response header type %d not supported",
            static_cast<int>(accessor.GetHeaderType()));
        return false;
    }

    if (!accessor.CheckCmdCode(SOFA_BOLT_CMD_RESPONSE)) {
        cntl->SetFailed(ERESPONSE, "Response header cmd code %d not supported",
            static_cast<int>(accessor.GetCmdCode()));
        return false;
    }

    if (!accessor.CheckResponseStatus(SOFA_BOLT_RESPONSE_STATUS_SUCCESS)) {
        uint32_t status = static_cast<uint32_t>(accessor.GetResponseStatus());
        auto it = g_sofa_bolt_status_message.find(status);
        cntl->SetFailed(ERESPONSE, "Response failed, server returned status %d, message %s",
             status, (it == g_sofa_bolt_status_message.end() ? "unknown status" : it->second));
        return false;
    }

    // Ignore ver2 check
    if (!accessor.CheckCodec(SOFA_BOLT_PROTOBUF)) {
        cntl->SetFailed(ERESPONSE, "Response codec %d not supported", static_cast<int>(accessor.GetCodec()));
        return false;
    }

    return true;
}

bool CheckSofaBoltCrc32(
    const butil::IOBuf& meta, const butil::IOBuf& payload, uint32_t checksum, Controller* cntl) {
    JavaCompatibleCRC32 crc32;
    crc32.update(meta);
    crc32.update(payload);
    if (checksum != crc32.checksum()) {
        cntl->SetFailed(ERESPONSE, "Checksum exam failed, expected %8x, %8x calculated",
                        checksum, crc32.checksum());
        return false;
    }
    return true;
}

template<typename UnpackedResponseHeader>
void ProcessSofaBoltResponseImpl(MostCommonMessage* msg, Controller* cntl) {
    UnpackedResponseHeader header;
    ExtractSofaHeader(&msg->meta, &header);

    SofaBoltHeaderReadAccessor<UnpackedResponseHeader> accessor(&header, true /* need_network_to_host_reorder */);
    if (!CheckSofaBoltResponseHeader(accessor, cntl)) {
        return;
    }

    if (accessor.HasCrcCheckOption()) {
        size_t payload_size = accessor.GetClassLen() + accessor.GetHeaderLen() + accessor.GetContentLen();
        uint32_t checksum;
        size_t checksum_size = sizeof(uint32_t);
        msg->payload.copy_to(&checksum, checksum_size, payload_size);
        // Pop back the 4 bytes crc32 checksum
        msg->payload.pop_back(checksum_size);
        checksum = butil::NetToHost32(checksum);
        if (!CheckSofaBoltCrc32(msg->meta, msg->payload, checksum, cntl)) {
            return;
        }
    }

    // Usually we have rpc context set by user code.
    if (!cntl->HasRpcContext()) {
        SofaBoltContext* context = new SofaBoltContext();
        cntl->SetRpcContext(context);
    } 

    SofaBoltContextPrivateAccessor context_accessor(
            static_cast<SofaBoltContext*>(ControllerPrivateAccessor(cntl).get_mutable_rpc_context()));

    if (accessor.GetClassLen() > 0) {
        std::string class_name;
        msg->payload.cutn(&class_name, accessor.GetClassLen());
        context_accessor.SetResponseClassName(std::move(class_name));
    }

#define IOBUF_CUT_WITH_SIZE_CHECK(buf, target, size, cntl)                                                   \
do {                                                                                                    \
    if (size != (buf).cutn(&target, size)) {                                                            \
        cntl->SetFailed(ERESPONSE, "Fail to parse sofa bolt header map, not enought response size");    \
        break;                                                                                          \
    }                                                                                                   \
} while (0)

    if (accessor.GetHeaderLen() > 0) {
        ssize_t header_size_remain = accessor.GetHeaderLen();   
        size_t size_bytes = sizeof(uint32_t);
        std::string key, value;
        while (header_size_remain > 0) {
            uint32_t key_size, value_size;
            IOBUF_CUT_WITH_SIZE_CHECK(msg->payload, key_size, size_bytes, cntl);
            key_size = butil::NetToHost32(key_size);
            IOBUF_CUT_WITH_SIZE_CHECK(msg->payload, key, key_size, cntl);
            IOBUF_CUT_WITH_SIZE_CHECK(msg->payload, value_size, size_bytes, cntl);
            value_size = butil::NetToHost32(value_size);
            IOBUF_CUT_WITH_SIZE_CHECK(msg->payload, value, value_size, cntl);
            context_accessor.AddResponseHeader(key, value);
            header_size_remain -= size_bytes + key_size + size_bytes + value_size;
            key.clear();
            value.clear();
        }
    }
#undef IOBUF_CUT_WITH_CHECK
    if (cntl->Failed() || !cntl->response() || accessor.GetContentLen() == 0) {
        return;
    }
    if (!ParsePbFromIOBuf(cntl->response(), msg->payload)) {
        cntl->SetFailed(ERESPONSE, "Fail to parse response message, response_size = %zd",
                        msg->payload.size());  
    }
}

// Actions to sofa bolt response
void ProcessSofaBoltResponse(InputMessageBase* msg_base) {
    DestroyingPtr<MostCommonMessage> msg(static_cast<MostCommonMessage*>(msg_base));

    Socket* socket = msg->socket();
    // Fetch correlation id that we saved before in `PackSofaBoltRequest'
    const bthread_id_t cid = { static_cast<uint64_t>(socket->correlation_id()) };
    Controller* cntl = NULL;
    const int rc = bthread_id_lock(cid, (void**)&cntl);
    if (rc != 0) {
        LOG_IF(ERROR, rc != EINVAL && rc != EPERM)
            << "Fail to lock correlation_id=" << cid << ": " << berror(rc);
        return;
    }

    const int saved_error = cntl->ErrorCode();
    ControllerPrivateAccessor accessor(cntl);
    // We need protocol to determine the header size.
    uint8_t protocol;
    msg->meta.copy_to(&protocol, sizeof(uint8_t));

    switch(protocol) {
        case SOFA_BOLT_V1:
            ProcessSofaBoltResponseImpl<SofaBoltResponseHeaderV1>(msg.get(), cntl);
            break;
        default:
            ProcessSofaBoltResponseImpl<SofaBoltResponseHeaderV2>(msg.get(), cntl);
            break;
    }

    msg.reset();
    accessor.OnResponse(cid, saved_error);
}

bool SofaBoltCheckContext(Controller* controller) {
    const SofaBoltContext* context = static_cast<const SofaBoltContext*>(controller->GetRpcContext());
    if (!context) {
        controller->SetFailed(EREQUEST, "Sofa bolt request context not set");
        return false;
    }
    SofaBoltProtocolVersion protocol = context->GetRequestProtocolVersion();
    if (protocol != SOFA_BOLT_V1 && protocol != SOFA_BOLT_V2) {
        controller->SetFailed(EREQUEST, "Unsupported sofa bolt protocol version %d", static_cast<int>(protocol));
        return false;
    }
    // If user code set crc check to v1, just ignore it instead of setting request failed.
    if (protocol == SOFA_BOLT_V1 && context->RequestCrc32CheckEnabled()) {
        LOG(WARNING) << "Sofa bolt v1 does not support crc check, option ignored";
    }
    return true;
}

// Serialize a sofa bolt request.
void SerializeSofaBoltRequest(butil::IOBuf* buf,
                              Controller* cntl,
                              const google::protobuf::Message* request) {
    CompressType type = cntl->request_compress_type(); 
    if (type != COMPRESS_TYPE_NONE) {
        cntl->SetFailed(EREQUEST, "Sofa bolt does not support data compression"); 
        return;
    }
    if (!SofaBoltCheckContext(cntl)) {
        return;
    }
    return SerializeRequestDefault(buf, cntl, request);
}

static size_t IOBufCopyKV(butil::IOBufAppender& appender,
    const char* key, size_t key_size, const char* value, size_t value_size) {
    uint32_t key_size_network = butil::HostToNet32(key_size);
    uint32_t value_size_network = butil::HostToNet32(value_size);

    appender.append(&key_size_network, 4);
    appender.append(key, key_size);
    appender.append(&value_size_network, 4);
    appender.append(value, value_size);
    return 4 /* key_size */ + key_size /* key */ + 4 /* value_size */ + value_size /* value */;
}

static const char g_sofa_bolt_default_class_name[] = "com.alipay.sofa.rpc.core.request.SofaRequest";
static const char g_sofa_bolt_service_key_name[] = "service";
static const char g_sofa_bolt_sofa_service_key_name[] = "sofa_head_target_service";
static const char g_sofa_bolt_sofa_head_method_key_name[] = "sofa_head_method_name";
static const char g_sofa_bolt_rpc_trace_id_key[] = "rpc_trace_context.sofaTraceId";

bool ShouldUseCustomServiceId(
        const SofaBoltContext& context, const google::protobuf::MethodDescriptor* method, std::string* service_id) {
    if (context.GetRequestServiceName().size() > 0) {
        return false;
    }

    const google::protobuf::ServiceOptions& options = method->service()->options();
    if (!options.HasExtension(brpc::custom_service_id)) {
        return false;
    }

    std::string id = options.GetExtension(brpc::custom_service_id);
    if (id.size() > 0) {
        *service_id = std::move(id);
        return true;
    }

    return false;
}

template<typename SofaBoltHeader>
void PackSofaBoltRequestImpl(butil::IOBuf* iobuf_out,
                SocketMessage**, /* not used */
                uint64_t correlation_id,
                const google::protobuf::MethodDescriptor* method,
                Controller* cntl,
                const butil::IOBuf& request,
                const Authenticator* /* not used */) {
    SofaBoltHeader header;
    SofaBoltHeaderWriteAccessor<SofaBoltHeader> accessor(&header);

    // SerializeSofaBoltRequest have already checked the context not NULL, we don't check it here again.
    const SofaBoltContext& context = *static_cast<const SofaBoltContext*>(cntl->GetRpcContext());
    SofaBoltProtocolVersion protocol = context.GetRequestProtocolVersion();
    accessor.SetProtocol(protocol);
    accessor.SetVer1IfApplicable(protocol);

    accessor.SetHeaderType(SOFA_BOLT_REQUEST);
    accessor.SetCmdCode(SOFA_BOLT_CMD_REQUEST);
    // We can't use cntl->log_id() as sofa bolt request_id because it's 64 bit, we just randomly generate one for sofa bolt.
    accessor.SetRequestId(static_cast<uint32_t>(butil::fast_rand_less_than(1ULL << 32)));
    accessor.SetCodec(SOFA_BOLT_PROTOBUF);
    // Options silently ignored for protocol v1
    if (context.RequestCrc32CheckEnabled()) {
        accessor.SetEnableCrcCheckIfApplicable();
    }
    // Take negative timeout or 0 or any value >= UINT32_MAX as MAX_TIME_OUT(UINT32_MAX)
    {
        int64_t timeout = cntl->timeout_ms();
        int64_t uint32_max = static_cast<int64_t>(std::numeric_limits<uint32_t>::max());
        uint32_t timeout_send = uint32_max;
        if (timeout > 0 && timeout < uint32_max) {
            timeout_send = static_cast<uint32_t>(timeout);
        }
        accessor.SetTimeout(timeout_send);
    }

    butil::IOBufAppender payload_appender;
    // ClassName && ClassLen
    {
        const char* class_name = g_sofa_bolt_default_class_name;
        size_t class_len = sizeof(g_sofa_bolt_default_class_name) - 1;
        accessor.SetClassLen(class_len);
        // COPY class_name into payload
        payload_appender.append(class_name, class_len);
    }
    // HeaderLen && Header 
    {
        size_t header_len = 0;
        size_t key_size = sizeof(g_sofa_bolt_service_key_name) - 1;
        size_t value_size;
        std::string service_identifier;

        // Sofa bolt service identifier: ${service_name}:${service_version}:[${service_unique_id}], 
        // where service_unique_id may be abstent.
        // The service identifier choosing rule:
        // 1. First to check if there's service name set in context, if set, use service name from context.
        // 2. Check if there is service identifier set in the service description proto (see brpc/options.proto) 
        //      to see the `custom_service_id` option
        // 3. Use method->setvice()->full_name() as service name
        if (!ShouldUseCustomServiceId(context, method, &service_identifier)) {
            if (context.GetRequestServiceName().size() > 0) {
                service_identifier = context.GetRequestServiceName();
            } else {
                service_identifier = method->service()->full_name();
            }
            service_identifier.append(":");
            const char* service_version = context.GetRequestServiceVersion().empty() ? 
                        "1.0" : context.GetRequestServiceVersion().c_str();
            service_identifier.append(service_version);

            if (context.GetRequestServiceUniqueId().size() > 0) {
                service_identifier.append(":");
                service_identifier.append(context.GetRequestServiceUniqueId().data());
            }
        }
        value_size = service_identifier.size();

        // "service = xxxx"
        header_len += IOBufCopyKV(payload_appender, g_sofa_bolt_service_key_name,
                        key_size,  service_identifier.data(), value_size);

        // "sofa_head_target_service = xxxx"
        key_size = sizeof(g_sofa_bolt_sofa_service_key_name) - 1;
        header_len += IOBufCopyKV(payload_appender, g_sofa_bolt_sofa_service_key_name,
                        key_size, service_identifier.data(), value_size);

        // "sofa_head_method_name = xxxx"
        key_size = sizeof(g_sofa_bolt_sofa_head_method_key_name) - 1;
        const std::string& method_name = method->name();
        value_size = method_name.size();
        header_len += IOBufCopyKV(payload_appender, g_sofa_bolt_sofa_head_method_key_name,
                        key_size, method_name.data(), value_size);

        // "rpc_trace_context.sofaTraceId = xxxx"
        // First use request_id as sofa bolt trace, if request_id not set, randomly generate one
        key_size = sizeof(g_sofa_bolt_rpc_trace_id_key) - 1;
        if (!cntl->request_id().empty()) {
            value_size = cntl->request_id().size();
            header_len += IOBufCopyKV(payload_appender, g_sofa_bolt_rpc_trace_id_key,
                            key_size, cntl->request_id().data(), value_size);
        } else {
            std::string request_id = butil::fast_rand_printable(20);
            value_size = request_id.size();
            header_len += IOBufCopyKV(payload_appender, g_sofa_bolt_rpc_trace_id_key,
                            key_size, request_id.data(), value_size);
        }
        accessor.SetHeaderLen(header_len);
    }
    // Content
    accessor.SetContentLen(request.size());

    butil::IOBuf head, payload;
    accessor.HeaderHostOrderToNetwork();
    accessor.PackToIOBuf(&head);

    payload_appender.move_to(payload);
    payload.append(request);

    if (accessor.IsCrc32CheckEnabledIfApplicable()) {
        JavaCompatibleCRC32 crc32;
        crc32.update(head);
        crc32.update(payload);
        uint32_t checksum = crc32.checksum();
        checksum = butil::HostToNet32(checksum);
        payload.append(&checksum, sizeof(uint32_t));
    }
    iobuf_out->append(head);
    iobuf_out->append(payload);
}

// Pack `request' to `method' into `buf'.
void PackSofaBoltRequest(butil::IOBuf* iobuf_out,
                         SocketMessage** user_message_out,
                         uint64_t correlation_id,
                         const google::protobuf::MethodDescriptor* method,
                         Controller* cntl,
                         const butil::IOBuf& request,
                         const Authenticator* auth) {
    ControllerPrivateAccessor accessor(cntl);
    // Store `correlation_id' into Socket since sofa bolt protocol
    // doesn't contain this field
    accessor.get_sending_socket()->set_correlation_id(correlation_id);

    // SerializeSofaBoltRequest have already checked the context not NULL, we don't check it here again.
    const SofaBoltContext* context = static_cast<const SofaBoltContext*>(cntl->GetRpcContext());
    SofaBoltProtocolVersion protocol = context->GetRequestProtocolVersion();
    switch (protocol) {
        case SOFA_BOLT_V1:
            PackSofaBoltRequestImpl<SofaBoltRequestHeaderV1>(
                            iobuf_out, user_message_out, correlation_id, method, cntl, request, auth);
            break;
        default:
            PackSofaBoltRequestImpl<SofaBoltRequestHeaderV2>(
                            iobuf_out, user_message_out, correlation_id, method, cntl, request, auth);
            break;
    }
}

}
}
