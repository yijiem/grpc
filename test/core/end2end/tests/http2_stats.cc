// Copyright 2023 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "absl/base/thread_annotations.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"

#include <grpc/status.h>
#include <grpc/support/log.h>
#include <grpc/support/time.h>

#include "src/core/lib/channel/call_tracer.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/channel/channel_fwd.h"
#include "src/core/lib/channel/channel_stack.h"
#include "src/core/lib/channel/context.h"
#include "src/core/lib/channel/metrics.h"
#include "src/core/lib/channel/promise_based_filter.h"
#include "src/core/lib/channel/tcp_tracer.h"
#include "src/core/lib/config/core_configuration.h"
#include "src/core/lib/experiments/experiments.h"
#include "src/core/lib/gprpp/notification.h"
#include "src/core/lib/gprpp/sync.h"
#include "src/core/lib/gprpp/time.h"
#include "src/core/lib/iomgr/error.h"
#include "src/core/lib/promise/arena_promise.h"
#include "src/core/lib/promise/context.h"
#include "src/core/lib/resource_quota/arena.h"
#include "src/core/lib/slice/slice.h"
#include "src/core/lib/slice/slice_buffer.h"
#include "src/core/lib/surface/channel_stack_type.h"
#include "src/core/lib/transport/metadata_batch.h"
#include "src/core/lib/transport/transport.h"
#include "test/core/end2end/end2end_tests.h"

namespace grpc_core {
namespace {

Mutex* g_mu;
Notification* g_client_call_ended_notify;
Notification* g_server_call_ended_notify;

class FakeCallTracer : public ClientCallTracer {
 public:
  class FakeCallAttemptTracer : public CallAttemptTracer {
   public:
    std::string TraceId() override { return ""; }
    std::string SpanId() override { return ""; }
    bool IsSampled() override { return false; }
    void RecordSendInitialMetadata(
        grpc_metadata_batch* /*send_initial_metadata*/) override {}
    void RecordSendTrailingMetadata(
        grpc_metadata_batch* /*send_trailing_metadata*/) override {}
    void RecordSendMessage(const SliceBuffer& /*send_message*/) override {}
    void RecordSendCompressedMessage(
        const SliceBuffer& /*send_compressed_message*/) override {}
    void RecordReceivedInitialMetadata(
        grpc_metadata_batch* /*recv_initial_metadata*/) override {}
    void RecordReceivedMessage(const SliceBuffer& /*recv_message*/) override {}
    void RecordReceivedDecompressedMessage(
        const SliceBuffer& /*recv_decompressed_message*/) override {}

    void RecordReceivedTrailingMetadata(
        absl::Status /*status*/,
        grpc_metadata_batch* /*recv_trailing_metadata*/,
        const grpc_transport_stream_stats* transport_stream_stats) override {
      MutexLock lock(g_mu);
      transport_stream_stats_ = *transport_stream_stats;
    }

    void RecordCancel(grpc_error_handle /*cancel_error*/) override {}
    std::shared_ptr<TcpTracerInterface> StartNewTcpTrace() override {
      return nullptr;
    }
    void RecordEnd(const gpr_timespec& /*latency*/) override {
      g_client_call_ended_notify->Notify();
      delete this;
    }
    void RecordAnnotation(absl::string_view /*annotation*/) override {}
    void RecordAnnotation(const Annotation& /*annotation*/) override {}

    void AddOptionalLabels(
        OptionalLabelComponent /*component*/,
        std::shared_ptr<std::map<std::string, std::string>> /*labels*/)
        override {}

    static grpc_transport_stream_stats transport_stream_stats() {
      MutexLock lock(g_mu);
      return transport_stream_stats_;
    }

   private:
    static grpc_transport_stream_stats transport_stream_stats_
        ABSL_GUARDED_BY(g_mu);
  };

  explicit FakeCallTracer() {}
  ~FakeCallTracer() override {}
  std::string TraceId() override { return ""; }
  std::string SpanId() override { return ""; }
  bool IsSampled() override { return false; }

  FakeCallAttemptTracer* StartNewAttempt(
      bool /*is_transparent_retry*/) override {
    return new FakeCallAttemptTracer;
  }

