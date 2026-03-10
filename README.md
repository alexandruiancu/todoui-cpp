# todo UI cpp
C++ UI reimplementation of [LFS148](https://github.com/lftraining/LFS148-code)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/alexandruiancu/todoui-cpp)
## Building:
Usual CMake recipe in CMakeLists.no-cpm.txt
A cleaner/simplier recipe using CMake dependency management
## Adding CPM:
wget -O cmake/CPM.cmake https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
## Traces go to signoz ( instead of Jaeger )
![alt text](https://github.com/alexandruiancu/todoui-cpp/blob/main/todo_trace_to_signoz.png?raw=true)
## dependencies
meson, OpenSSL, zlib, boost asio
