// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"

#include "opentelemetry/exporters/ostream/span_exporter.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"

#include "opentelemetry/exporters/otlp/otlp_http.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"

#include "opentelemetry/exporters/otlp/otlp_grpc_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"

#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/provider.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_context.h"
#include "opentelemetry/sdk/trace/tracer_context_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include <opentelemetry/sdk/resource/resource.h>

#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"

#include <grpcpp/grpcpp.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <concepts>

using grpc::ClientContext;
using grpc::ServerContext;
namespace trace_sdk = opentelemetry::sdk::trace;

const uint16_t nDefaultGrpcCollector = 4317;
const uint16_t nDefaultHttpCollector = 4318;

namespace
{
  template <typename T>
  class HttpTextMapCarrier : public opentelemetry::context::propagation::TextMapCarrier
  {
  public:
    HttpTextMapCarrier(T &headers) : headers_(headers) {}
    HttpTextMapCarrier() = default;
    virtual opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view key) const noexcept override
    {
      std::string key_to_compare = key.data();
      // Header's first letter seems to be  automatically capitaliazed by our test http-server, so
      // compare accordingly.
      if (key == opentelemetry::trace::propagation::kTraceParent)
      {
        key_to_compare = "Traceparent";
      }
      else if (key == opentelemetry::trace::propagation::kTraceState)
      {
        key_to_compare = "Tracestate";
      }
      auto it = headers_.find(key_to_compare);
      if (it != headers_.end())
      {
        return it->second;
      }
      return "";
    }
  
    virtual void Set(opentelemetry::nostd::string_view key,
                     opentelemetry::nostd::string_view value) noexcept override
    {
      headers_.insert(std::pair<std::string, std::string>(std::string(key), std::string(value)));
    }
  
    T headers_;
  };
  
  class GrpcClientCarrier : public opentelemetry::context::propagation::TextMapCarrier
  {
  public:
    GrpcClientCarrier(ClientContext *context) : context_(context) {}
    GrpcClientCarrier() = default;
    virtual opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view /* key */) const noexcept override
    {
      return "";
    }
  
    virtual void Set(opentelemetry::nostd::string_view key,
                     opentelemetry::nostd::string_view value) noexcept override
    {
      std::cout << " Client ::: Adding " << key << " " << value << "\n";
      context_->AddMetadata(std::string(key), std::string(value));
    }
  
    ClientContext *context_ = nullptr;
  };
  
  class GrpcServerCarrier : public opentelemetry::context::propagation::TextMapCarrier
  {
  public:
    GrpcServerCarrier(ServerContext *context) : context_(context) {}
    GrpcServerCarrier() = default;
    virtual opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view key) const noexcept override
    {
      auto it = context_->client_metadata().find({key.data(), key.size()});
      if (it != context_->client_metadata().end())
      {
        return opentelemetry::nostd::string_view(it->second.data(), it->second.size());
      }
      return "";
    }
  
    virtual void Set(opentelemetry::nostd::string_view /* key */,
                     opentelemetry::nostd::string_view /* value */) noexcept override
    {
      // Not required for server
    }
  
    ServerContext *context_ = nullptr;
  };

//auto build_console_processor()
//{
//  auto exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create();
//  auto processor =
//      trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
//  return processor;
//}

  template<typename ExporterT, typename ExporterOptionsT>
  auto build_processor(uint16_t port = 0) {
  
    auto exporter_fnc = []<typename T>(uint16_t port) {
      if constexpr(std::same_as<T, opentelemetry::exporter::otlp::OtlpHttpExporterOptions>) {
        if ( 0 == port ) {
          port = nDefaultHttpCollector;
        }
  
        opentelemetry::exporter::otlp::OtlpHttpExporterOptions opts;
        opts.url                             = "http://localhost:" + std::to_string(port) + "/v1/traces";
        opts.retry_policy_max_attempts       = 5;
        opts.retry_policy_initial_backoff    = std::chrono::duration<float>{0.1f};
        opts.retry_policy_max_backoff        = std::chrono::duration<float>{5.0f};
        opts.retry_policy_backoff_multiplier = 1.0f;
        auto exporter = opentelemetry::exporter::otlp::OtlpHttpExporterFactory::Create(opts);
  
        return exporter;
      } else if constexpr(std::same_as<T, opentelemetry::exporter::otlp::OtlpGrpcExporterOptions>) {
        if ( 0 == port ) {
          port = nDefaultGrpcCollector;
        }
  
        opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
        opts.endpoint                           = "localhost:" + std::to_string(port);
        opts.use_ssl_credentials                = false;
        //opts.use_ssl_credentials              = true;
        //opts.ssl_credentials_cacert_as_string = "ssl-certificate";
        auto exporter = opentelemetry::exporter::otlp::OtlpGrpcExporterFactory::Create(opts);
  
        return exporter;
      }
    };
  
    auto processor_fnc = [&]<typename T>() {
      if constexpr(std::same_as<T, opentelemetry::exporter::trace::OStreamSpanExporter>) {
        auto exporter = opentelemetry::exporter::trace::OStreamSpanExporterFactory::Create();
        auto processor =
            trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
  
        return processor;
      } else if constexpr(std::same_as<T, opentelemetry::exporter::otlp::OtlpHttpExporter> ||
          std::same_as<T, opentelemetry::exporter::otlp::OtlpGrpcExporter>) {
  
          auto exporter = exporter_fnc.template operator()<ExporterOptionsT>(port);
          trace_sdk::BatchSpanProcessorOptions bspOpts;
          auto processor = trace_sdk::BatchSpanProcessorFactory::Create(std::move(exporter), bspOpts);
  
          return processor;
      }
    };
  
    return processor_fnc.template operator()<ExporterT>();
  }
  
  void InitTracer() {
    // Create a resource with service name
    auto resource_attributes = opentelemetry::sdk::resource::ResourceAttributes {
        {"service.name", "todoui-cpp-service"},
        {"service.version", "1.0.0"},
        {"service.instance.id", "instance-1"}
    };
    
    auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);
  
    std::vector<std::unique_ptr<trace_sdk::SpanProcessor>> processors;
    //processors.push_back(
    //  std::move(build_processor<opentelemetry::exporter::trace::OStreamSpanExporter, void>()));
    //processors.push_back(
    //  std::move(build_processor<opentelemetry::exporter::otlp::OtlpHttpExporter, 
    //            opentelemetry::exporter::otlp::OtlpHttpExporterOptions>()
    //            ));
    processors.push_back(
      std::move(build_processor<opentelemetry::exporter::otlp::OtlpGrpcExporter, 
                opentelemetry::exporter::otlp::OtlpGrpcExporterOptions>()
                ));
  
    std::shared_ptr<opentelemetry::trace::TracerProvider> provider = 
      trace_sdk::TracerProviderFactory::Create(
        std::move(processors),
        resource
      );
    opentelemetry::trace::Provider::SetTracerProvider(provider);
  }
  
  void CleanupTracer()
  {
    std::shared_ptr<opentelemetry::trace::TracerProvider> none;
    trace_sdk::Provider::SetTracerProvider(none);
  }
  
  opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> get_tracer(std::string tracer_name)
  {
    auto provider = opentelemetry::trace::Provider::GetTracerProvider();
    return provider->GetTracer(tracer_name);
  }
  
}  // namespace
