cmake_minimum_required( VERSION 3.20 )
project( todoui-cpp )

# Place CXX flags here or later ?
#set( CMAKE_CXX_FLAGS "-g3 -Wall" ) - not sure this is the right place
set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# include("${instrumentation-playground_SOURCE_DIR}/cmake/curl.cmake")
include("${todoui-cpp_SOURCE_DIR}/cmake/crow.cmake")
include("${todoui-cpp_SOURCE_DIR}/cmake/cpr.cmake")

add_executable(todoui-cpp "driver.cpp")
target_link_libraries(todoui-cpp PRIVATE Crow::Crow cpr::cpr)
