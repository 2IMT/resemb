# Copyright (c) 2025 2IMT
# Licensed under the MIT License.
# See LICENSE file in the project root for details.

cmake_minimum_required(VERSION 3.12)

project(resemb VERSION 0.1.0 LANGUAGES C)

set(RESEMB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(RESEMB_GEN_EXECUTABLE resemb_gen)

add_executable(resemb_gen "src/resemb_gen.c")
set_target_properties(resemb_gen PROPERTIES
    C_STANDARD 99
)
if (MSVC)
    target_compile_options(resemb_gen PRIVATE /W4 /permissive-)
else()
    target_compile_options(resemb_gen PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(resemb INTERFACE)
target_include_directories(resemb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
add_library(resemb::resemb ALIAS resemb)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/resembFunctions.cmake)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
  TARGETS resemb resemb_gen
  EXPORT resembTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY "include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT resembTargets
  NAMESPACE resemb::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/resemb
)

set(PACKAGE_BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
configure_package_config_file(
  cmake/resembConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/resembConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/resemb
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/resembConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/resembConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/resembConfigVersion.cmake"
  cmake/resembFunctions.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/resemb
)
