cmake_minimum_required( VERSION 3.20 )
project( todoui-cpp )

# Place CXX flags here or later ?
#set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# add package manager
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME opentelemetry-cpp
    GITHUB_REPOSITORY open-telemetry/opentelemetry-cpp
    GIT_TAG v1.23.0
    OPTIONS
        "BUILD_SHARED_LIBS ON"
        "WITH_ABSEIL ON"
        "WITH_OTLP_HTTP ON"
        "WITH_OTLP_GRPC ON"
        "WITH_BENCHMARK OFF"
        "WITH_EXAMPLES OFF"
        "WITH_TESTS OFF"
        "OPENTELEMETRY_INSTALL ON"
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

## (a)
#CPMAddPackage("gh:jinja2cpp/Jinja2Cpp#1.3.2")
## (b)
#add_subdirectory(Jinja2Cpp)
## (c)
#find_package(Jinja2Cpp REQUIRED)

CPMAddPackage(
  NAME jinja2cpp
#  GITHUB_REPOSITORY jinja2cpp/Jinja2Cpp
  GITHUB_REPOSITORY alexandruiancu/Jinja2Cpp
#  GIT_TAG 1.3.2
  GIT_TAG fix-thirdparty-internal-deps
  OPTIONS
      "JINJA2CPP_WITH_JSON_BINDINGS=rapid"
  )

#CPMAddPackage("gh:kroketio/Jinja2CppMedium")
#CPMAddPackage(
#  NAME jinja2cpp_medium
#  GITHUB_REPOSITORY git@github.com:kroketio/Jinja2CppMedium.git
##  OPTIONS
##      "JINJA2CPP_DEPS_MODE=internal"
##      "JINJA2CPP_BUILD_SHARED=OFF"
#  )

set(include_DIR "${todoui-cpp_SOURCE_DIR}/include")
add_executable(todoui-cpp "driver.cpp")

target_include_directories (todoui-cpp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(todoui-cpp PRIVATE 
    # opentelemetry targets
    opentelemetry-cpp::api
    opentelemetry-cpp::sdk
    opentelemetry-cpp::ostream_span_exporter
    opentelemetry-cpp::otlp_http_exporter
    opentelemetry-cpp::otlp_grpc_exporter
    # spdlog targets
    spdlog::spdlog_header_only

    cpr::cpr
    # Crow targets
    Crow::Crow
    ## jinja2
    jinja2cpp
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
install(TARGETS 
    todoui-cpp 
    cpr 
    jinja2cpp 
    libcurl_shared 

    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Optional: Install any additional files (configs, resources, etc.)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/templates
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
