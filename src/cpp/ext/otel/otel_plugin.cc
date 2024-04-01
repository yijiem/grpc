//
//
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
//
//

#include <grpc/support/port_platform.h>

#include "src/cpp/ext/otel/otel_plugin.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "opentelemetry/metrics/meter.h"
#include "opentelemetry/metrics/meter_provider.h"
#include "opentelemetry/metrics/sync_instruments.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/nostd/unique_ptr.h"
#include "opentelemetry/nostd/variant.h"

#include <grpc/support/log.h>
#include <grpcpp/ext/otel_plugin.h>
#include <grpcpp/version_info.h>

#include "src/core/client_channel/client_channel_filter.h"
#include "src/core/lib/channel/call_tracer.h"
#include "src/core/lib/channel/channel_args.h"
#include "src/core/lib/config/core_configuration.h"
#include "src/core/lib/gprpp/match.h"
#include "src/core/lib/surface/channel_stack_type.h"
#include "src/cpp/ext/otel/key_value_iterable.h"
#include "src/cpp/ext/otel/otel_client_call_tracer.h"
#include "src/cpp/ext/otel/otel_server_call_tracer.h"

namespace grpc {
namespace internal {

absl::string_view OpenTelemetryMethodKey() { return "grpc.method"; }

absl::string_view OpenTelemetryStatusKey() { return "grpc.status"; }

absl::string_view OpenTelemetryTargetKey() { return "grpc.target"; }

namespace {
absl::flat_hash_set<std::string> BaseMetrics() {
  absl::flat_hash_set<std::string> base_metrics{
      std::string(grpc::OpenTelemetryPluginBuilder::
                      kClientAttemptStartedInstrumentName),
      std::string(grpc::OpenTelemetryPluginBuilder::
                      kClientAttemptDurationInstrumentName),
      std::string(
          grpc::OpenTelemetryPluginBuilder::
              kClientAttemptSentTotalCompressedMessageSizeInstrumentName),
      std::string(
          grpc::OpenTelemetryPluginBuilder::
              kClientAttemptRcvdTotalCompressedMessageSizeInstrumentName),
      std::string(
          grpc::OpenTelemetryPluginBuilder::kServerCallStartedInstrumentName),
      std::string(
          grpc::OpenTelemetryPluginBuilder::kServerCallDurationInstrumentName),
      std::string(grpc::OpenTelemetryPluginBuilder::
                      kServerCallSentTotalCompressedMessageSizeInstrumentName),
      std::string(grpc::OpenTelemetryPluginBuilder::
                      kServerCallRcvdTotalCompressedMessageSizeInstrumentName)};
  grpc_core::GlobalInstrumentsRegistry::ForEach(
      [&](const grpc_core::GlobalInstrumentsRegistry::
              GlobalInstrumentDescriptor& descriptor) {
        if (descriptor.enable_by_default) {
          base_metrics.emplace(descriptor.name);
        }
      });
  return base_metrics;
}
}  // namespace

class OpenTelemetryPlugin::NPCMetricsKeyValueIterable
    : public opentelemetry::common::KeyValueIterable {
 public:
  NPCMetricsKeyValueIterable(
      absl::Span<const absl::string_view> label_keys,
      absl::Span<const absl::string_view> label_values,
      absl::Span<const absl::string_view> optional_label_keys,
      absl::Span<const absl::string_view> optional_label_values,
      const OptionalLabelsBitSet& optional_labels_bits)
      : label_keys_(label_keys),
        label_values_(label_values),
        optional_label_keys_(optional_label_keys),
        optional_label_values_(optional_label_values),
        optional_labels_bits_(optional_labels_bits) {}

  bool ForEachKeyValue(opentelemetry::nostd::function_ref<
                       bool(opentelemetry::nostd::string_view,
                            opentelemetry::common::AttributeValue)>
                           callback) const noexcept override {
    for (size_t i = 0; i < label_keys_.size(); i++) {
      if (!callback(AbslStrViewToOpenTelemetryStrView(label_keys_[i]),
                    AbslStrViewToOpenTelemetryStrView(label_values_[i]))) {
        return false;
      }
    }
    for (size_t i = 0; i < optional_label_keys_.size(); ++i) {
      if (!optional_labels_bits_.test(i)) {
        continue;
      }
      if (!callback(
              AbslStrViewToOpenTelemetryStrView(optional_label_keys_[i]),
              AbslStrViewToOpenTelemetryStrView(optional_label_values_[i]))) {
        return false;
      }
    }
    return true;
  }

  size_t size() const noexcept override {
    return label_keys_.size() + optional_labels_bits_.count();
  }

 private:
  absl::Span<const absl::string_view> label_keys_;
  absl::Span<const absl::string_view> label_values_;
  absl::Span<const absl::string_view> optional_label_keys_;
  absl::Span<const absl::string_view> optional_label_values_;
  const OptionalLabelsBitSet& optional_labels_bits_;
};

//
// OpenTelemetryPluginBuilderImpl
//

OpenTelemetryPluginBuilderImpl::OpenTelemetryPluginBuilderImpl()
    : metrics_(BaseMetrics()) {}

OpenTelemetryPluginBuilderImpl::~OpenTelemetryPluginBuilderImpl() = default;

OpenTelemetryPluginBuilderImpl&
OpenTelemetryPluginBuilderImpl::SetMeterProvider(
    std::shared_ptr<opentelemetry::metrics::MeterProvider> meter_provider) {
  meter_provider_ = std::move(meter_provider);
  return *this;
}

OpenTelemetryPluginBuilderImpl& OpenTelemetryPluginBuilderImpl::EnableMetrics(
    absl::Span<const absl::string_view> metric_names) {
  for (const auto& metric_name : metric_names) {
    metrics_.emplace(metric_name);
  }
  return *this;
}

OpenTelemetryPluginBuilderImpl& OpenTelemetryPluginBuilderImpl::DisableMetrics(
    absl::Span<const absl::string_view> metric_names) {
  for (const auto& metric_name : metric_names) {
    metrics_.erase(metric_name);
  }
  return *this;
}

OpenTelemetryPluginBuilderImpl&
OpenTelemetryPluginBuilderImpl::DisableAllMetrics() {
  metrics_.clear();
  return *this;
}

OpenTelemetryPluginBuilderImpl&
OpenTelemetryPluginBuilderImpl::SetTargetSelector(
    absl::AnyInvocable<bool(absl::string_view /*target*/) const>
        target_selector) {
  target_selector_ = std::move(target_selector);
  return *this;
}

OpenTelemetryPluginBuilderImpl&
OpenTelemetryPluginBuilderImpl::SetTargetAttributeFilter(
    absl::AnyInvocable<bool(absl::string_view /*target*/) const>
        target_attribute_filter) {
  target_attribute_filter_ = std::move(target_attribute_filter);
  return *this;
}

OpenTelemetryPluginBuilderImpl&
OpenTelemetryPluginBuilderImpl::SetGenericMethodAttributeFilter(
    absl::AnyInvocable<bool(absl::string_view /*generic_method*/) const>
        generic_method_attribute_filter) {
  generic_method_attribute_filter_ = std::move(generic_method_attribute_filter);
  return *this;
}

OpenTelemetryPluginBuilderImpl&
OpenTelemetryPluginBuilderImpl::SetServerSelector(
    absl::AnyInvocable<bool(const grpc_core::ChannelArgs& /*args*/) const>
        server_selector) {
  server_selector_ = std::move(server_selector);
  return *this;
}

OpenTelemetryPluginBuilderImpl& OpenTelemetryPluginBuilderImpl::AddPluginOption(
    std::unique_ptr<InternalOpenTelemetryPluginOption> option) {
  // We allow a limit of 64 plugin options to be registered at this time.
  GPR_ASSERT(plugin_options_.size() < 64);
  plugin_options_.push_back(std::move(option));
  return *this;
}

OpenTelemetryPluginBuilderImpl&
OpenTelemetryPluginBuilderImpl::AddOptionalLabel(
    absl::string_view optional_label_key) {
  if (optional_label_keys_ == nullptr) {
    optional_label_keys_ = std::make_shared<std::set<absl::string_view>>();
  }
  optional_label_keys_->emplace(optional_label_key);
  return *this;
}

absl::Status OpenTelemetryPluginBuilderImpl::BuildAndRegisterGlobal() {
  if (meter_provider_ == nullptr) {
    return absl::OkStatus();
  }
  grpc_core::GlobalStatsPluginRegistry::RegisterStatsPlugin(
      std::make_shared<OpenTelemetryPlugin>(
          metrics_, meter_provider_, std::move(target_selector_),
          std::move(target_attribute_filter_),
          std::move(generic_method_attribute_filter_),
          std::move(server_selector_), std::move(plugin_options_),
          std::move(optional_label_keys_)));
  return absl::OkStatus();
}

OpenTelemetryPlugin::OpenTelemetryPlugin(
    const absl::flat_hash_set<std::string>& metrics,
    opentelemetry::nostd::shared_ptr<opentelemetry::metrics::MeterProvider>
        meter_provider,
    absl::AnyInvocable<bool(absl::string_view /*target*/) const>
        target_selector,
    absl::AnyInvocable<bool(absl::string_view /*target*/) const>
        target_attribute_filter,
    absl::AnyInvocable<bool(absl::string_view /*generic_method*/) const>
        generic_method_attribute_filter,
    absl::AnyInvocable<bool(const grpc_core::ChannelArgs& /*args*/) const>
        server_selector,
    std::vector<std::unique_ptr<InternalOpenTelemetryPluginOption>>
        plugin_options,
    std::shared_ptr<std::set<absl::string_view>> optional_label_keys)
    : meter_provider_(std::move(meter_provider)),
      target_selector_(std::move(target_selector)),
      server_selector_(std::move(server_selector)),
      target_attribute_filter_(std::move(target_attribute_filter)),
      generic_method_attribute_filter_(
          std::move(generic_method_attribute_filter)),
      plugin_options_(std::move(plugin_options)) {
  auto meter = meter_provider_->GetMeter("grpc-c++", GRPC_CPP_VERSION_STRING);
  // Per-call metrics.
  if (metrics.contains(grpc::OpenTelemetryPluginBuilder::
                           kClientAttemptStartedInstrumentName)) {
    client_.attempt.started = meter->CreateUInt64Counter(
        std::string(grpc::OpenTelemetryPluginBuilder::
                        kClientAttemptStartedInstrumentName),
        "Number of client call attempts started", "{attempt}");
  }
  if (metrics.contains(grpc::OpenTelemetryPluginBuilder::
                           kClientAttemptDurationInstrumentName)) {
    client_.attempt.duration = meter->CreateDoubleHistogram(
        std::string(grpc::OpenTelemetryPluginBuilder::
                        kClientAttemptDurationInstrumentName),
        "End-to-end time taken to complete a client call attempt", "s");
  }
  if (metrics.contains(
          grpc::OpenTelemetryPluginBuilder::
              kClientAttemptSentTotalCompressedMessageSizeInstrumentName)) {
    client_.attempt.sent_total_compressed_message_size =
        meter->CreateUInt64Histogram(
            std::string(
                grpc::OpenTelemetryPluginBuilder::
                    kClientAttemptSentTotalCompressedMessageSizeInstrumentName),
            "Compressed message bytes sent per client call attempt", "By");
  }
  if (metrics.contains(
          grpc::OpenTelemetryPluginBuilder::
              kClientAttemptRcvdTotalCompressedMessageSizeInstrumentName)) {
    client_.attempt.rcvd_total_compressed_message_size =
        meter->CreateUInt64Histogram(
            std::string(
                grpc::OpenTelemetryPluginBuilder::
                    kClientAttemptRcvdTotalCompressedMessageSizeInstrumentName),
            "Compressed message bytes received per call attempt", "By");
  }
  if (metrics.contains(
          grpc::OpenTelemetryPluginBuilder::kServerCallStartedInstrumentName)) {
    server_.call.started = meter->CreateUInt64Counter(
        std::string(
            grpc::OpenTelemetryPluginBuilder::kServerCallStartedInstrumentName),
        "Number of server calls started", "{call}");
  }
  if (metrics.contains(grpc::OpenTelemetryPluginBuilder::
                           kServerCallDurationInstrumentName)) {
    server_.call.duration = meter->CreateDoubleHistogram(
        std::string(grpc::OpenTelemetryPluginBuilder::
                        kServerCallDurationInstrumentName),
        "End-to-end time taken to complete a call from server transport's "
        "perspective",
        "s");
  }
  if (metrics.contains(
          grpc::OpenTelemetryPluginBuilder::
              kServerCallSentTotalCompressedMessageSizeInstrumentName)) {
    server_.call.sent_total_compressed_message_size =
        meter->CreateUInt64Histogram(
            std::string(
                grpc::OpenTelemetryPluginBuilder::
                    kServerCallSentTotalCompressedMessageSizeInstrumentName),
            "Compressed message bytes sent per server call", "By");
  }
  if (metrics.contains(
          grpc::OpenTelemetryPluginBuilder::
              kServerCallRcvdTotalCompressedMessageSizeInstrumentName)) {
    server_.call.rcvd_total_compressed_message_size =
        meter->CreateUInt64Histogram(
            std::string(
                grpc::OpenTelemetryPluginBuilder::
                    kServerCallRcvdTotalCompressedMessageSizeInstrumentName),
            "Compressed message bytes received per server call", "By");
  }
  // Non-per-call metrics.
  grpc_core::GlobalInstrumentsRegistry::ForEach(
      [&, this](const grpc_core::GlobalInstrumentsRegistry::
                    GlobalInstrumentDescriptor& descriptor) {
        GPR_ASSERT(descriptor.optional_label_keys.size() <=
                   kOptionalLabelsSizeLimit);
        if (instruments_data_.size() < descriptor.index + 1) {
          instruments_data_.resize(descriptor.index + 1);
        }
        if (!metrics.contains(descriptor.name)) {
          return;
        }
        switch (descriptor.instrument_type) {
          case grpc_core::GlobalInstrumentsRegistry::InstrumentType::kCounter:
            switch (descriptor.value_type) {
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kUInt64:
                instruments_data_[descriptor.index].instrument =
                    meter->CreateUInt64Counter(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                break;
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kDouble:
                instruments_data_[descriptor.index].instrument =
                    meter->CreateDoubleCounter(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                break;
              default:
                grpc_core::Crash(
                    absl::StrFormat("Unknown or unsupported value type: %d",
                                    descriptor.value_type));
            }
            break;
          case grpc_core::GlobalInstrumentsRegistry::InstrumentType::kHistogram:
            switch (descriptor.value_type) {
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kUInt64:
                instruments_data_[descriptor.index].instrument =
                    meter->CreateUInt64Histogram(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                break;
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kDouble:
                instruments_data_[descriptor.index].instrument =
                    meter->CreateDoubleHistogram(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                break;
              default:
                grpc_core::Crash(
                    absl::StrFormat("Unknown or unsupported value type: %d",
                                    descriptor.value_type));
            }
            break;
          case grpc_core::GlobalInstrumentsRegistry::InstrumentType::kGauge:
            switch (descriptor.value_type) {
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kInt64: {
                auto observable_state = std::make_unique<GaugeState<int64_t>>();
                observable_state->id = descriptor.index;
                observable_state->ot_plugin = this;
                observable_state->instrument =
                    meter->CreateInt64ObservableGauge(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                instruments_data_[descriptor.index].instrument =
                    std::move(observable_state);
                break;
              }
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kDouble: {
                auto observable_state = std::make_unique<GaugeState<double>>();
                observable_state->id = descriptor.index;
                observable_state->ot_plugin = this;
                observable_state->instrument =
                    meter->CreateDoubleObservableGauge(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                instruments_data_[descriptor.index].instrument =
                    std::move(observable_state);
                break;
              }
              default:
                grpc_core::Crash(
                    absl::StrFormat("Unknown or unsupported value type: %d",
                                    descriptor.value_type));
            }
            break;
          case grpc_core::GlobalInstrumentsRegistry::InstrumentType::
              kCallbackGauge:
            switch (descriptor.value_type) {
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kInt64: {
                auto observable_state =
                    std::make_unique<CallbackGaugeState<int64_t>>();
                observable_state->id = descriptor.index;
                observable_state->ot_plugin = this;
                observable_state->instrument =
                    meter->CreateInt64ObservableGauge(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                instruments_data_[descriptor.index].instrument =
                    std::move(observable_state);
                break;
              }
              case grpc_core::GlobalInstrumentsRegistry::ValueType::kDouble: {
                auto observable_state =
                    std::make_unique<CallbackGaugeState<double>>();
                observable_state->id = descriptor.index;
                observable_state->ot_plugin = this;
                observable_state->instrument =
                    meter->CreateDoubleObservableGauge(
                        std::string(descriptor.name),
                        std::string(descriptor.description),
                        std::string(descriptor.unit));
                instruments_data_[descriptor.index].instrument =
                    std::move(observable_state);
                break;
              }
              default:
                grpc_core::Crash(
                    absl::StrFormat("Unknown or unsupported value type: %d",
                                    descriptor.value_type));
            }
            break;
          default:
            grpc_core::Crash(absl::StrFormat("Unknown instrument_type: %d",
                                             descriptor.instrument_type));
        }
        for (size_t i = 0; i < descriptor.optional_label_keys.size(); ++i) {
          if (optional_label_keys->find(descriptor.optional_label_keys[i]) !=
              optional_label_keys->end()) {
            instruments_data_[descriptor.index].optional_labels_bits.set(i);
          }
        }
      });
}

OpenTelemetryPlugin::CallbackMetricReporter::CallbackMetricReporter(
    OpenTelemetryPlugin* ot_plugin, grpc_core::RegisteredMetricCallback* key)
    : ot_plugin_(ot_plugin), key_(key) {
  // Since we are updating the timestamp and updating the cache for all
  // registered instruments in a RegisteredMetricCallback, we will need to
  // clear all the cache cells for this RegisteredMetricCallback first, so
  // that if a particular combination of labels was previously present but
  // is no longer present, we won't continue to report it.
  for (const auto& handle : key->metrics()) {
    grpc_core::Match(
        handle,
        [&](const grpc_core::GlobalInstrumentsRegistry::
                GlobalCallbackInt64GaugeHandle& handle) {
          auto& callback_gauge_state =
              absl::get<std::unique_ptr<CallbackGaugeState<int64_t>>>(
                  ot_plugin_->instruments_data_.at(handle.index).instrument);
          callback_gauge_state->caches[key].clear();
        },
        [&](const grpc_core::GlobalInstrumentsRegistry::
                GlobalCallbackDoubleGaugeHandle& handle) {
          auto& callback_gauge_state =
              absl::get<std::unique_ptr<CallbackGaugeState<double>>>(
                  ot_plugin_->instruments_data_.at(handle.index).instrument);
          callback_gauge_state->caches[key].clear();
        });
  }
}

void OpenTelemetryPlugin::CallbackMetricReporter::Report(
    grpc_core::GlobalInstrumentsRegistry::GlobalCallbackInt64GaugeHandle handle,
    int64_t value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = ot_plugin_->instruments_data_.at(handle.index);
  auto* callback_gauge_state =
      absl::get_if<std::unique_ptr<CallbackGaugeState<int64_t>>>(
          &instrument_data.instrument);
  GPR_ASSERT(callback_gauge_state != nullptr);
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  auto& cell = (*callback_gauge_state)->caches.at(key_);
  const auto key = std::make_pair(
      std::vector<absl::string_view>(label_values.begin(), label_values.end()),
      std::vector<absl::string_view>(optional_values.begin(),
                                     optional_values.end()));
  bool inserted = false;
  std::tie(std::ignore, inserted) = cell.emplace(key, value);
  GPR_ASSERT(inserted);
}

void OpenTelemetryPlugin::CallbackMetricReporter::Report(
    grpc_core::GlobalInstrumentsRegistry::GlobalCallbackDoubleGaugeHandle
        handle,
    double value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = ot_plugin_->instruments_data_.at(handle.index);
  auto* callback_gauge_state =
      absl::get_if<std::unique_ptr<CallbackGaugeState<double>>>(
          &instrument_data.instrument);
  GPR_ASSERT(callback_gauge_state != nullptr);
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  auto& cell = (*callback_gauge_state)->caches.at(key_);
  const auto key = std::make_pair(
      std::vector<absl::string_view>(label_values.begin(), label_values.end()),
      std::vector<absl::string_view>(optional_values.begin(),
                                     optional_values.end()));
  bool inserted = false;
  std::tie(std::ignore, inserted) = cell.emplace(key, value);
  GPR_ASSERT(inserted);
}

std::pair<bool, std::shared_ptr<grpc_core::StatsPlugin::ScopeConfig>>
OpenTelemetryPlugin::IsEnabledForChannel(const ChannelScope& scope) const {
  if (target_selector_ == nullptr || target_selector_(scope.target())) {
    return {true, std::make_shared<ClientScopeConfig>(this, scope)};
  }
  return {false, nullptr};
}

std::pair<bool, std::shared_ptr<grpc_core::StatsPlugin::ScopeConfig>>
OpenTelemetryPlugin::IsEnabledForServer(
    const grpc_core::ChannelArgs& args) const {
  // Return true only if there is no server selector registered or if the server
  // selector returns true.
  if (server_selector_ == nullptr || server_selector_(args)) {
    return {true, std::make_shared<ServerScopeConfig>(this, args)};
  }
  return {false, nullptr};
}

void OpenTelemetryPlugin::AddCounter(
    grpc_core::GlobalInstrumentsRegistry::GlobalUInt64CounterHandle handle,
    uint64_t value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = instruments_data_.at(handle.index);
  if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
    // This instrument is disabled.
    return;
  }
  GPR_ASSERT(absl::holds_alternative<
             std::unique_ptr<opentelemetry::metrics::Counter<uint64_t>>>(
      instrument_data.instrument));
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  absl::get<std::unique_ptr<opentelemetry::metrics::Counter<uint64_t>>>(
      instrument_data.instrument)
      ->Add(value, NPCMetricsKeyValueIterable(
                       descriptor.label_keys, label_values,
                       descriptor.optional_label_keys, optional_values,
                       instrument_data.optional_labels_bits));
}

void OpenTelemetryPlugin::AddCounter(
    grpc_core::GlobalInstrumentsRegistry::GlobalDoubleCounterHandle handle,
    double value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = instruments_data_.at(handle.index);
  if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
    // This instrument is disabled.
    return;
  }
  GPR_ASSERT(absl::holds_alternative<
             std::unique_ptr<opentelemetry::metrics::Counter<double>>>(
      instrument_data.instrument));
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  absl::get<std::unique_ptr<opentelemetry::metrics::Counter<double>>>(
      instrument_data.instrument)
      ->Add(value, NPCMetricsKeyValueIterable(
                       descriptor.label_keys, label_values,
                       descriptor.optional_label_keys, optional_values,
                       instrument_data.optional_labels_bits));
}

void OpenTelemetryPlugin::RecordHistogram(
    grpc_core::GlobalInstrumentsRegistry::GlobalUInt64HistogramHandle handle,
    uint64_t value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = instruments_data_.at(handle.index);
  if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
    // This instrument is disabled.
    return;
  }
  GPR_ASSERT(absl::holds_alternative<
             std::unique_ptr<opentelemetry::metrics::Histogram<uint64_t>>>(
      instrument_data.instrument));
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  absl::get<std::unique_ptr<opentelemetry::metrics::Histogram<uint64_t>>>(
      instrument_data.instrument)
      ->Record(value,
               NPCMetricsKeyValueIterable(descriptor.label_keys, label_values,
                                          descriptor.optional_label_keys,
                                          optional_values,
                                          instrument_data.optional_labels_bits),
               opentelemetry::context::Context{});
}

void OpenTelemetryPlugin::RecordHistogram(
    grpc_core::GlobalInstrumentsRegistry::GlobalDoubleHistogramHandle handle,
    double value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = instruments_data_.at(handle.index);
  if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
    // This instrument is disabled.
    return;
  }
  GPR_ASSERT(absl::holds_alternative<
             std::unique_ptr<opentelemetry::metrics::Histogram<double>>>(
      instrument_data.instrument));
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  absl::get<std::unique_ptr<opentelemetry::metrics::Histogram<double>>>(
      instrument_data.instrument)
      ->Record(value,
               NPCMetricsKeyValueIterable(descriptor.label_keys, label_values,
                                          descriptor.optional_label_keys,
                                          optional_values,
                                          instrument_data.optional_labels_bits),
               opentelemetry::context::Context{});
}

void OpenTelemetryPlugin::SetGauge(
    grpc_core::GlobalInstrumentsRegistry::GlobalInt64GaugeHandle handle,
    int64_t value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = instruments_data_.at(handle.index);
  if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
    // This instrument is disabled.
    return;
  }
  GPR_ASSERT(absl::holds_alternative<std::unique_ptr<GaugeState<int64_t>>>(
      instrument_data.instrument));
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  auto& gauge_state = absl::get<std::unique_ptr<GaugeState<int64_t>>>(
      instrument_data.instrument);
  const auto key = std::make_pair(
      std::vector<absl::string_view>(label_values.begin(), label_values.end()),
      std::vector<absl::string_view>(optional_values.begin(),
                                     optional_values.end()));
  grpc_core::MutexLock lock(&gauge_state->mu);
  auto iter = gauge_state->cache.find(key);
  if (iter == gauge_state->cache.end()) {
    gauge_state->cache.emplace(key, value);
  } else {
    iter->second = value;
  }
  if (!std::exchange(gauge_state->ot_callback_registered, true)) {
    gauge_state->instrument->AddCallback(&GaugeCallback<int64_t>,
                                         gauge_state.get());
  }
}

void OpenTelemetryPlugin::SetGauge(
    grpc_core::GlobalInstrumentsRegistry::GlobalDoubleGaugeHandle handle,
    double value, absl::Span<const absl::string_view> label_values,
    absl::Span<const absl::string_view> optional_values) {
  const auto& instrument_data = instruments_data_.at(handle.index);
  if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
    // This instrument is disabled.
    return;
  }
  GPR_ASSERT(absl::holds_alternative<std::unique_ptr<GaugeState<double>>>(
      instrument_data.instrument));
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(handle);
  GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
  GPR_ASSERT(descriptor.optional_label_keys.size() == optional_values.size());
  auto& gauge_state = absl::get<std::unique_ptr<GaugeState<double>>>(
      instrument_data.instrument);
  const auto key = std::make_pair(
      std::vector<absl::string_view>(label_values.begin(), label_values.end()),
      std::vector<absl::string_view>(optional_values.begin(),
                                     optional_values.end()));
  grpc_core::MutexLock lock(&gauge_state->mu);
  auto iter = gauge_state->cache.find(key);
  if (iter == gauge_state->cache.end()) {
    gauge_state->cache.emplace(key, value);
  } else {
    iter->second = value;
  }
  if (!std::exchange(gauge_state->ot_callback_registered, true)) {
    gauge_state->instrument->AddCallback(&GaugeCallback<double>,
                                         gauge_state.get());
  }
}

void OpenTelemetryPlugin::AddCallback(
    grpc_core::RegisteredMetricCallback* callback) {
  grpc_core::MutexLock lock(&mu_);
  callback_timestamps_.emplace(callback, grpc_core::Timestamp{});
  for (const auto& handle : callback->metrics()) {
    grpc_core::Match(
        handle,
        [&](const grpc_core::GlobalInstrumentsRegistry::
                GlobalCallbackInt64GaugeHandle& handle) {
          const auto& instrument_data = instruments_data_.at(handle.index);
          if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
            // This instrument is disabled.
            return;
          }
          GPR_ASSERT(absl::holds_alternative<
                     std::unique_ptr<CallbackGaugeState<int64_t>>>(
              instrument_data.instrument));
          auto& callback_gauge_state =
              absl::get<std::unique_ptr<CallbackGaugeState<int64_t>>>(
                  instrument_data.instrument);
          callback_gauge_state->caches.emplace(
              callback, CallbackGaugeState<int64_t>::Cache{});
          if (!std::exchange(callback_gauge_state->ot_callback_registered,
                             true)) {
            callback_gauge_state->instrument->AddCallback(
                &CallbackGaugeCallback<int64_t>, callback_gauge_state.get());
          }
        },
        [&](const grpc_core::GlobalInstrumentsRegistry::
                GlobalCallbackDoubleGaugeHandle& handle) {
          const auto& instrument_data = instruments_data_.at(handle.index);
          if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
            // This instrument is disabled.
            return;
          }
          GPR_ASSERT(absl::holds_alternative<
                     std::unique_ptr<CallbackGaugeState<double>>>(
              instrument_data.instrument));
          auto& callback_gauge_state =
              absl::get<std::unique_ptr<CallbackGaugeState<double>>>(
                  instrument_data.instrument);
          callback_gauge_state->caches.emplace(
              callback, CallbackGaugeState<double>::Cache{});
          if (!std::exchange(callback_gauge_state->ot_callback_registered,
                             true)) {
            callback_gauge_state->instrument->AddCallback(
                &CallbackGaugeCallback<double>, callback_gauge_state.get());
          }
        });
  }
}

void OpenTelemetryPlugin::RemoveCallback(
    grpc_core::RegisteredMetricCallback* callback) {
  grpc_core::MutexLock lock(&mu_);
  callback_timestamps_.erase(callback);
  for (const auto& handle : callback->metrics()) {
    grpc_core::Match(
        handle,
        [&](const grpc_core::GlobalInstrumentsRegistry::
                GlobalCallbackInt64GaugeHandle& handle) {
          const auto& instrument_data = instruments_data_.at(handle.index);
          if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
            // This instrument is disabled.
            return;
          }
          GPR_ASSERT(absl::holds_alternative<
                     std::unique_ptr<CallbackGaugeState<int64_t>>>(
              instrument_data.instrument));
          auto& callback_gauge_state =
              absl::get<std::unique_ptr<CallbackGaugeState<int64_t>>>(
                  instrument_data.instrument);
          GPR_ASSERT(callback_gauge_state->ot_callback_registered);
          GPR_ASSERT(callback_gauge_state->caches.erase(callback) == 1);
          if (callback_gauge_state->caches.empty()) {
            callback_gauge_state->instrument->RemoveCallback(
                &CallbackGaugeCallback<int64_t>, callback_gauge_state.get());
            callback_gauge_state->ot_callback_registered = false;
          }
        },
        [&](const grpc_core::GlobalInstrumentsRegistry::
                GlobalCallbackDoubleGaugeHandle& handle) {
          const auto& instrument_data = instruments_data_.at(handle.index);
          if (absl::holds_alternative<Disabled>(instrument_data.instrument)) {
            // This instrument is disabled.
            return;
          }
          GPR_ASSERT(absl::holds_alternative<
                     std::unique_ptr<CallbackGaugeState<double>>>(
              instrument_data.instrument));
          auto& callback_gauge_state =
              absl::get<std::unique_ptr<CallbackGaugeState<double>>>(
                  instrument_data.instrument);
          GPR_ASSERT(callback_gauge_state->ot_callback_registered);
          GPR_ASSERT(callback_gauge_state->caches.erase(callback) == 1);
          if (callback_gauge_state->caches.empty()) {
            callback_gauge_state->instrument->RemoveCallback(
                &CallbackGaugeCallback<double>, callback_gauge_state.get());
            callback_gauge_state->ot_callback_registered = false;
          }
        });
  }
}

