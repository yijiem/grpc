/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// This is a modification of greeter_async_server.cc from grpc examples.
// Comments have been removed to make it easier to follow the code.
// For comments please refer to the original example.

#include <unistd.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "hellostreamingworld.grpc.pb.h"

#include <grpc/support/log.h>
#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>

using grpc::Alarm;
using grpc::Server;
using grpc::ServerAsyncWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using hellostreamingworld::HelloReply;
using hellostreamingworld::HelloRequest;
using hellostreamingworld::MultiGreeter;

struct Args {
  ServerCompletionQueue* cq;
  void* tag;
};

std::atomic_bool _shutdown(false);

void PutAlarm(void* args) {
  Args* aargs = (Args*) args;
  Alarm alarm;
  while (!_shutdown) {
    std::cout << "set alarm" << std::endl;
    alarm.Set(aargs->cq, gpr_now(gpr_clock_type::GPR_CLOCK_MONOTONIC), aargs->tag);
    sleep(1);
  }
}

class ServerImpl final {
 public:
  ~ServerImpl() {
    server_->Shutdown();
    cq_->Shutdown();
  }

  void Run() {
    std::string server_address("0.0.0.0:50051");

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);

    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    HandleRpcs();
  }

 private:
  class CallData {
   public:
    CallData(MultiGreeter::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service),
          cq_(cq),
          responder_(&ctx_),
          status_(CREATE),
          times_(0) {
      Proceed();
    }

    void Proceed() {
      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestsayHello(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
      } else if (status_ == PROCESS) {
        // Now that we go through this stage multiple times,
        // we don't want to create a new instance every time.
        // Refer to gRPC's original example if you don't understand
        // why we create a new instance of CallData here.
        if (times_ == 0) {
          Args* args = new Args();
          args->cq = cq_;
          args->tag = this;
          alarm_thread_ = std::thread(PutAlarm, args);
          // new CallData(service_, cq_);
        }

        if (times_++ >= 6) {
          status_ = FINISH;
          responder_.Finish(Status::OK, this);
        } else {
          std::string prefix("Hello ");
          reply_.set_message(prefix + request_.name() + ", no " +
                             request_.num_greetings());

          responder_.Write(reply_, this);
        }
      } else {
        std::cout << "status_: " << status_ << std::endl;
        GPR_ASSERT(status_ == FINISH);
        _shutdown = true;
        alarm_thread_.join();
        delete this;
      }
    }

   private:
    bool HasData() {
      // A dummy condition to make it sometimes have data and sometimes not
      return times_ % 2 == 0;
    }

    // void PutTaskBackToQueue() {
    //   alarm_.Set(cq_, gpr_now(gpr_clock_type::GPR_CLOCK_MONOTONIC), this);
    // }

    MultiGreeter::AsyncService* service_;
    ServerCompletionQueue* cq_;
    ServerContext ctx_;

    HelloRequest request_;
    HelloReply reply_;

    ServerAsyncWriter<HelloReply> responder_;

    int times_;
    // Alarm alarm_;
    std::thread alarm_thread_;

    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  // The current serving state.
  };

  void HandleRpcs() {
    new CallData(&service_, cq_.get());
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      std::cout << "Next" << std::endl;
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  MultiGreeter::AsyncService service_;
  std::unique_ptr<Server> server_;
};

int main(int argc, char** argv) {
  ServerImpl server;
  server.Run();

  return 0;
}
