#include <cstdlib>
#include <string>
#include <ctime>

#include "crow.h"
#include "cpr/cpr.h"

#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/sdk/trace/exporter.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"
//http sem conv
#include "opentelemetry/semconv/url_attributes.h"
#include "opentelemetry/semconv/http_attributes.h"
//service sem conv
#include "opentelemetry/semconv/service_attributes.h"

#include "instrument_logger.h"
#include "tracer_common.h"
#include "crow_intrumentor.h"
#include "utils.h"

using namespace std;
namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;

InstrumentLogger glblLogger;

int main(int argc, char *argv[]) {
  ConfigMapT app_config;
  int n_error = init_app_config(app_config);
  if (0 != n_error) {
    return n_error;
  }

  crow::logger::setHandler(&glblLogger);
  //crow::logger::setLogLevel(crow::LogLevel::Debug);
  //crow::logger::setLogLevel(crow::LogLevel::Info);
  crow::logger::setLogLevel(crow::LogLevel::Warning);

  // Create a resource with service name
  auto resource_attributes = opentelemetry::sdk::resource::ResourceAttributes {
    // loop over app_config["OTEL_RESOURCE_ATTRIBUTES"]
    {opentelemetry::semconv::service::kServiceName, "todoui-cpp-service"},
    {opentelemetry::semconv::service::kServiceVersion, "1.0.0"},
    {"service.instance.id", "instance-1"}
  };

  n_error = init_attributes(app_config, resource_attributes);
  if (0 != n_error) {
    return n_error;
  }

  InitTracer(app_config, resource_attributes);

  crow::App<RequestSpan> app;

  CROW_ROUTE(app, "/")
    .CROW_MIDDLEWARES(app, RequestSpan)
      .methods("GET"_method)([&app_config](const crow::request& req){
        std::string span_name(crow::method_name(req.method));
        auto tracer = get_tracer("todoui-cpp-tracer");
        // start active span
        opentelemetry::trace::StartSpanOptions options;
        options.kind = opentelemetry::trace::SpanKind::kClient;  // client
        //opentelemetry::ext::http::common::UrlParser url_parser(
        //  std::get<std::string>(app_config["BACKEND_URL"])
        //);
        cpr::Url url{
          opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"])
        };
        
        // extract context from http header
        auto extract_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
        opentelemetry::ext::http::client::Headers request_headers;
        for ( auto p : req.headers){
          request_headers.insert({p.first, p.second});
        }
        // get global propagator
        HttpTextMapCarrier<opentelemetry::ext::http::client::Headers> carrier(request_headers);
        auto propagator = opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
        auto new_ctx = propagator->Extract(carrier, extract_ctx);
        options.parent = opentelemetry::trace::GetSpan(new_ctx)->GetContext();
        auto span = tracer->StartSpan(span_name,
                                  {{opentelemetry::semconv::url::kUrlFull, url.str()},
                                  {opentelemetry::semconv::url::kUrlScheme, "http"/*url_parser.scheme_*/},
                                  {opentelemetry::semconv::http::kHttpRequestMethod, "GET"}},
                                  options);
        auto active_scope = tracer->WithActiveSpan(span);

        // inject context
        auto insert_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
        HttpTextMapCarrier<opentelemetry::ext::http::client::Headers> carrier2;
        propagator->Inject(carrier2, insert_ctx);

        auto page = crow::mustache::load("index.html");
        //CROW_LOG_INFO << url_parser.url_;
        CROW_LOG_INFO << url;

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
        /////////////////////////////////////////
        // TODO
        // problemtatic conversion from opentelemetry headers into cpr's
        // std::mutimap -> std::map
        cpr::Header crp_headers;
        //for (auto p : carrier.headers_){
        for (auto p : carrier2.headers_){
          crp_headers.insert(p);
        }
        /////////////////////////////////////////
        auto cpr_resp = cpr::Get(url, crp_headers);
        if(!cpr_resp.error) {
          span->SetAttribute(opentelemetry::semconv::http::kHttpResponseStatusCode, cpr_resp.status_code);
          for(auto h : cpr_resp.header){
            span->SetAttribute("http.header." + std::string(h.first), h.second);
          }
          if (cpr_resp.status_code >= 400) {
            span->SetStatus(opentelemetry::trace::StatusCode::kError);
          }
        } else {
          span->SetStatus(
            opentelemetry::trace::StatusCode::kError,
            "Response Status :" + cpr_resp.status_line);
        }
        todos["todos"] = crow::json::wvalue::list(
          build_elements(crow::json::load(cpr_resp.text))
        );
        span->End();

        return page.render(todos);
      });

  CROW_ROUTE(app, "/add")
  .methods("POST"_method)([&app_config](const crow::request& req){
    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("todoui-cpp-tracer");
    auto span = tracer->StartSpan("AddTodo");

    CROW_LOG_INFO << std::format("POST  {}/todos/{}", 
      opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]), req.body
    );

    crow::query_string qs = req.get_body_params();
    auto cpr_resp = cpr::Post(cpr::Url{
      opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]) + qs.get("todo")
    });
    CROW_LOG_DEBUG << "Add| CPR status code: " 
      << cpr_resp.status_code 
      << " CPR response: " << cpr_resp.text;

    span->End();

    crow::response resp;
    resp.moved("/");
    return resp;
  });

  CROW_ROUTE(app, "/delete")
  .methods("POST"_method)([&app_config](const crow::request& req){
    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("todoui-cpp-tracer");
    auto span = tracer->StartSpan("DeleteTodo");

    CROW_LOG_INFO << std::format("POST  {}/todos/{}",
      opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]), req.body
    );
    
    crow::query_string qs = req.get_body_params();
    auto cpr_resp = cpr::Post(cpr::Url{
      opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]) + qs.get("todo")
    });
    CROW_LOG_DEBUG << "Delete| CPR status code: " 
      << cpr_resp.status_code << " CPR response: " << cpr_resp.text;

    span->End();

    crow::response resp;
    resp.moved("/");
    return resp;
  });

  app.port(
    opentelemetry::nostd::get<uint16_t>(app_config["FRONTEND_PORT"])
  ).multithreaded().run();

  CleanupTracer();
  return 0;
}

