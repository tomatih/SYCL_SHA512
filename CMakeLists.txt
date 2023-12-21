cmake_minimum_required(VERSION 3.1...3.27)

# User options
set(USE_GPU yes)
set(CHECK_CORRECTNESS yes)

if(USE_GPU)
    # SYCL compatibility
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend \
    --cuda-gpu-arch=sm_86 -Wno-unknown-cuda-version")
endif()
# warning flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -Wextra -Wpedantic")
# optimisation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3")

project(
  Task4
  VERSION 1.0
  LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
# ClangD support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Macro definitions
if(USE_GPU)
    add_compile_definitions(GPU)
endif()

if(CHECK_CORRECTNESS)
    add_compile_definitions(CORRECTNESS)
endif()


# External packages
if(CHECK_CORRECTNESS)
    find_package(OpenSSL REQUIRED)
endif()

# Add my libraries
add_library(SHALib src/sha512.cpp src/sha512.hpp)
add_library(UtilLib src/utils.cpp src/utils.hpp)
if(CHECK_CORRECTNESS)
    add_library(CorrectLib src/correctness.cpp src/correctness.hpp)
endif()
add_library(PerfLib src/performance.cpp src/performance.hpp)
add_library(TargetLib src/target_match.cpp src/target_match.hpp)

# Add the output executable
add_executable(Task4 src/main.cpp)


# Link libraries
if(CHECK_CORRECTNESS)
    target_link_libraries(CorrectLib PRIVATE UtilLib)
    target_link_libraries(CorrectLib PRIVATE SHALib)
    target_link_libraries(CorrectLib PRIVATE OpenSSL::Crypto)
endif()
target_link_libraries(PerfLib PRIVATE UtilLib)
target_link_libraries(PerfLib PRIVATE SHALib)
target_link_libraries(TargetLib PRIVATE UtilLib)
target_link_libraries(TargetLib PRIVATE SHALib)


# Link final executable
if(CHECK_CORRECTNESS)
    target_link_libraries(Task4 PRIVATE CorrectLib)
endif()
target_link_libraries(Task4 PRIVATE PerfLib)
target_link_libraries(Task4 PRIVATE TargetLib)

# add "run" command
add_custom_target(run
    COMMAND Task4
    DEPENDS Task4
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)