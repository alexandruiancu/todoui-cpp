#include "crow/logging.h"
#include "spdlog/spdlog.h"

// opentelemetry
#include "opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_factory.h"

#include "opentelemetry/logs/logger.h"
#include "opentelemetry/logs/logger_provider.h"
#include "opentelemetry/logs/provider.h"

#include "opentelemetry/sdk/logs/exporter.h"
#include "opentelemetry/sdk/logs/logger_provider.h"
#include "opentelemetry/sdk/logs/logger_provider_factory.h"
#include "opentelemetry/sdk/logs/processor.h"
#include "opentelemetry/sdk/logs/provider.h"
#include "opentelemetry/sdk/logs/simple_log_record_processor_factory.h"


void InitLogger()
{
  // Create ostream log exporter instance
  auto exporter =
      std::unique_ptr<opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporter>(
        new opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporter
      );
  auto processor = opentelemetry::sdk::logs::SimpleLogRecordProcessorFactory::Create(
    std::move(exporter)
  );

  std::shared_ptr<opentelemetry::sdk::logs::LoggerProvider> sdk_provider(
      opentelemetry::sdk::logs::LoggerProviderFactory::Create(
        std::move(processor)
      )
    );

  // Set the global logger provider
  const std::shared_ptr<opentelemetry::logs::LoggerProvider> &api_provider = sdk_provider;
  opentelemetry::sdk::logs::Provider::SetLoggerProvider(api_provider);
}

void CleanupLogger()
{
  std::shared_ptr<opentelemetry::logs::LoggerProvider> noop;
  opentelemetry::sdk::logs::Provider::SetLoggerProvider(noop);
}

class InstrumentLogger : public crow::ILogHandler {

  opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> logger;

  public:
    InstrumentLogger() {
      auto provider = opentelemetry::logs::Provider::GetLoggerProvider();
      logger = provider->GetLogger("todoui-cpp_logger", "todoui-cpp");
    }

    void log(const std::string& message, crow::LogLevel level) {
      // "message" doesn't contain the timestamp and loglevel
      // prefix the default logger does and it doesn't end
      // in a newline.
      //std::cerr << message << std::endl;

      switch (level){
        case crow::LogLevel::Debug:
          logger->Debug(message);
          break;
        case crow::LogLevel::Info:
          logger->Info(message);
          break;
        case crow::LogLevel::Warning:
          logger->Warn(message);
          break;
        case crow::LogLevel::Error:
          logger->Error(message);
          break;
        case crow::LogLevel::Critical:
          logger->Fatal(message);
          break;
      }
    }
};