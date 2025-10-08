# Copyright The OpenTelemetry Authors
# SPDX-License-Identifier: Apache-2.0

# Import CPR targets
# 1. Find an installed CPR package
# 2. Use FetchContent to fetch and build CPR (and its submodules) from GitHub

# Including the CMakeFindDependencyMacro resolves an error from
include(CMakeFindDependencyMacro)

find_package(cpr::cpr CONFIG QUIET)
set(cpr_PROVIDER "find_package")

if(NOT cpr_FOUND)
  FetchContent_Declare(
      "cpr"
      GIT_REPOSITORY  "https://github.com/libcpr/cpr.git"
      GIT_TAG "${cpr_GIT_TAG}"
      )
  set(cpr_PROVIDER "fetch_repository")

  FetchContent_MakeAvailable(cpr)

  # Set the cpr_VERSION variable from the git tag.
  string(REGEX REPLACE "^v([0-9]+\\.[0-9]+\\.[0-9]+)$" "\\1" cpr_VERSION "${cpr_GIT_TAG}")

endif()

if(NOT TARGET cpr::cpr)
	message(FATAL_ERROR "A required CPR target (cpr::cpr) was not imported")
endif()