  void RecordAnnotation(absl::string_view /*annotation*/) override {}
  void RecordAnnotation(const Annotation& /*annotation*/) override {}
};

grpc_transport_stream_stats
    FakeCallTracer::FakeCallAttemptTracer::transport_stream_stats_;

class FakeServerCallTracer : public ServerCallTracer {
 public:
  ~FakeServerCallTracer() override {}
  void RecordSendInitialMetadata(
      grpc_metadata_batch* /*send_initial_metadata*/) override {}
  void RecordSendTrailingMetadata(
      grpc_metadata_batch* /*send_trailing_metadata*/) override {}
  void RecordSendMessage(const SliceBuffer& /*send_message*/) override {}
  void RecordSendCompressedMessage(
      const SliceBuffer& /*send_compressed_message*/) override {}
  void RecordReceivedInitialMetadata(
      grpc_metadata_batch* /*recv_initial_metadata*/) override {}
  void RecordReceivedMessage(const SliceBuffer& /*recv_message*/) override {}
  void RecordReceivedDecompressedMessage(
      const SliceBuffer& /*recv_decompressed_message*/) override {}
  void RecordCancel(grpc_error_handle /*cancel_error*/) override {}
  std::shared_ptr<TcpTracerInterface> StartNewTcpTrace() override {
    return nullptr;
  }
  void RecordReceivedTrailingMetadata(
      grpc_metadata_batch* /*recv_trailing_metadata*/) override {}

  void RecordEnd(const grpc_call_final_info* final_info) override {
    MutexLock lock(g_mu);
    transport_stream_stats_ = final_info->stats.transport_stream_stats;
    g_server_call_ended_notify->Notify();
  }

  void RecordAnnotation(absl::string_view /*annotation*/) override {}
  void RecordAnnotation(const Annotation& /*annotation*/) override {}
  std::string TraceId() override { return ""; }
  std::string SpanId() override { return ""; }
  bool IsSampled() override { return false; }

  static grpc_transport_stream_stats transport_stream_stats() {
    MutexLock lock(g_mu);
    return transport_stream_stats_;
  }

