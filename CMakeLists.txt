cmake_minimum_required( VERSION 3.20 )
project( todoui-cpp )

# Place CXX flags here or later ?
#set( CMAKE_CXX_FLAGS "-g3 -Wall" ) - not sure this is the right place
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

include("${todoui-cpp_SOURCE_DIR}/cmake/opentelemetry-cpp.cmake")
include("${todoui-cpp_SOURCE_DIR}/cmake/crow.cmake")
include("${todoui-cpp_SOURCE_DIR}/cmake/cpr.cmake")

set(include_DIR "${todoui-cpp_SOURCE_DIR}/include")
add_executable(todoui-cpp "driver.cpp")

 target_include_directories (todoui-cpp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(todoui-cpp PRIVATE 
    Crow::Crow cpr::cpr 
    opentelemetry-cpp::ostream_span_exporter
    opentelemetry-cpp::api
    opentelemetry-cpp::sdk
    opentelemetry-cpp::otlp_http_exporter
    opentelemetry-cpp::otlp_grpc_exporter)
