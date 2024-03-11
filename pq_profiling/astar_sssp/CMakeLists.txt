cmake_minimum_required(VERSION 3.10)

project(benchmark CXX)

include_directories(../CPS/include)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++... instead of -std=gnu++...
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g")

find_package(TBB REQUIRED COMPONENTS tbb)
find_package(Threads REQUIRED)

file(GLOB SRC_FILES src/*.cpp)
add_executable(benchmark ${SRC_FILES})

target_link_libraries(benchmark TBB::tbb Threads::Threads)
target_compile_options(benchmark PRIVATE ${TBB_CXX_STD_FLAG})
