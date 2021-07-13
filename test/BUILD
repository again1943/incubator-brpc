# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

load("//:bazel/brpc.bzl", "brpc_proto_library")

config_setting(
    name = "darwin",
    values = {"cpu": "darwin"},
    visibility = ["//visibility:public"],
)

COPTS = [
    "-D__STDC_FORMAT_MACROS",
    "-DBTHREAD_USE_FAST_PTHREAD_MUTEX",
    "-D__const__=",
    "-D_GNU_SOURCE",
    "-DUSE_SYMBOLIZE",
    "-DNO_TCMALLOC",
    "-D__STDC_LIMIT_MACROS",
    "-D__STDC_CONSTANT_MACROS",
    "-fPIC",
    "-Wno-unused-parameter",
    "-fno-omit-frame-pointer",
    "-DGFLAGS_NS=google",
    "-DBAZEL_TEST=1",
    "-DBVAR_NOT_LINK_DEFAULT_VARIABLES",
    "-DUNIT_TEST",
] + select({
    "//:with_glog": ["-DBRPC_WITH_GLOG=1"],
    "//conditions:default": ["-DBRPC_WITH_GLOG=0"],
}) + select({
    "@bazel_tools//src/conditions:linux_aarch64"  : [
      # aarch64 char is unsigned by default, to be consistent with x86_64
      # manually specify it as signed char
      "-fsigned-char",
      # gcse optimization makes brpc crash unser aarch64,
      # See https://github.com/apache/incubator-brpc/issues/845
      "-fno-gcse",
    ],
    "//conditions:default": [
      "-msse4",
      "-msse4.2",
    ],
}) + select({
    # Clang is so amusing about "-faccess-control" flag.
    # 1.  Before clang12, someone mistakenly changed it as "no-access-control"
    #     without any test, thus if you really want to disable C++ access control,
    #     you need to type "-fno-no-access-control". This was fixed in clang 12
    #     under linux. See: https://reviews.llvm.org/D93104
    #
    # 2.  The clang compiler on MacOS is even more funny, the newest version of apple
    #     clang, "Apple clang version 12.0.5 (clang-1205.0.22.11)", when you type
    #     "clang --help |grep \"access-control\"", you get:
    #          -fno-access-control
    #          -fno-no-access-control  Disable C++ access control
    #
    #     Ok, it seems Apple does not fix the problem in clang12. We exptect the
    #     "-fno-no-access-control" flag would at least work consisten with the
    #     manaul.
    #
    #     But when you specify "-fno-no-access-control", you get:
    #         "error: unknown argument: '-fno-access-control'"
    #
    #     And when you specify "-fno-access-control", Apple clang take it as you're
    #     enforcing C++ access control. So you'll get all the access control violation
    #     error in the unittest code.
    #
    #     Finally we give up all the attempts and rollback to use macro on MacOS to
    #     make private/protected attributes accessible in our unittest.
    ":darwin"  : [
      "-Dprivate=public",
      "-Dprotected=public",
    ],
    "//conditions:default": [
      # Use -fno-access-control instead of macro for gcc
      "-fno-access-control",
    ],
})

LINKOPTS = [
    "-lpthread",
    "-lssl",
    "-lcrypto",
    "-ldl",
    "-lz",
] + select({
    ":darwin": [],
    "//conditions:default": [
      "-lrt",

    ],
})

