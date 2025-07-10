cmake_minimum_required(VERSION 3.28)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

#The project name is set here. In this case it's Rocket, but you can change it to whatever you want.
project(ZPMemoryPool  VERSION 1.0.0 LANGUAGES CXX)

#Require C++23
set(CXX_STANDARD_REQUIRED ON) #Make C++23 a hard requirement
set(CMAKE_CXX_STANDARD 23) # Default C++ standard for targets

# 指定源文件目录下的所有 .cpp 文件
file(GLOB SOURCES "*.cc")


find_package(fmt CONFIG REQUIRED)
find_package(gtest CONFIG REQUIRED)

#We're using the project name as the target name, but you can change it to make them different.
#With this setup, the name of the executable will be the same as the project name.
add_executable(${PROJECT_NAME} ${SOURCES})

# Create a library for the memory pool code
add_library(ZPMemoryPoolLib version1/ZPmemoryPool.cc)
target_include_directories(ZPMemoryPoolLib PUBLIC version1)

# Create test executable
add_executable(tests test/test_main.cc)
target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main ZPMemoryPoolLib)

# Create example test executable
add_executable(example_tests test/example_test.cc)
target_link_libraries(example_tests PRIVATE GTest::gtest GTest::gtest_main ZPMemoryPoolLib)

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
