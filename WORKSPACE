# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

workspace(name = "com_github_brpc_brpc")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

skylib_version = "0.8.0"
http_archive(
    name = "bazel_skylib",
    type = "tar.gz",
    url = "https://github.com/bazelbuild/bazel-skylib/releases/download/{}/bazel-skylib.{}.tar.gz".format (skylib_version, skylib_version),
    sha256 = "2ef429f5d7ce7111263289644d233707dba35e39696377ebab8b0bc701f7818e",
)

http_archive(
    name = "rules_foreign_cc",
    sha256 = "c2cdcf55ffaf49366725639e45dedd449b8c3fe22b54e31625eb80ce3a240f1e",
    strip_prefix = "rules_foreign_cc-0.1.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.1.0.zip",
)
load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

http_archive(
  name = "com_google_protobuf",
  strip_prefix = "protobuf-3.6.1.3",
  sha256 = "9510dd2afc29e7245e9e884336f848c8a6600a14ae726adb6befdb4f786f0be2",
  type = "zip",
  url = "https://github.com/protocolbuffers/protobuf/archive/v3.6.1.3.zip",
)

http_archive(
    name = "com_github_gflags_gflags",
    strip_prefix = "gflags-46f73f88b18aee341538c0dfc22b1710a6abedef",
    url = "https://github.com/gflags/gflags/archive/46f73f88b18aee341538c0dfc22b1710a6abedef.tar.gz",
    sha256 = "a8263376b409900dd46830e4e34803a170484707327854cc252fc5865275a57d",
)

all_content = """filegroup(name = "all", srcs = glob(["**"]), visibility = ["//visibility:public"])"""
http_archive(
  name = "com_github_gperftools_gperftools",
  strip_prefix = "gperftools-2.8",
  sha256 = "240deacdd628b6459671b83eb0c4db8e97baadf659f25b92e9a078d536bd513e",
  urls = ["https://github.com/gperftools/gperftools/releases/download/gperftools-2.8/gperftools-2.8.tar.gz"],
  build_file_content = all_content,
)

bind(
    name = "gflags",
    actual = "@com_github_gflags_gflags//:gflags",
)

http_archive(
    name = "com_github_google_leveldb",
    build_file = "//:leveldb.BUILD",
    strip_prefix = "leveldb-a53934a3ae1244679f812d998a4f16f2c7f309a6",
    url = "https://github.com/google/leveldb/archive/a53934a3ae1244679f812d998a4f16f2c7f309a6.tar.gz",
    sha256 = "3912ac36dbb264a62797d68687711c8024919640d89b6733f9342ada1d16cda1",
)

http_archive(
    name = "com_github_google_glog",
    build_file = "//:glog.BUILD",
    strip_prefix = "glog-a6a166db069520dbbd653c97c2e5b12e08a8bb26",
    url = "https://github.com/google/glog/archive/a6a166db069520dbbd653c97c2e5b12e08a8bb26.tar.gz"
)

http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-0fe96607d85cf3a25ac40da369db62bbee2939a5",
    url = "https://github.com/google/googletest/archive/0fe96607d85cf3a25ac40da369db62bbee2939a5.tar.gz",
    sha256 = "80532b7a9c62945eed127a9cfa502f4b9f4af2a0c2329146026fd423e539f578",
)

new_local_repository(
    name = "openssl",
    path = "/usr",
    build_file = "//:openssl.BUILD",
)

new_local_repository(
    name = "openssl_macos",
    build_file = "//:openssl.BUILD",
    path = "/usr/local/opt/openssl",
)

bind(
    name = "ssl",
    actual = "@openssl//:ssl"
)

bind(
    name = "ssl_macos",
    actual = "@openssl_macos//:ssl"
)

new_local_repository(
    name = "zlib",
    build_file = "//:zlib.BUILD",
    path = "/usr",
)
