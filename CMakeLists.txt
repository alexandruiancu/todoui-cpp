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
CPMAddPackage(
    NAME crow
    GITHUB_REPOSITORY crowcpp/crow
    GIT_TAG v1.3.0
    OPTIONS
        "CROW_BUILD_EXAMPLES=OFF"
        "CROW_BUILD_TESTS=OFF"
)
CPMAddPackage("gh:libcpr/cpr#1.12.0")
CPMAddPackage("gh:gabime/spdlog@1.16.0")

set(include_DIR "${todoui-cpp_SOURCE_DIR}/include")
add_executable(todoui-cpp "driver.cpp")

target_include_directories (todoui-cpp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(todoui-cpp PRIVATE 
    cpr::cpr
    # Crow targets
    Crow::Crow
    # opentelemetry targets
    opentelemetry-cpp::api
    opentelemetry-cpp::sdk
    opentelemetry-cpp::ostream_span_exporter
    opentelemetry-cpp::otlp_http_exporter
    opentelemetry-cpp::otlp_grpc_exporter
    # spdlog targets
    spdlog::spdlog_header_only
)

add_custom_command(TARGET todoui-cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/templates
    $<TARGET_FILE_DIR:todoui-cpp>/templates
    COMMENT "useful when running without install, from build directory"
)

# Set the install prefix
# use command line -DCMAKE_INSTALL_PREFIX=${workspaceFolder}/build/opt to set prefix

# Install the dependency's targets if they are exported
# Note: This assumes the dependency properly exports its targets
install(TARGETS todoui-cpp cpr 
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include/
)

# Optional: Install any additional files (configs, resources, etc.)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/templates
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)