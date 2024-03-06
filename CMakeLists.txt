cmake_minimum_required(VERSION 3.12)
project(Uni-C)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Set the output directory for the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add the include directory
include_directories(include)

# Find all source files recursively
file(GLOB_RECURSE SOURCES src/*.cpp)

# Create the executable
add_executable(Compiler ${SOURCES})

# Add Debug compiler arguments
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-g)
endif()
# Add optimization flag for release build
if(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(-OFast)
endif()
