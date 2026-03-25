cmake_minimum_required(VERSION 3.16)

project(HelloAgents VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)

# Header-only aggregation target for current include layout.
add_library(hello_agents_headers INTERFACE)
target_include_directories(hello_agents_headers
	INTERFACE
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/libs/include
)

# Chapter 01 sample/test executable.
add_executable(ch01
	tests/ch01.cpp
)

target_link_libraries(ch01
	PRIVATE
		hello_agents_headers
		CURL::libcurl
)

enable_testing()
add_test(NAME ch01_no_args COMMAND ch01)
set_tests_properties(ch01_no_args PROPERTIES WILL_FAIL TRUE)
