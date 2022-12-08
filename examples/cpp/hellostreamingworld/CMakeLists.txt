# Just for playing

cmake_minimum_required(VERSION 3.5.1)

project(HelloWorld C CXX)

include(../cmake/common.cmake)

# Proto file
get_filename_component(hsw_proto "../../protos/hellostreamingworld.proto" ABSOLUTE)
get_filename_component(hsw_proto_path "${hsw_proto}" PATH)

# Generated sources
set(hsw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/hellostreamingworld.pb.cc")
set(hsw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/hellostreamingworld.pb.h")
set(hsw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/hellostreamingworld.grpc.pb.cc")
set(hsw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/hellostreamingworld.grpc.pb.h")
add_custom_command(
      OUTPUT "${hsw_proto_srcs}" "${hsw_proto_hdrs}" "${hsw_grpc_srcs}" "${hsw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hsw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hsw_proto}"
      DEPENDS "${hsw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hsw_grpc_proto
add_library(hsw_grpc_proto
  ${hsw_grpc_srcs}
  ${hsw_grpc_hdrs}
  ${hsw_proto_srcs}
  ${hsw_proto_hdrs})
target_link_libraries(hsw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets greeter_[async_](client|server)
foreach(_target
  greeter_streaming_client
  greeter_async_streaming_server
  greeter_async_streaming_server_alarm
  greeter_async_streaming_server_alarm_thread
  greeter_async_streaming_server_queue_to_front
  greeter_async_streaming_server_queue_to_back)
  add_executable(${_target} "${_target}.cc")
  target_link_libraries(${_target}
    hsw_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
