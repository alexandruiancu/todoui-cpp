cmake_minimum_required( VERSION 3.20 )
project( todoui-cpp )

# Place CXX flags here or later ?
#set( CMAKE_CXX_FLAGS "-g3 -Wall" ) - not sure this is the right place
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# add package manager
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME opentelemetry-cpp
    GITHUB_REPOSITORY open-telemetry/opentelemetry-cpp
    GIT_TAG v1.23.0
    OPTIONS
        "WITH_ABSEIL ON"
        "WITH_OTLP_HTTP ON"
        "WITH_OTLP_GRPC ON"
        "WITH_BENCHMARK OFF"
        "WITH_EXAMPLES OFF"
        "WITH_TESTS OFF"
)
CPMAddPackage("gh:crowcpp/crow@1.3.0")
CPMAddPackage("gh:libcpr/cpr#1.12.0")

set(include_DIR "${todoui-cpp_SOURCE_DIR}/include")
add_executable(todoui-cpp "driver.cpp")

target_include_directories (todoui-cpp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(todoui-cpp PRIVATE 
    Crow::Crow cpr::cpr
    opentelemetry-cpp::api
    opentelemetry-cpp::sdk
    opentelemetry-cpp::ostream_span_exporter
    opentelemetry-cpp::otlp_http_exporter
    opentelemetry-cpp::otlp_grpc_exporter)
