cmake_minimum_required(VERSION 3.16.0)
project(metis LANGUAGES CXX)

include(cmake/CPM.cmake)

CPMAddPackage(
	NAME util
	GITHUB_REPOSITORY krox/util
	GIT_TAG dbc2b40084b8940dfeaa780375de818de5a8fbc0
)

CPMAddPackage(
	NAME cli11
	GITHUB_REPOSITORY CLIUtils/CLI11
	GIT_TAG 291c58789c031208f08f4f261a858b5b7083e8e2 # v2.3.2
)

CPMAddPackage(
	NAME Catch2
	GITHUB_REPOSITORY catchorg/Catch2
	GIT_TAG 2ab20a0e008845e02bd06248e61ca6e5ad1aba33 # v3.3.1
)

file(GLOB files_cpp "src/metis/*.cpp")
add_executable(metis "src/main.cpp" ${files_cpp})

target_compile_features(metis PUBLIC cxx_std_20)
target_include_directories(metis PUBLIC src)
target_link_libraries(metis PUBLIC util CLI11::CLI11)
target_compile_options(metis PUBLIC -Wall -Wextra -Werror -g -march=native)

# unittests
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	add_executable(tests tests/movegen.cpp ${files_cpp})
	target_compile_features(tests PRIVATE cxx_std_17)
	target_link_libraries(tests PUBLIC Catch2::Catch2WithMain util CLI11::CLI11)
	target_include_directories(tests PUBLIC src)
	target_compile_options(tests PUBLIC -Wall -Wextra -Werror -g -march=native)
endif()
