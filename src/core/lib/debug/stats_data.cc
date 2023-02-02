//
// Copyright 2017 gRPC authors.
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
//

//
// Automatically generated by tools/codegen/core/gen_stats_data.py
//

#include <grpc/support/port_platform.h>

#include "src/core/lib/debug/stats_data.h"

#include <stdint.h>

namespace grpc_core {
namespace {
union DblUint {
  double dbl;
  uint64_t uint;
};
}  // namespace
void HistogramCollector_65536_26::Collect(Histogram_65536_26* result) const {
  for (int i = 0; i < 26; i++) {
    result->buckets_[i] += buckets_[i].load(std::memory_order_relaxed);
  }
}
Histogram_65536_26 operator-(const Histogram_65536_26& left,
                             const Histogram_65536_26& right) {
  Histogram_65536_26 result;
  for (int i = 0; i < 26; i++) {
    result.buckets_[i] = left.buckets_[i] - right.buckets_[i];
  }
  return result;
}
void HistogramCollector_16777216_20::Collect(
    Histogram_16777216_20* result) const {
  for (int i = 0; i < 20; i++) {
    result->buckets_[i] += buckets_[i].load(std::memory_order_relaxed);
  }
}
Histogram_16777216_20 operator-(const Histogram_16777216_20& left,
                                const Histogram_16777216_20& right) {
  Histogram_16777216_20 result;
  for (int i = 0; i < 20; i++) {
    result.buckets_[i] = left.buckets_[i] - right.buckets_[i];
  }
  return result;
}
void HistogramCollector_80_10::Collect(Histogram_80_10* result) const {
  for (int i = 0; i < 10; i++) {
    result->buckets_[i] += buckets_[i].load(std::memory_order_relaxed);
  }
}
Histogram_80_10 operator-(const Histogram_80_10& left,
                          const Histogram_80_10& right) {
  Histogram_80_10 result;
  for (int i = 0; i < 10; i++) {
    result.buckets_[i] = left.buckets_[i] - right.buckets_[i];
  }
  return result;
}
const absl::string_view
    GlobalStats::counter_name[static_cast<int>(Counter::COUNT)] = {
        "client_calls_created",    "server_calls_created",
        "client_channels_created", "client_subchannels_created",
        "server_channels_created", "insecure_connections_created",
        "syscall_write",           "syscall_read",
        "tcp_read_alloc_8k",       "tcp_read_alloc_64k",
        "http2_settings_writes",   "http2_pings_sent",
        "http2_writes_begun",      "http2_transport_stalls",
        "http2_stream_stalls",     "cq_pluck_creates",
        "cq_next_creates",         "cq_callback_creates",
};
const absl::string_view GlobalStats::counter_doc[static_cast<int>(
    Counter::COUNT)] = {
    "Number of client side calls created by this process",
    "Number of server side calls created by this process",
    "Number of client channels created",
    "Number of client subchannels created",
    "Number of server channels created",
    "Number of insecure connections created",
    "Number of write syscalls (or equivalent - eg sendmsg) made by this "
    "process",
    "Number of read syscalls (or equivalent - eg recvmsg) made by this process",
    "Number of 8k allocations by the TCP subsystem for reading",
    "Number of 64k allocations by the TCP subsystem for reading",
    "Number of settings frames sent",
    "Number of HTTP2 pings sent by process",
    "Number of HTTP2 writes initiated",
    "Number of times sending was completely stalled by the transport flow "
    "control window",
    "Number of times sending was completely stalled by the stream flow control "
    "window",
    "Number of completion queues created for cq_pluck (indicates sync api "
    "usage)",
    "Number of completion queues created for cq_next (indicates cq async api "
    "usage)",
    "Number of completion queues created for cq_callback (indicates callback "
    "api usage)",
};
const absl::string_view GlobalStats::histogram_name[static_cast<int>(
    Histogram::COUNT)] = {
    "call_initial_size",       "tcp_write_size",      "tcp_write_iov_size",
    "tcp_read_size",           "tcp_read_offer",      "tcp_read_offer_iov_size",
    "http2_send_message_size", "http2_metadata_size",
};
const absl::string_view GlobalStats::histogram_doc[static_cast<int>(
    Histogram::COUNT)] = {
    "Initial size of the grpc_call arena created at call start",
    "Number of bytes offered to each syscall_write",
    "Number of byte segments offered to each syscall_write",
    "Number of bytes received by each syscall_read",
    "Number of bytes offered to each syscall_read",
    "Number of byte segments offered to each syscall_read",
    "Size of messages received by HTTP2 transport",
    "Number of bytes consumed by metadata, according to HPACK accounting rules",
};
namespace {
const int kStatsTable0[27] = {0,    1,     2,     4,     7,     11,   17,
                              26,   40,    61,    92,    139,   210,  317,
                              478,  721,   1087,  1638,  2468,  3719, 5604,
                              8443, 12721, 19166, 28875, 43502, 65536};
const uint8_t kStatsTable1[29] = {3,  3,  4,  5,  6,  6,  7,  8,  9,  10,
                                  11, 11, 12, 13, 14, 15, 16, 16, 17, 18,
                                  19, 20, 21, 21, 22, 23, 24, 25, 26};
const int kStatsTable2[21] = {
    0,     1,      3,      8,       19,      45,      106,
    250,   588,    1383,   3252,    7646,    17976,   42262,
    99359, 233593, 549177, 1291113, 3035402, 7136218, 16777216};
const uint8_t kStatsTable3[23] = {2,  3,  3,  4,  5,  6,  7,  8,
                                  8,  9,  10, 11, 12, 12, 13, 14,
                                  15, 16, 16, 17, 18, 19, 20};
const int kStatsTable4[11] = {0, 1, 2, 4, 7, 11, 17, 26, 38, 56, 80};
const uint8_t kStatsTable5[9] = {3, 3, 4, 5, 6, 6, 7, 8, 9};
}  // namespace
int Histogram_65536_26::BucketFor(int value) {
  if (value < 3) {
    if (value < 0) {
      return 0;
    } else {
      return value;
    }
  } else {
    if (value < 49153) {
      DblUint val;
      val.dbl = value;
      const int bucket =
          kStatsTable1[((val.uint - 4613937818241073152ull) >> 51)];
      return bucket - (value < kStatsTable0[bucket]);
    } else {
      return 25;
    }
  }
}
int Histogram_16777216_20::BucketFor(int value) {
  if (value < 2) {
    if (value < 0) {
      return 0;
    } else {
      return value;
    }
  } else {
    if (value < 8388609) {
      DblUint val;
      val.dbl = value;
      const int bucket =
          kStatsTable3[((val.uint - 4611686018427387904ull) >> 52)];
      return bucket - (value < kStatsTable2[bucket]);
    } else {
      return 19;
    }
  }
}
int Histogram_80_10::BucketFor(int value) {
  if (value < 3) {
    if (value < 0) {
      return 0;
    } else {
      return value;
    }
  } else {
    if (value < 49) {
      DblUint val;
      val.dbl = value;
      const int bucket =
          kStatsTable5[((val.uint - 4613937818241073152ull) >> 51)];
      return bucket - (value < kStatsTable4[bucket]);
    } else {
      if (value < 56) {
        return 8;
      } else {
        return 9;
      }
    }
  }
}
GlobalStats::GlobalStats()
    : client_calls_created{0},
      server_calls_created{0},
      client_channels_created{0},
      client_subchannels_created{0},
      server_channels_created{0},
      insecure_connections_created{0},
      syscall_write{0},
      syscall_read{0},
      tcp_read_alloc_8k{0},
      tcp_read_alloc_64k{0},
      http2_settings_writes{0},
      http2_pings_sent{0},
      http2_writes_begun{0},
      http2_transport_stalls{0},
      http2_stream_stalls{0},
      cq_pluck_creates{0},
      cq_next_creates{0},
      cq_callback_creates{0} {}
HistogramView GlobalStats::histogram(Histogram which) const {
  switch (which) {
    default:
      GPR_UNREACHABLE_CODE(return HistogramView());
    case Histogram::kCallInitialSize:
      return HistogramView{&Histogram_65536_26::BucketFor, kStatsTable0, 26,
                           call_initial_size.buckets()};
    case Histogram::kTcpWriteSize:
      return HistogramView{&Histogram_16777216_20::BucketFor, kStatsTable2, 20,
                           tcp_write_size.buckets()};
    case Histogram::kTcpWriteIovSize:
      return HistogramView{&Histogram_80_10::BucketFor, kStatsTable4, 10,
                           tcp_write_iov_size.buckets()};
    case Histogram::kTcpReadSize:
      return HistogramView{&Histogram_16777216_20::BucketFor, kStatsTable2, 20,
                           tcp_read_size.buckets()};
    case Histogram::kTcpReadOffer:
      return HistogramView{&Histogram_16777216_20::BucketFor, kStatsTable2, 20,
                           tcp_read_offer.buckets()};
    case Histogram::kTcpReadOfferIovSize:
      return HistogramView{&Histogram_80_10::BucketFor, kStatsTable4, 10,
                           tcp_read_offer_iov_size.buckets()};
    case Histogram::kHttp2SendMessageSize:
      return HistogramView{&Histogram_16777216_20::BucketFor, kStatsTable2, 20,
                           http2_send_message_size.buckets()};
    case Histogram::kHttp2MetadataSize:
      return HistogramView{&Histogram_65536_26::BucketFor, kStatsTable0, 26,
                           http2_metadata_size.buckets()};
  }
}
std::unique_ptr<GlobalStats> GlobalStatsCollector::Collect() const {
  auto result = std::make_unique<GlobalStats>();
  for (const auto& data : data_) {
    result->client_calls_created +=
        data.client_calls_created.load(std::memory_order_relaxed);
    result->server_calls_created +=
        data.server_calls_created.load(std::memory_order_relaxed);
    result->client_channels_created +=
        data.client_channels_created.load(std::memory_order_relaxed);
    result->client_subchannels_created +=
        data.client_subchannels_created.load(std::memory_order_relaxed);
    result->server_channels_created +=
        data.server_channels_created.load(std::memory_order_relaxed);
    result->insecure_connections_created +=
        data.insecure_connections_created.load(std::memory_order_relaxed);
    result->syscall_write += data.syscall_write.load(std::memory_order_relaxed);
    result->syscall_read += data.syscall_read.load(std::memory_order_relaxed);
    result->tcp_read_alloc_8k +=
        data.tcp_read_alloc_8k.load(std::memory_order_relaxed);
    result->tcp_read_alloc_64k +=
        data.tcp_read_alloc_64k.load(std::memory_order_relaxed);
    result->http2_settings_writes +=
        data.http2_settings_writes.load(std::memory_order_relaxed);
    result->http2_pings_sent +=
        data.http2_pings_sent.load(std::memory_order_relaxed);
    result->http2_writes_begun +=
        data.http2_writes_begun.load(std::memory_order_relaxed);
    result->http2_transport_stalls +=
        data.http2_transport_stalls.load(std::memory_order_relaxed);
    result->http2_stream_stalls +=
        data.http2_stream_stalls.load(std::memory_order_relaxed);
    result->cq_pluck_creates +=
        data.cq_pluck_creates.load(std::memory_order_relaxed);
    result->cq_next_creates +=
        data.cq_next_creates.load(std::memory_order_relaxed);
    result->cq_callback_creates +=
        data.cq_callback_creates.load(std::memory_order_relaxed);
    data.call_initial_size.Collect(&result->call_initial_size);
    data.tcp_write_size.Collect(&result->tcp_write_size);
    data.tcp_write_iov_size.Collect(&result->tcp_write_iov_size);
    data.tcp_read_size.Collect(&result->tcp_read_size);
    data.tcp_read_offer.Collect(&result->tcp_read_offer);
    data.tcp_read_offer_iov_size.Collect(&result->tcp_read_offer_iov_size);
    data.http2_send_message_size.Collect(&result->http2_send_message_size);
    data.http2_metadata_size.Collect(&result->http2_metadata_size);
  }
  return result;
}
std::unique_ptr<GlobalStats> GlobalStats::Diff(const GlobalStats& other) const {
  auto result = std::make_unique<GlobalStats>();
  result->client_calls_created =
      client_calls_created - other.client_calls_created;
  result->server_calls_created =
      server_calls_created - other.server_calls_created;
  result->client_channels_created =
      client_channels_created - other.client_channels_created;
  result->client_subchannels_created =
      client_subchannels_created - other.client_subchannels_created;
  result->server_channels_created =
      server_channels_created - other.server_channels_created;
  result->insecure_connections_created =
      insecure_connections_created - other.insecure_connections_created;
  result->syscall_write = syscall_write - other.syscall_write;
  result->syscall_read = syscall_read - other.syscall_read;
  result->tcp_read_alloc_8k = tcp_read_alloc_8k - other.tcp_read_alloc_8k;
  result->tcp_read_alloc_64k = tcp_read_alloc_64k - other.tcp_read_alloc_64k;
  result->http2_settings_writes =
      http2_settings_writes - other.http2_settings_writes;
  result->http2_pings_sent = http2_pings_sent - other.http2_pings_sent;
  result->http2_writes_begun = http2_writes_begun - other.http2_writes_begun;
  result->http2_transport_stalls =
      http2_transport_stalls - other.http2_transport_stalls;
  result->http2_stream_stalls = http2_stream_stalls - other.http2_stream_stalls;
  result->cq_pluck_creates = cq_pluck_creates - other.cq_pluck_creates;
  result->cq_next_creates = cq_next_creates - other.cq_next_creates;
  result->cq_callback_creates = cq_callback_creates - other.cq_callback_creates;
  result->call_initial_size = call_initial_size - other.call_initial_size;
  result->tcp_write_size = tcp_write_size - other.tcp_write_size;
  result->tcp_write_iov_size = tcp_write_iov_size - other.tcp_write_iov_size;
  result->tcp_read_size = tcp_read_size - other.tcp_read_size;
  result->tcp_read_offer = tcp_read_offer - other.tcp_read_offer;
  result->tcp_read_offer_iov_size =
      tcp_read_offer_iov_size - other.tcp_read_offer_iov_size;
  result->http2_send_message_size =
      http2_send_message_size - other.http2_send_message_size;
  result->http2_metadata_size = http2_metadata_size - other.http2_metadata_size;
  return result;
}
}  // namespace grpc_core
