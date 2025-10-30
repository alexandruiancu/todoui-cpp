
# SPDX-License-Identifier: Apache-2.0

# Import spdlog targets
# 1. Find an installed spdlog package
# 2. Use FetchContent to fetch and build Crow (and its submodules) from GitHub
find_package(spdlog QUIET)
set(spdlog_PROVIDER "find_package")

if(NOT spdlog_FOUND)
    message(STATUS "spdlog not found, fetching from GitHub...")

    include(FetchContent)
    include(CMakeFindDependencyMacro)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
	#        GIT_TAG        ${spdlog_GIT_TAG} # Specify the version you want
        GIT_TAG        v1.16.0
    )
    set(spdlog_PROVIDER "fetch_repository")
    FetchContent_MakeAvailable(spdlog)

    message(STATUS "spdlog fetched successfully")
else()
    message(STATUS "spdlog found: ${spdlog_VERSION}")
endif()

#if(NOT TARGET spdlog::spdlog)
if(NOT TARGET spdlog::spdlog_header_only)
  message(FATAL_ERROR "A required spdlog target ( spdlog::spdlog or stdlog::spdlog_header_only ) was not imported")
endif()