template <typename ValueType>
void OpenTelemetryPlugin::GaugeCallback(
    opentelemetry::metrics::ObserverResult result, void* arg) {
  auto* gauge_state = static_cast<GaugeState<ValueType>*>(arg);
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor(
          {gauge_state->id});
  grpc_core::MutexLock gauge_lock(&gauge_state->mu);
  for (const auto& pair : gauge_state->cache) {
    const auto& label_values = pair.first.first;
    const auto& optional_label_values = pair.first.second;
    GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
    GPR_ASSERT(descriptor.optional_label_keys.size() ==
               optional_label_values.size());
    auto& instrument_data =
        gauge_state->ot_plugin->instruments_data_.at(gauge_state->id);
    opentelemetry::nostd::get<opentelemetry::nostd::shared_ptr<
        opentelemetry::metrics::ObserverResultT<ValueType>>>(result)
        ->Observe(pair.second,
                  NPCMetricsKeyValueIterable(
                      descriptor.label_keys, label_values,
                      descriptor.optional_label_keys, optional_label_values,
                      instrument_data.optional_labels_bits));
  }
}

template <typename ValueType>
void OpenTelemetryPlugin::CallbackGaugeState<ValueType>::Observe(
    opentelemetry::metrics::ObserverResult& result, const Cache& cache) {
  const auto& descriptor =
      grpc_core::GlobalInstrumentsRegistry::GetInstrumentDescriptor({id});
  for (const auto& pair : cache) {
    const auto& label_values = pair.first.first;
    const auto& optional_label_values = pair.first.second;
    GPR_ASSERT(descriptor.label_keys.size() == label_values.size());
    GPR_ASSERT(descriptor.optional_label_keys.size() ==
               optional_label_values.size());
    auto& instrument_data = ot_plugin->instruments_data_.at(id);
    opentelemetry::nostd::get<opentelemetry::nostd::shared_ptr<
        opentelemetry::metrics::ObserverResultT<ValueType>>>(result)
        ->Observe(pair.second,
                  NPCMetricsKeyValueIterable(
                      descriptor.label_keys, label_values,
                      descriptor.optional_label_keys, optional_label_values,
                      instrument_data.optional_labels_bits));
  }
}

