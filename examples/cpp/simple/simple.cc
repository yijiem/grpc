#include <grpcpp/grpcpp.h>

#include <iostream>

int main() {
  std::cout << grpc::Version() << std::endl;

  grpc_init();
  grpc_shutdown();
}
