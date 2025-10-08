#include <cstdlib>
#include <unordered_map>
#include <string>
#include <variant>

#include "crow.h"
#include "cpr/cpr.h"

class LogHandler : public crow::ILogHandler
{
public:
    void log(const std::string& message, crow::LogLevel /*level*/) override
    {
        std::cerr << "LogHandler -> " << message;
    }
};

int main(int argc, char *argv[]) {
  // set defaults
  using ConfigValT = std::variant<std::string, uint32_t>;
  std::unordered_map<std::string, ConfigValT> app_config={
          {"FRONTEND_PORT", (uint32_t)5000},
          {"BACKEND_URL", "http://localhost:8080/todos/"}
  };

  //crow::logger::setLogLevel(crow::LogLevel::Debug);
  //crow::logger::setLogLevel(crow::LogLevel::Info);
  crow::logger::setLogLevel(crow::LogLevel::Warning);

  // update with environment
  {
    const char *pszEnvFrontendPort = std::getenv("BACKEND_PORT");
    if (pszEnvFrontendPort != nullptr)
      app_config["FRONTEND_PORT"] = pszEnvFrontendPort;

    const char *pszEnvBackendURL = std::getenv("BACKEND_URL");
    if (pszEnvBackendURL != nullptr)
      app_config["BACKEND_URL"] = pszEnvBackendURL;
  }

  crow::SimpleApp app;

  CROW_ROUTE(app, "/")
  .methods("GET"_method)([&app_config](const crow::request& req){
      auto page = crow::mustache::load("index.html");
      CROW_LOG_INFO << std::format("GET {}/todos/", 
        std::get<std::string>(app_config["BACKEND_URL"])
      );

      auto build_elements = [&](const crow::json::rvalue &json) {
        std::vector<crow::mustache::context> v;
        for(auto t : json){
          crow::mustache::context c;
          c["todo"] = t;
          v.push_back(std::move(c));
        }
        return v;
      };

      crow::mustache::context todos;
      auto cpr_resp = cpr::Get(cpr::Url{
        std::get<std::string>(app_config["BACKEND_URL"])
      });
      todos["todos"] = crow::json::wvalue::list(
        build_elements(crow::json::load(cpr_resp.text))
      );
      return page.render(todos);
  });

  CROW_ROUTE(app, "/add")
  .methods("POST"_method)([&app_config](const crow::request& req){
    CROW_LOG_INFO << std::format("POST  {}/todos/{}",
        std::get<std::string>(app_config["BACKEND_URL"]), req.body
    );

    crow::query_string qs = req.get_body_params();
    auto cpr_resp = cpr::Post(cpr::Url{
      std::get<std::string>(app_config["BACKEND_URL"]) + qs.get("todo")
    });
    CROW_LOG_DEBUG << "Add| CPR status code: " 
      << cpr_resp.status_code 
      << " CPR response: " << cpr_resp.text;

    crow::response resp;
    resp.moved("/");
    return resp;
  });

  CROW_ROUTE(app, "/delete")
  .methods("POST"_method)([&app_config](const crow::request& req){
    CROW_LOG_INFO << std::format("POST  {}/todos/{}",
      std::get<std::string>(app_config["BACKEND_URL"]), req.body
    );
    
    crow::query_string qs = req.get_body_params();
    auto cpr_resp = cpr::Post(cpr::Url{
      std::get<std::string>(app_config["BACKEND_URL"]) + qs.get("todo")
    });
    CROW_LOG_DEBUG << "Delete| CPR status code: " 
      << cpr_resp.status_code << " CPR response: " << cpr_resp.text;

    crow::response resp;
    resp.moved("/");
    return resp;
  });

  app.port(
    std::get<uint32_t>(app_config["FRONTEND_PORT"])
  ).multithreaded().run();

  return 0;
}