template <typename ValueType>
void OpenTelemetryPlugin::CallbackGaugeCallback(
    opentelemetry::metrics::ObserverResult result, void* arg) {
  auto* callback_gauge_state = static_cast<CallbackGaugeState<ValueType>*>(arg);
  auto now = grpc_core::Timestamp::Now();
  grpc_core::MutexLock plugin_lock(&callback_gauge_state->ot_plugin->mu_);
  for (auto& elem : callback_gauge_state->caches) {
    auto* registered_metric_callback = elem.first;
    auto iter = callback_gauge_state->ot_plugin->callback_timestamps_.find(
        registered_metric_callback);
    GPR_ASSERT(iter !=
               callback_gauge_state->ot_plugin->callback_timestamps_.end());
    if (now - iter->second < registered_metric_callback->min_interval()) {
      // Use cached value.
      callback_gauge_state->Observe(result, elem.second);
      continue;
    }
    // Otherwise update and use the cache.
    iter->second = now;
    CallbackMetricReporter reporter(callback_gauge_state->ot_plugin,
                                    registered_metric_callback);
    registered_metric_callback->Run(reporter);
    callback_gauge_state->Observe(result, elem.second);
  }
}

grpc_core::ClientCallTracer* OpenTelemetryPlugin::GetClientCallTracer(
    const grpc_core::Slice& path, bool registered_method,
    std::shared_ptr<grpc_core::StatsPlugin::ScopeConfig> scope_config) {
  return grpc_core::GetContext<grpc_core::Arena>()
      ->ManagedNew<ClientCallTracer>(
          path, grpc_core::GetContext<grpc_core::Arena>(), registered_method,
          this,
          std::static_pointer_cast<OpenTelemetryPlugin::ClientScopeConfig>(
              scope_config));
}