 private:
  static grpc_transport_stream_stats transport_stream_stats_
      ABSL_GUARDED_BY(g_mu);
};

grpc_transport_stream_stats FakeServerCallTracer::transport_stream_stats_;

class FakeServerCallTracerFactory : public ServerCallTracerFactory {
 public:
  ServerCallTracer* CreateNewServerCallTracer(
      Arena* arena, const ChannelArgs& /*args*/) override {
    return arena->ManagedNew<FakeServerCallTracer>();
  }
};

class FakeStatsPlugin : public grpc_core::StatsPlugin {
  bool IsEnabledForChannel(const ChannelScope& /*scope*/) const override {
    return true;
  }
  bool IsEnabledForServer(const ChannelArgs& /*args*/) const override {
    return true;
  }
  void AddCounter(
      GlobalInstrumentsRegistry::GlobalUInt64CounterHandle handle,
      uint64_t value, absl::Span<const absl::string_view> label_values,
      absl::Span<const absl::string_view> optional_values) override {}
  void AddCounter(
      GlobalInstrumentsRegistry::GlobalDoubleCounterHandle handle, double value,
      absl::Span<const absl::string_view> label_values,
      absl::Span<const absl::string_view> optional_values) override {}
  void RecordHistogram(
      GlobalInstrumentsRegistry::GlobalUInt64HistogramHandle handle,
      uint64_t value, absl::Span<const absl::string_view> label_values,
      absl::Span<const absl::string_view> optional_values) override {}
  void RecordHistogram(
      GlobalInstrumentsRegistry::GlobalDoubleHistogramHandle handle,
      double value, absl::Span<const absl::string_view> label_values,
      absl::Span<const absl::string_view> optional_values) override {}
  void SetGauge(GlobalInstrumentsRegistry::GlobalInt64GaugeHandle handle,
                int64_t value, absl::Span<const absl::string_view> label_values,
                absl::Span<const absl::string_view> optional_values) override {}
  void SetGauge(GlobalInstrumentsRegistry::GlobalDoubleGaugeHandle handle,
                double value, absl::Span<const absl::string_view> label_values,
                absl::Span<const absl::string_view> optional_values) override {}
  void AddCallback(RegisteredMetricCallback* callback) override {}
  void RemoveCallback(RegisteredMetricCallback* callback) override {}
  ClientCallTracer* GetClientCallTracer(absl::string_view canonical_target,
                                        Slice path, Arena* arena,
                                        bool registered_method) override {
    return arena->ManagedNew<FakeCallTracer>();
  }
  ServerCallTracerFactory* GetServerCallTracerFactory(Arena* arena) override {
    return arena->ManagedNew<FakeServerCallTracerFactory>();
  }
};

// This test verifies the HTTP2 stats on a stream
CORE_END2END_TEST(Http2FullstackSingleHopTest, StreamStats) {
  if (!IsHttp2StatsFixEnabled()) {
    GTEST_SKIP() << "Test needs http2_stats_fix experiment to be enabled";
  }
  g_mu = new Mutex();
  g_client_call_ended_notify = new Notification();
  g_server_call_ended_notify = new Notification();
  grpc_core::GlobalStatsPluginRegistry::RegisterStatsPlugin(
      std::make_shared<FakeStatsPlugin>());
  auto send_from_client = RandomSlice(10);
  auto send_from_server = RandomSlice(20);
  CoreEnd2endTest::IncomingStatusOnClient server_status;
  CoreEnd2endTest::IncomingMetadata server_initial_metadata;
  CoreEnd2endTest::IncomingMessage server_message;
  CoreEnd2endTest::IncomingMessage client_message;
  CoreEnd2endTest::IncomingCloseOnServer client_close;
  {
    auto c = NewClientCall("/foo").Timeout(Duration::Minutes(5)).Create();
    c.NewBatch(1)
        .SendInitialMetadata({})
        .SendMessage(send_from_client.Ref())
        .SendCloseFromClient()
        .RecvInitialMetadata(server_initial_metadata)
        .RecvMessage(server_message)
        .RecvStatusOnClient(server_status);
    auto s = RequestCall(101);
    Expect(101, true);
    Step(Duration::Minutes(1));
    s.NewBatch(102).SendInitialMetadata({}).RecvMessage(client_message);
    Expect(102, true);
    Step(Duration::Minutes(1));
    s.NewBatch(103)
        .SendStatusFromServer(GRPC_STATUS_UNIMPLEMENTED, "xyz", {})
        .SendMessage(send_from_server.Ref())
        .RecvCloseOnServer(client_close);
    Expect(103, true);
    Expect(1, true);
    Step(Duration::Minutes(1));
    EXPECT_EQ(s.method(), "/foo");
  }
  EXPECT_EQ(server_status.status(), GRPC_STATUS_UNIMPLEMENTED);
  EXPECT_EQ(server_status.message(), "xyz");
  EXPECT_FALSE(client_close.was_cancelled());
  EXPECT_EQ(client_message.payload(), send_from_client);
  EXPECT_EQ(server_message.payload(), send_from_server);
  // Make sure that the calls have ended for the stats to have been collected
  g_client_call_ended_notify->WaitForNotificationWithTimeout(absl::Seconds(5));
  g_server_call_ended_notify->WaitForNotificationWithTimeout(absl::Seconds(5));

  auto client_transport_stats =
      FakeCallTracer::FakeCallAttemptTracer::transport_stream_stats();
  auto server_transport_stats = FakeServerCallTracer::transport_stream_stats();
  EXPECT_EQ(client_transport_stats.outgoing.data_bytes,
            send_from_client.size());
  EXPECT_EQ(client_transport_stats.incoming.data_bytes,
            send_from_server.size());
  EXPECT_EQ(server_transport_stats.outgoing.data_bytes,
            send_from_server.size());
  EXPECT_EQ(server_transport_stats.incoming.data_bytes,
            send_from_client.size());
  // At the very minimum, we should have 9 bytes from initial header frame, 9
  // bytes from data header frame, 5 bytes from the grpc header on data and 9
  // bytes from the trailing header frame. The actual number might be more due
  // to RST_STREAM (13 bytes) and WINDOW_UPDATE (13 bytes) frames.
  EXPECT_GE(client_transport_stats.outgoing.framing_bytes, 32);
  EXPECT_LE(client_transport_stats.outgoing.framing_bytes, 58);
  EXPECT_GE(client_transport_stats.incoming.framing_bytes, 32);
  EXPECT_LE(client_transport_stats.incoming.framing_bytes, 58);
  EXPECT_GE(server_transport_stats.outgoing.framing_bytes, 32);
  EXPECT_LE(server_transport_stats.outgoing.framing_bytes, 58);
  EXPECT_GE(server_transport_stats.incoming.framing_bytes, 32);
  EXPECT_LE(server_transport_stats.incoming.framing_bytes, 58);

  delete ServerCallTracerFactory::Get(ChannelArgs());
  ServerCallTracerFactory::RegisterGlobal(nullptr);
  delete g_client_call_ended_notify;
  g_client_call_ended_notify = nullptr;
  delete g_server_call_ended_notify;
  g_server_call_ended_notify = nullptr;
  delete g_mu;
  g_mu = nullptr;
}

}  // namespace
}  // namespace grpc_core
