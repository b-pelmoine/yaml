cmake_minimum_required(VERSION 3.28)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(YAML_MASTER_PROJECT ON)
    message(STATUS "CMake version: ${CMAKE_VERSION}")
endif ()

option(YAML_BUILD_TESTING "build unit tests" YAML_MASTER_PROJECT)

if (YAML_BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project (yaml_cpp 
	LANGUAGES CXX
	VERSION 0.1.0
)

add_subdirectory(yaml_library)

if (YAML_BUILD_TESTING)
    add_subdirectory(test)
endif()