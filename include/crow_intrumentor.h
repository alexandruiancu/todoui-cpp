// need to read few otel SDK env variables probably
// see: https://opentelemetry.io/docs/languages/sdk-configuration/general/
//  OTEL_RESOURCE_ATTRIBUTES=service.name=todoui-flask
//  OTEL_METRICS_EXPORTER=otlp

#include "crow.h"
#include "tracer_common.h"

struct RequestSpan : crow::ILocalMiddleware
{
    // Values from this context can be accessed from handlers
    struct context
    {
        void StartSpan(crow::request& req, crow::response& res){
            tracer = get_tracer("todoui-cpp-service");
            // start internal or server span
            opentelemetry::trace::StartSpanOptions options;
            options.kind = opentelemetry::trace::SpanKind::kServer;
            span = tracer->StartSpan(req.url, 
              {/*{std::make_pair(opentelemetry::semconv::url::kUrlFull, "http://localhost:5000")},*/
              {std::make_pair(opentelemetry::semconv::url::kUrlScheme, "http")},
              {std::make_pair(opentelemetry::semconv::http::kHttpRequestMethod, crow::method_name(req.method))}},
              options);
            auto active_scope = tracer->WithActiveSpan(span);

            // inject context
            auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
            opentelemetry::ext::http::client::Headers request_headers;
            for ( auto p : req.headers){
                request_headers.insert({p.first, p.second});
            }
            // get global propagator
            HttpTextMapCarrier<opentelemetry::ext::http::client::Headers> carrier(request_headers);
            auto propagator = opentelemetry::context::propagation::GlobalTextMapPropagator::GetGlobalPropagator();
            propagator->Inject(carrier, current_ctx);

            span->SetAttribute(opentelemetry::semconv::http::kHttpResponseStatusCode, res.code);
            for(auto h : carrier.headers_){
                span->SetAttribute("http.header." + std::string(h.first), h.second);
            }
            req.headers.insert(std::make_pair(opentelemetry::trace::propagation::kTraceParent.data(), 
                carrier.Get(opentelemetry::trace::propagation::kTraceParent).data()));
        }
        void EndSpan() {
            span->End();
        }
        std::string tracer_name = "crow_instrumentor";
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> tracer;
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> span;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        ctx.StartSpan(req, res);
    }

    void after_handle(crow::request& /*req*/, crow::response& res, context& ctx) {
        ctx.EndSpan();
    }
};
