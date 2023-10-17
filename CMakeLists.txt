cmake_minimum_required(VERSION 3.16.0)
project(metis LANGUAGES CXX)

include(cmake/CPM.cmake)

CPMAddPackage(
	NAME util
	GITHUB_REPOSITORY krox/util
	GIT_TAG 9b6b0560d4edb3c8e50d10f7b2ed184e512407dc
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

CPMAddPackage(
	NAME Eigen
	GITLAB_REPOSITORY libeigen/eigen
	GIT_TAG 764b132a79aaf33e958169a14a011b2e0eec1400 # v3.4.1
	OPTIONS "BUILD_TESTING OFF"
)

find_package(OpenMP REQUIRED)

file(GLOB files_cpp "src/metis/*.cpp")
add_executable(metis "src/main.cpp" ${files_cpp})

target_compile_features(metis PUBLIC cxx_std_20)
target_include_directories(metis PUBLIC src)
target_link_libraries(metis PUBLIC util CLI11::CLI11 Eigen3::Eigen OpenMP::OpenMP_CXX)
target_compile_options(metis PUBLIC -Wall -Wextra -Werror -g -march=native)

# unittests
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	add_executable(tests tests/movegen.cpp ${files_cpp})
	target_compile_features(tests PRIVATE cxx_std_17)
	target_link_libraries(tests PUBLIC Catch2::Catch2WithMain util CLI11::CLI11 Eigen3::Eigen OpenMP::OpenMP_CXX)
	target_include_directories(tests PUBLIC src)
	target_compile_options(tests PUBLIC -Wall -Wextra -Werror -g -march=native)
endif()