TEST_BUTIL_SOURCES = [
    "at_exit_unittest.cc",
    "atomicops_unittest.cc",
    "base64_unittest.cc",
    "big_endian_unittest.cc",
    "bits_unittest.cc",
    "hash_tables_unittest.cc",
    "linked_list_unittest.cc",
    "mru_cache_unittest.cc",
    "small_map_unittest.cc",
    "stack_container_unittest.cc",
    "cpu_unittest.cc",
    "crash_logging_unittest.cc",
    "leak_tracker_unittest.cc",
    "stack_trace_unittest.cc",
    "environment_unittest.cc",
    "file_util_unittest.cc",
    "dir_reader_posix_unittest.cc",
    "file_path_unittest.cc",
    "file_unittest.cc",
    "scoped_temp_dir_unittest.cc",
    "guid_unittest.cc",
    "hash_unittest.cc",
    "lazy_instance_unittest.cc",
    "aligned_memory_unittest.cc",
    "linked_ptr_unittest.cc",
    "ref_counted_memory_unittest.cc",
    "ref_counted_unittest.cc",
    "scoped_ptr_unittest.cc",
    "scoped_vector_unittest.cc",
    "singleton_unittest.cc",
    "weak_ptr_unittest.cc",
    "observer_list_unittest.cc",
    "file_descriptor_shuffle_unittest.cc",
    "rand_util_unittest.cc",
    "safe_numerics_unittest.cc",
    "scoped_clear_errno_unittest.cc",
    "scoped_generic_unittest.cc",
    "security_unittest.cc",
    "sha1_unittest.cc",
    "stl_util_unittest.cc",
    "nullable_string16_unittest.cc",
    "safe_sprintf_unittest.cc",
    "string16_unittest.cc",
    "stringprintf_unittest.cc",
    "string_number_conversions_unittest.cc",
    "string_piece_unittest.cc",
    "string_split_unittest.cc",
    "string_tokenizer_unittest.cc",
    "string_util_unittest.cc",
    "stringize_macros_unittest.cc",
    "sys_string_conversions_unittest.cc",
    "utf_offset_string_conversions_unittest.cc",
    "utf_string_conversions_unittest.cc",
    "cancellation_flag_unittest.cc",
    "condition_variable_unittest.cc",
    "lock_unittest.cc",
    "waitable_event_unittest.cc",
    "type_traits_unittest.cc",
    "non_thread_safe_unittest.cc",
    "platform_thread_unittest.cc",
    "simple_thread_unittest.cc",
    "thread_checker_unittest.cc",
    "thread_collision_warner_unittest.cc",
    "thread_id_name_manager_unittest.cc",
    "thread_local_storage_unittest.cc",
    "thread_local_unittest.cc",
    "watchdog_unittest.cc",
    "time_unittest.cc",
    "version_unittest.cc",
    "logging_unittest.cc",
    "cacheline_unittest.cpp",
    "class_name_unittest.cpp",
    "endpoint_unittest.cpp",
    "unique_ptr_unittest.cpp",
    "errno_unittest.cpp",
    "fd_guard_unittest.cpp",
    "file_watcher_unittest.cpp",
    "find_cstr_unittest.cpp",
    "scoped_lock_unittest.cpp",
    "status_unittest.cpp",
    "string_printf_unittest.cpp",
    "string_splitter_unittest.cpp",
    "synchronous_event_unittest.cpp",
    "temp_file_unittest.cpp",
    "baidu_thread_local_unittest.cpp",
    "baidu_time_unittest.cpp",
    "flat_map_unittest.cpp",
    "crc32c_unittest.cc",
    "iobuf_unittest.cpp",
    "object_pool_unittest.cpp",
    "test_switches.cc",
    "scoped_locale.cc",
    "recordio_unittest.cpp",
    #"popen_unittest.cpp",
    "bounded_queue_unittest.cc",
    "butil_unittest_main.cpp",
] + select({
        "@bazel_tools//tools/osx:darwin": [],
        "//conditions:default": [
           "test_file_util_linux.cc",
           "proc_maps_linux_unittest.cc",
        ],
})

