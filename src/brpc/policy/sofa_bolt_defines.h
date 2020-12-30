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


#ifndef BRPC_SOFA_BOLT_DEFINES_H
#define BRPC_SOFA_BOLT_DEFINES_H

#include <stdint.h>
#include <unordered_map>

namespace brpc {
namespace policy {

/**
 * Request command protocol for v1
 * 0     1     2           4           6           8          10           12          14         16
 * +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
 * |proto| type| cmdcode   |ver2 |   requestId           |codec|        timeout        |  classLen |
 * +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
 * |headerLen  | contentLen            |                             ... ...                       |
 * +-----------+-----------+-----------+                                                           +
 * |               className + header  + content  bytes                                            |
 * +                                                                                               +
 * |                               ... ...                                                         |
 * +-----------------------------------------------------------------------------------------------+
 * 
 * proto: code for protocol
 * type: request/response/request oneway
 * cmdcode: code for remoting command
 * ver2:version for remoting command
 * requestId: id of request
 * codec: code for codec
 * headerLen: length of header
 * contentLen: length of content
 * 
 * Response command protocol for v1
 * 0     1     2     3     4           6           8          10           12          14         16
 * +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
 * |proto| type| cmdcode   |ver2 |   requestId           |codec|respstatus |  classLen |headerLen  |
 * +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
 * | contentLen            |                  ... ...                                              |
 * +-----------------------+                                                                       +
 * |                         className + header  + content  bytes                                  |
 * +                                                                                               +
 * |                               ... ...                                                         |
 * +-----------------------------------------------------------------------------------------------+
 * respstatus: response status
 * 
 */

/**
 * Request command protocol for v2
 * 0     1     2           4           6           8          10     11     12          14         16
 * +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+------+-----+-----+-----+-----+
 * |proto| ver1|type | cmdcode   |ver2 |   requestId           |codec|switch|   timeout             |
 * +-----------+-----------+-----------+-----------+-----------+------------+-----------+-----------+
 * |classLen   |headerLen  |contentLen             |           ...                                  |
 * +-----------+-----------+-----------+-----------+                                                +
 * |               className + header  + content  bytes                                             |
 * +                                                                                                +
 * |                               ... ...                                  | CRC32(optional)       |
 * +------------------------------------------------------------------------------------------------+
 * 
 * proto: code for protocol
 * ver1: version for protocol
 * type: request/response/request oneway
 * cmdcode: code for remoting command
 * ver2:version for remoting command
 * requestId: id of request
 * codec: code for codec
 * switch: function switch for protocol
 * headerLen: length of header
 * contentLen: length of content
 * CRC32: CRC32 of the frame(Exists when ver1 > 1)
 *
 * Response command protocol for v2
 * 0     1     2     3     4           6           8          10     11    12          14          16
 * +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+------+-----+-----+-----+-----+
 * |proto| ver1| type| cmdcode   |ver2 |   requestId           |codec|switch|respstatus |  classLen |
 * +-----------+-----------+-----------+-----------+-----------+------------+-----------+-----------+
 * |headerLen  | contentLen            |                      ...                                   |
 * +-----------------------------------+                                                            +
 * |               className + header  + content  bytes                                             |
 * +                                                                                                +
 * |                               ... ...                                  | CRC32(optional)       |
 * +------------------------------------------------------------------------------------------------+
 * respstatus: response status
 * 
 */

enum SofaBoltProtocolVersion {
    SOFA_BOLT_V1 = 1,
    SOFA_BOLT_V2 = 2,
};

enum SofaBoltCodecType {
    SOFA_BOLT_HESSIAN2 = 1,
    SOFA_BOLT_PROTOBUF = 11,
};

enum SofaBoltCommandCodeType {
    SOFA_BOLT_CMD_HEARTBEAT = 0,
    SOFA_BOLT_CMD_REQUEST = 1,
    SOFA_BOLT_CMD_RESPONSE = 2,
};

enum SofaBoltHeaderType {
    SOFA_BOLT_RESPONSE = 0,
    SOFA_BOLT_REQUEST = 1,
    SOFA_BOLT_ONEWAY = 2,
};

enum SofaBoltProtocolOptions {
    SOFA_BOLT_CRC_CHECK = 0x1,
};

// Don't ask me why the reponse status value is not continous integer(from hex 0x09 to 0x10),
// I guess the sofa bolt author mistakenly mixed up decimal integer and hex integer since the
// protocol was developed. See the origional sofa bolt java implementation:
// https://github.com/sofastack/sofa-bolt/blob/master/src/main/java/com/alipay/remoting/ResponseStatus.java#L45
enum SofaBoltResponseStatus {
    #define SOFA_BOLT_STATUS(name, value)       SOFA_BOLT_RESPONSE_STATUS_##name = value
    SOFA_BOLT_STATUS(SUCCESS, 0x00), // Ok
    SOFA_BOLT_STATUS(ERROR, 0x01), // Error caught
    SOFA_BOLT_STATUS(SERVER_EXCEPTION, 0x02), // Exception caught
    SOFA_BOLT_STATUS(UNKNOWN, 0x03), // Unknown...
    SOFA_BOLT_STATUS(SERVER_THREADPOOL_BUSY, 0x04), // Process thread pool busy
    SOFA_BOLT_STATUS(ERROR_COMM, 0x05), // Error of communication
    SOFA_BOLT_STATUS(NO_PROCESSOR, 0x06), // No processor find
    SOFA_BOLT_STATUS(TIMEOUT, 0x07), // Timeout
    SOFA_BOLT_STATUS(CLIENT_SEND_ERROR, 0x08), // Send failed
    SOFA_BOLT_STATUS(CODEC_EXCEPTION, 0x09), // Exception in encode or decode
    SOFA_BOLT_STATUS(CONNECTION_CLOSED, 0x10), // Connection closed.
    SOFA_BOLT_STATUS(SERVER_SERIAL_EXCEPTION, 0x11), // server serialize exception
    SOFA_BOLT_STATUS(SERVER_DESERIAL_EXCEPTION, 0x12),// server deserialize exception
    #undef SOFA_BOLT_STATUS
};

struct SofaBoltRequestHeaderV1 {
    uint8_t proto;
    uint8_t type;
    uint16_t cmd_code;
    uint8_t ver2;
    uint32_t request_id;
    uint8_t codec;
    uint32_t timeout;
    uint16_t class_len;
    uint16_t header_len;
    uint32_t content_len;
};

struct SofaBoltRequestHeaderV2 {
    uint8_t proto;
    uint8_t ver1;
    uint8_t type;
    uint16_t cmd_code;
    uint8_t ver2;
    uint32_t request_id;
    uint8_t codec;
    uint8_t options;
    uint32_t timeout;
    uint16_t class_len;
    uint16_t header_len;
    uint32_t content_len;
};

// For sofa bolt response (V1 and V2), we have packed and unpacked version of the header.
// The packed version is used to probe the header field's offset from the begging of the response 
// packet and the unpacked version is used for reading to avoid unaligned memory access.
struct SofaBoltResponseHeaderV1Packed {
    uint8_t proto;
    uint8_t type;
    uint16_t cmd_code;
    uint8_t ver2;
    uint32_t request_id;
    uint8_t codec;
    uint16_t resp_status;
    uint16_t class_len;
    uint16_t header_len;
    uint32_t content_len;
} __attribute__((__packed__));

struct SofaBoltResponseHeaderV1 {
    uint8_t proto;
    uint8_t type;
    uint16_t cmd_code;
    uint8_t ver2;
    uint32_t request_id;
    uint8_t codec;
    uint16_t resp_status;
    uint16_t class_len;
    uint16_t header_len;
    uint32_t content_len;
};

struct SofaBoltResponseHeaderV2Packed {
    uint8_t proto;
    uint8_t ver1;
    uint8_t type;
    uint16_t cmd_code;
    uint8_t ver2;
    uint32_t request_id;
    uint8_t codec;
    uint8_t options;
    uint16_t resp_status;
    uint16_t class_len;
    uint16_t header_len;
    uint32_t content_len;
} __attribute__((__packed__));

struct SofaBoltResponseHeaderV2 {
    uint8_t proto;
    uint8_t ver1;
    uint8_t type;
    uint16_t cmd_code;
    uint8_t ver2;
    uint32_t request_id;
    uint8_t codec;
    uint8_t options;
    uint16_t resp_status;
    uint16_t class_len;
    uint16_t header_len;
    uint32_t content_len;
};

template<typename SofaBoltHeader>
struct SofaBoltHeaderTrait;

template<>
struct SofaBoltHeaderTrait<SofaBoltRequestHeaderV1> {
    const static SofaBoltHeaderType type = SOFA_BOLT_REQUEST;
    const static SofaBoltProtocolVersion version = SOFA_BOLT_V1;
};

template<>
struct SofaBoltHeaderTrait<SofaBoltRequestHeaderV2> {
    const static SofaBoltHeaderType type = SOFA_BOLT_REQUEST;
    const static SofaBoltProtocolVersion version = SOFA_BOLT_V2;
};

template<>
struct SofaBoltHeaderTrait<SofaBoltResponseHeaderV1> {
    const static SofaBoltHeaderType type = SOFA_BOLT_RESPONSE;
    const static SofaBoltProtocolVersion version = SOFA_BOLT_V1;
    typedef SofaBoltResponseHeaderV1Packed PackedType;
};

template<>
struct SofaBoltHeaderTrait<SofaBoltResponseHeaderV2> {
    const static SofaBoltHeaderType type = SOFA_BOLT_RESPONSE;
    const static SofaBoltProtocolVersion version = SOFA_BOLT_V2;
    typedef SofaBoltResponseHeaderV2Packed PackedType;
};

}
}
#endif