grpc_core::ServerCallTracer* OpenTelemetryPlugin::GetServerCallTracer(
    std::shared_ptr<grpc_core::StatsPlugin::ScopeConfig> scope_config) {
  return grpc_core::GetContext<grpc_core::Arena>()
      ->ManagedNew<ServerCallTracer>(
          this,
          std::static_pointer_cast<OpenTelemetryPlugin::ServerScopeConfig>(
              scope_config));
}

}  // namespace internal

constexpr absl::string_view
    OpenTelemetryPluginBuilder::kClientAttemptStartedInstrumentName;
constexpr absl::string_view
    OpenTelemetryPluginBuilder::kClientAttemptDurationInstrumentName;
constexpr absl::string_view OpenTelemetryPluginBuilder::
    kClientAttemptSentTotalCompressedMessageSizeInstrumentName;
constexpr absl::string_view OpenTelemetryPluginBuilder::
    kClientAttemptRcvdTotalCompressedMessageSizeInstrumentName;
constexpr absl::string_view
    OpenTelemetryPluginBuilder::kServerCallStartedInstrumentName;
constexpr absl::string_view
    OpenTelemetryPluginBuilder::kServerCallDurationInstrumentName;
constexpr absl::string_view OpenTelemetryPluginBuilder::
    kServerCallSentTotalCompressedMessageSizeInstrumentName;
