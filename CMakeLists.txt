# CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(shared_ptr CXX)
set(SOURCE_FILES ${PROJECT_SOURCE_DIR}/shared_ptr_test.cpp)
add_executable(shared_ptr ${SOURCE_FILES})
target_compile_features(shared_ptr PRIVATE cxx_std_20)
include_directories(${PROJECT_SOURCE_DIR})