proto_library(
    name = "test_proto",
    srcs = glob([
        "*.proto",
      ],
    ),
    strip_import_prefix = "/test",
    deps = [
      "//:brpc_idl_options_proto",
    ],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "cc_test_proto",
    deps = [
        ":test_proto",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "butil_test",
    srcs = TEST_BUTIL_SOURCES + [
        "scoped_locale.h",
        "multiprocess_func_list.h",
        "test_switches.h",
    ],
    deps = [
        ":cc_test_proto",
        "//:brpc",
        "@com_google_googletest//:gtest",
        "@com_github_gflags_gflags//:gflags",
    ],
    copts = COPTS,
)

BVAR_UNIT_CASES = {
  # "${case_name}" => has_self_defined_main_function
  "bvar_agent_group_unittest" : False,
  "bvar_file_dumper_unittest" : False,
  #"bvar_lock_timer_unittest" : False,
  "bvar_percentile_unittest"  : False,
  #"bvar_recorder_unittest"   : False,
  "bvar_reducer_unittest"     : False,
  "bvar_sampler_unittest"     : False,
  "bvar_status_unittest"      : False,
  "bvar_variable_unittest"    : True,
}

[cc_test(
    name = case[0],
    srcs = [
        case[0] + ".cpp"
    ],
    deps = [
        "//:brpc",
        "@com_google_googletest//:gtest_main" if case[1] else "@com_google_googletest//:gtest",
    ],
    copts = COPTS,
) for case in BVAR_UNIT_CASES.items()]

BTHREAD_UNIT_CASES = [
  "bthread_butex_unittest",
  "bthread_cond_unittest",
  "bthread_countdown_event_unittest",
  "bthread_dispatcher_unittest",
  "bthread_execution_queue_unittest",
  "bthread_fd_unittest",
  "bthread_futex_unittest",
  "bthread_id_unittest",
  "bthread_key_unittest",
  "bthread_list_unittest",
  "bthread_mutex_unittest",
  "bthread_ping_pong_unittest",
  "bthread_rwlock_unittest",
  "bthread_sched_yield_unittest",
  "bthread_setconcurrency_unittest",
  "bthread_timer_thread_unittest",
  "bthread_unittest",
  "bthread_work_stealing_queue_unittest",
]

[cc_test(
    name = case,
    srcs = [
     	case + ".cpp",
    ],
    deps = [
        "//:bthread",
        "//gperftools:gperftools",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_github_gflags_gflags//:gflags",
    ],
    copts = COPTS,
) for case in BTHREAD_UNIT_CASES]

cc_library(
  name = "test_util",
  srcs = [
    "brpc_test_util.cpp",
  ],
  hdrs = [
    "brpc_test_util.h",
  ],
)

# CaseName => has_self_defined_main_function
BRPC_UNIT_CASES = {
    "brpc_channel_unittest"               : True,
    "brpc_circuit_breaker_unittest"       : True,
    "brpc_esp_protocol_unittest"          : True,
    "brpc_grpc_protocol_unittest"         : True,
    "brpc_h2_unsent_message_unittest"     : True,
    "brpc_http_rpc_protocol_unittest"     : True,
    "brpc_hulu_pbrpc_protocol_unittest"   : True,
    "brpc_input_messenger_unittest"       : True,
    "brpc_memcache_unittest"              : True,
    "brpc_mongo_protocol_unittest"        : True,
    "brpc_prometheus_metrics_unittest"    : True,
    "brpc_public_pbrpc_protocol_unittest" : True,
    "brpc_redis_unittest"                 : True,
    "brpc_rtmp_unittest"                  : True,
    "brpc_server_unittest"                : True,
    "brpc_socket_map_unittest"            : True,
    "brpc_socket_unittest"                : True,
    "brpc_sofa_pbrpc_protocol_unittest"   : True,
    "brpc_adaptive_class_unittest"        : False,
    "brpc_builtin_service_unittest"       : False,
    "brpc_controller_unittest"            : False,
    "brpc_event_dispatcher_unittest"      : False,
    "brpc_extension_unittest"             : False,
    "brpc_hpack_unittest"                 : False,
    "brpc_http_message_unittest"          : False,
    "brpc_http_parser_unittest"           : False,
    "brpc_http_status_code_unittest"      : False,
    "brpc_load_balancer_unittest"         : False,
    "brpc_naming_service_filter_unittest" : False,
    "brpc_naming_service_unittest"        : False,
    "brpc_nova_pbrpc_protocol_unittest"   : False,
    "brpc_proto_unittest"                 : False,
    "brpc_protobuf_json_unittest"         : False,
    "brpc_repeated_field_unittest"        : False,
    "brpc_snappy_compress_unittest"       : False,
    "brpc_ssl_unittest"                   : False,
    "brpc_streaming_rpc_unittest"         : False,
    "brpc_uri_unittest"                   : False,
}

[cc_test(
    name = case[0],
    srcs = [
     	case[0] + ".cpp",
    ],
    deps = [
        ":cc_test_proto",
        ":test_util",
        "//:brpc",
        "//gperftools:gperftools",
        #"@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest_main" if case[1] else "@com_google_googletest//:gtest",
    ],
    copts = COPTS,
    data = [
        ":jsonout",
        ":cert1.crt",
        ":cert1.key",
        ":cert2.crt",
        ":cert2.key",
    ],
) for case in BRPC_UNIT_CASES.items()]
