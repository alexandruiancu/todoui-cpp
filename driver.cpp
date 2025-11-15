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

#include "jinja2cpp/template.h"

using namespace std;
namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;

//InstrumentLogger glblLogger;

int main(int argc, char *argv[]) {
  ConfigMapT app_config;
  int n_error = init_app_config(app_config);
  if (0 != n_error) {
    return n_error;
  }

  InitLogger();
  InstrumentLogger localLogger;
  
  crow::logger::setHandler(&localLogger);
  crow::logger::setLogLevel(crow::LogLevel::Debug);
  //crow::logger::setLogLevel(crow::LogLevel::Info);
  //crow::logger::setLogLevel(crow::LogLevel::Warning);

  // Create a resource with service name
  auto resource_attributes = opentelemetry::sdk::resource::ResourceAttributes {
    // loop over app_config["OTEL_RESOURCE_ATTRIBUTES"]
    {opentelemetry::semconv::service::kServiceName, "todoui-cpp-service"},
    {opentelemetry::semconv::service::kServiceVersion, "1.0.0"},
    {"service.instance.id", "instance-1"}
  };

  InitTracer(app_config, resource_attributes);

  crow::App<RequestSpan> app;
  jinja2::Template tpl;

  CROW_ROUTE(app, "/")
    .CROW_MIDDLEWARES(app, RequestSpan)
      .methods("GET"_method)([&app_config, &tpl](const crow::request& req){
        std::string span_name(crow::method_name(req.method));
        auto tracer = get_tracer("todoui-cpp-tracer");
        // start active span
        opentelemetry::trace::StartSpanOptions options;
        options.kind = opentelemetry::trace::SpanKind::kClient;  // client
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
                                  {/*{opentelemetry::semconv::url::kUrlFull, url.str()},*/
                                  {opentelemetry::semconv::url::kUrlScheme, "http"/*url_parser.scheme_*/},
                                  {opentelemetry::semconv::http::kHttpRequestMethod, "GET"}},
                                  options);
        auto active_scope = tracer->WithActiveSpan(span);

        // inject context
        auto insert_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
        HttpTextMapCarrier<opentelemetry::ext::http::client::Headers> carrier2;
        propagator->Inject(carrier2, insert_ctx);

        //auto page = crow::mustache::load("index.html");
        tpl.LoadFromFile("templates/index.html");
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
        auto build_elements_v2 = [&](const crow::json::rvalue &json) {
          jinja2::ValuesList v;
          for(auto& t : json) {
              v.emplace_back(t.s());
          }
          return v;
        };

        //crow::mustache::context todos;
        jinja2::ValuesMap todos;
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
        //todos["todos"] = crow::json::wvalue::list(
        //  build_elements(crow::json::load(cpr_resp.text))
        //);

        todos["todos"] = build_elements_v2(crow::json::load(cpr_resp.text));
        span->End();

        //return page.render(todos);
        return tpl.RenderAsString(todos).value().c_str();
      });

  CROW_ROUTE(app, "/add")
    .CROW_MIDDLEWARES(app, RequestSpan)
      .methods("POST"_method)([&app_config](const crow::request& req){
        auto tracer = get_tracer("todoui-cpp-tracer");
        std::string span_name(crow::method_name(req.method));
        // start active span
        opentelemetry::trace::StartSpanOptions options;
        options.kind = opentelemetry::trace::SpanKind::kClient;  // client
        cpr::Url url{
          opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]) + "/todos/" + req.body
        };
      
        CROW_LOG_INFO << std::format("POST  {}/todos/{}", 
          opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]), req.body
        );
      
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
                                  {{opentelemetry::semconv::url::kUrlScheme, "http"},
                                  {opentelemetry::semconv::http::kHttpRequestMethod, "POST"}},
                                  options);
        auto active_scope = tracer->WithActiveSpan(span);
        
        // inject context
        auto insert_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
        HttpTextMapCarrier<opentelemetry::ext::http::client::Headers> carrier2;
        propagator->Inject(carrier2, insert_ctx);
        
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
        crow::query_string qs = req.get_body_params();
        auto cpr_resp = cpr::Post(cpr::Url{
            opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]) + qs.get("todo")
          },
          crp_headers
        );
      
        if (!cpr_resp.error){
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
      
        CROW_LOG_DEBUG << "Add| CPR status code: " 
          << cpr_resp.status_code 
          << " CPR response: " << cpr_resp.text;
      
        span->End();
      
        crow::response resp;
        resp.moved("/");
        return resp;
      });

  CROW_ROUTE(app, "/delete")
    .CROW_MIDDLEWARES(app, RequestSpan)
      .methods("POST"_method)([&app_config](const crow::request& req){
        auto tracer = get_tracer("todoui-cpp-tracer");
        std::string span_name(crow::method_name(req.method));
        // start active span
        opentelemetry::trace::StartSpanOptions options;
        options.kind = opentelemetry::trace::SpanKind::kClient;  // client
      
        CROW_LOG_INFO << std::format("POST  {}/todos/{}",
          opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]), req.body
        );

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
                                  {/*{opentelemetry::semconv::url::kUrlFull, url.str()},*/
                                  {opentelemetry::semconv::url::kUrlScheme, "http"/*url_parser.scheme_*/},
                                  {opentelemetry::semconv::http::kHttpRequestMethod, "POST"}},
                                  options);
        auto active_scope = tracer->WithActiveSpan(span);
        
        // inject context
        auto insert_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
        HttpTextMapCarrier<opentelemetry::ext::http::client::Headers> carrier2;
        propagator->Inject(carrier2, insert_ctx);
        
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
        crow::query_string qs = req.get_body_params();
        auto cpr_resp = cpr::Post(cpr::Url{
            opentelemetry::nostd::get<std::string>(app_config["BACKEND_URL"]) + qs.get("todo")
          },
          crp_headers
        );
      
        if (!cpr_resp.error){
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

  auto add_env_to_config = [&config]<typename T = std::string>(const char *pszName) {
    if (NULL == pszName ) {
      return;
    }
    const char *pszVal = std::getenv(pszName);
    if ( NULL == pszVal) {
      return;
    }
    std::stringstream ss;
    ss << pszVal;
    T tmp;
    ss >> tmp;
    config[pszName] = tmp;
  };

  // app specific
  add_env_to_config.operator()<uint16_t>("FRONTEND_PORT");
  add_env_to_config("BACKEND_URL");
  // OTEL
  add_env_to_config("OTEL_EXPORTER_OTLP_ENDPOINT");
  add_env_to_config("OTEL_RESOURCE_ATTRIBUTES");
  add_env_to_config("OTEL_METRICS_EXPORTER");
  // defaults
  ConfigMapT defaults{
    {"FRONTEND_PORT", (uint16_t)5001},
    {"BACKEND_URL", "http://localhost:8081/todos/"}
  };
  std::for_each(defaults.begin(), defaults.end(), [&config](auto it){
    if(!config.contains(it.first)){
      config.emplace(it);
    }
  });

  return 0;
}