constexpr absl::string_view OpenTelemetryPluginBuilder::
    kServerCallRcvdTotalCompressedMessageSizeInstrumentName;

//
// OpenTelemetryPluginBuilder
//

OpenTelemetryPluginBuilder::OpenTelemetryPluginBuilder()
    : impl_(std::make_unique<internal::OpenTelemetryPluginBuilderImpl>()) {}

OpenTelemetryPluginBuilder::~OpenTelemetryPluginBuilder() = default;

OpenTelemetryPluginBuilder& OpenTelemetryPluginBuilder::SetMeterProvider(
    std::shared_ptr<opentelemetry::metrics::MeterProvider> meter_provider) {
  impl_->SetMeterProvider(std::move(meter_provider));
  return *this;
}

OpenTelemetryPluginBuilder&
OpenTelemetryPluginBuilder::SetTargetAttributeFilter(
    absl::AnyInvocable<bool(absl::string_view /*target*/) const>
        target_attribute_filter) {
  impl_->SetTargetAttributeFilter(std::move(target_attribute_filter));
  return *this;
}

OpenTelemetryPluginBuilder&
OpenTelemetryPluginBuilder::SetGenericMethodAttributeFilter(
    absl::AnyInvocable<bool(absl::string_view /*generic_method*/) const>
        generic_method_attribute_filter) {
  impl_->SetGenericMethodAttributeFilter(
      std::move(generic_method_attribute_filter));
  return *this;
}

