load("//bazel:grpc_build_system.bzl", "grpc_cc_library", "grpc_cc_test", "grpc_package")

licenses(["notice"])

grpc_cc_test(
    name = "stub_test",
    srcs = ["stub_test.cc"],
    external_deps = ["gtest"],
    language = "C++",
    deps = [
        "//:grpc++",
        "//examples/protos:helloworld_cc_grpc",
        "@com_google_googletest//:gtest_main",
    ],
)
