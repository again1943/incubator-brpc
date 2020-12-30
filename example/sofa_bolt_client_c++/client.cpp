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

// A client sending requests to server every 1 second.

#include <sstream>
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>
#include <brpc/options.pb.h>
#include <brpc/policy/sofa_bolt_context.h>
#include "brpc/kvmap.h"
#include "echo_service.pb.h"

DEFINE_int32(sofa_bolt_version, 1, "Sofa bolt version, 1(v1) or 2(v2)");
DEFINE_bool(enable_crc_check, false, "Enable crc check. Only valid for v2");
DEFINE_string(service_name, "", "Sofa bolt service name");
DEFINE_string(service_version, "1.0", "Sofa bolt service version");
DEFINE_string(server, "127.0.0.1:12200", "IP Address of server");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_int32(timeout_ms, 1000, "RPC timeout in milliseconds");
DEFINE_int32(interval_ms, 10, "Milliseconds between consecutive requests");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");


int main(int argc, char* argv[]) {
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_SOFA_BOLT;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms;
    if (channel.Init(FLAGS_server.data(), FLAGS_load_balancer.data(), &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    int log_id = 0;
    while (!brpc::IsAskedToQuit()) {
#define ECHO_NS com::alipay::sofa::rpc::protobuf
        ECHO_NS::EchoRequest request;
        ECHO_NS::EchoResponse response;
        ECHO_NS::ProtoService_Stub stub(&channel);
        std::stringstream stream;
        stream << "xyz:" << log_id;
        request.set_name(stream.str());
        request.set_group(ECHO_NS::A);
#undef ECHO_NS
        brpc::Controller cntl;
        {
            brpc::policy::SofaBoltContext* context = new brpc::policy::SofaBoltContext();
            context->SetRequestProtocolVersion(static_cast<brpc::policy::SofaBoltProtocolVersion>(FLAGS_sofa_bolt_version));
            if (FLAGS_enable_crc_check) {
                context->RequestEnableCrc32Check();
            }
            if (FLAGS_service_name.size() > 0) {
                context->SetRequestServiceName(FLAGS_service_name);
            }
            if (FLAGS_service_version.size() > 0) {
                context->SetRequestServiceVersion(FLAGS_service_version);
            }
            cntl.SetRpcContext(context);
        }
        cntl.set_log_id(log_id++);
        stub.echoObj(&cntl, &request, &response, NULL);

       if (!cntl.Failed()) {
            const brpc::policy::SofaBoltContext* context = static_cast<const brpc::policy::SofaBoltContext*>(cntl.GetRpcContext());
            if (context->HasResponseHeaderMap()) {
                brpc::KVMap headers = context->GetResponseHeaderMap();
                for (brpc::KVMap::Iterator it = headers.Begin(); it != headers.End(); ++it) {
                    LOG(INFO) << "header_key:\n" << it->first << ":" << it->second;
                }
            }

            LOG(INFO) << "Received response from " << cntl.remote_side()
                << " to " << cntl.local_side()
                << ", ClassName=" << context->GetResponseClassName()
                << ", code=" << response.code() << ", message=" << response.message() 
                << " latency=" << cntl.latency_us() << "us";
        } else {
            LOG(WARNING) << cntl.ErrorText();
        }
        usleep(FLAGS_interval_ms * 1000L);
    }
    LOG(INFO) << "EchoClient is going to quit";
    return 0;
}
