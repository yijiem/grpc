#include <iostream>
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>

#include "examples/protos/helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::StatusCode;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;
using ::testing::AnyOf;
using ::testing::Eq;

namespace {

TEST(StubTest, ConnectToNonExistingEndpoint) {
  std::string target_str = "localhost:50051";
  std::unique_ptr<Greeter::Stub> stub =
      Greeter::NewStub(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  HelloRequest request;
  request.set_name("yijiem");
  HelloReply reply;
  ClientContext context;
  Status status = stub->SayHello(&context, request, &reply);
  std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  EXPECT_THAT(status.error_code(), AnyOf(Eq(StatusCode::UNAVAILABLE),
                                         Eq(StatusCode::INVALID_ARGUMENT),
                                         Eq(StatusCode::DEADLINE_EXCEEDED)));
}

}  // namespace

// int main(int argc, char** argv) {
//   ::testing::InitGoogleTest(&argc, argv);
//   RUN_ALL_TESTS();
// }
