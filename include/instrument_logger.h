#include "crow/logging.h"
#include "spdlog/spdlog.h"

class InstrumentLogger : public crow::ILogHandler {
 public:
  InstrumentLogger() {}
  void log(const std::string& message, crow::LogLevel /*level*/) {
    // "message" doesn't contain the timestamp and loglevel
    // prefix the default logger does and it doesn't end
    // in a newline.
    std::cerr << message << std::endl;
  }
};