int init_app_config(ConfigMapT &config){
  // app specific
  const char *pszEnvFrontendPort = std::getenv("BACKEND_PORT");
  if (pszEnvFrontendPort != nullptr)
    config["FRONTEND_PORT"] = (uint16_t)(std::stoul(pszEnvFrontendPort));
  const char *pszEnvBackendURL = std::getenv("BACKEND_URL");
  if (pszEnvBackendURL != nullptr)
    config["BACKEND_URL"] = pszEnvBackendURL;

  // OTEL
  const char *pszEnv = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
  if (pszEnv != nullptr)
    config["OTEL_EXPORTER_OTLP_ENDPOINT"] = pszEnv;
  pszEnv = std::getenv("OTEL_RESOURCE_ATTRIBUTES");
  if (pszEnv != nullptr)
    config["OTEL_RESOURCE_ATTRIBUTES"] = pszEnv;
  pszEnv = std::getenv("OTEL_METRICS_EXPORTER");
  if (pszEnv != nullptr)
    config["OTEL_METRICS_EXPORTER"] = pszEnv;

  ConfigMapT defaults{
    {"FRONTEND_PORT", (uint16_t)5000},
    {"BACKEND_URL", "http://localhost:8080/todos/"}
  };
  std::for_each(defaults.begin(), defaults.end(), [&config](auto it){
    if(!config.contains(it.first)){
      config.emplace(it);
    }
  });

  return 0;
}

int init_attributes(const ConfigMapT &config, opentelemetry::sdk::resource::ResourceAttributes &ra){
  //TODO implement
  return 0;
}