OpenTelemetryPluginBuilder& OpenTelemetryPluginBuilder::EnableMetrics(
    absl::Span<const absl::string_view> metric_names) {
  impl_->EnableMetrics(metric_names);
  return *this;
}

OpenTelemetryPluginBuilder& OpenTelemetryPluginBuilder::DisableMetrics(
    absl::Span<const absl::string_view> metric_names) {
  impl_->DisableMetrics(metric_names);
  return *this;
}

OpenTelemetryPluginBuilder& OpenTelemetryPluginBuilder::DisableAllMetrics() {
  impl_->DisableAllMetrics();
  return *this;
}

OpenTelemetryPluginBuilder& OpenTelemetryPluginBuilder::AddPluginOption(
    std::unique_ptr<OpenTelemetryPluginOption> option) {
  impl_->AddPluginOption(
      std::unique_ptr<grpc::internal::InternalOpenTelemetryPluginOption>(
          static_cast<grpc::internal::InternalOpenTelemetryPluginOption*>(
              option.release())));
  return *this;
}

OpenTelemetryPluginBuilder& OpenTelemetryPluginBuilder::AddOptionalLabel(
    absl::string_view optional_label_key) {
  impl_->AddOptionalLabel(optional_label_key);
  return *this;
}

absl::Status OpenTelemetryPluginBuilder::BuildAndRegisterGlobal() {
  return impl_->BuildAndRegisterGlobal();
}

}  // namespace grpc
