cmake_minimum_required(VERSION 3.22...3.27)

project(
        Task4
        VERSION 1.0
        LANGUAGES C CXX)

# User options
set(CHECK_CORRECTNESS ON CACHE BOOL "Build with OpenSSL correctness checking")
set(USE_GPU ON CACHE BOOL "Build with GPU support")
set(NVIDIA_GPU ON CACHE BOOL "Add support for NVIDIA GPUs" )
set(NVIDIA_ARCH sm_86 CACHE STRING "NVIDIA architectures")
set(AMD_GPU OFF CACHE BOOL "Add support for AMD GPUs")
set(AMD_ARCH gfx1030 CACHE STRING "AMD architectures")
set(INTEL_GPU ON CACHE BOOL "Add support for INTEL GPUs")

# C++ standard
set(CMAKE_CXX_STANDARD 20)
# ClangD support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# warning flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -Wextra -Wpedantic")
# optimisation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3")

# Handle OpenSSL options
if(CHECK_CORRECTNESS)
    # Macro definition
    add_compile_definitions(CORRECTNESS)
    # Link library
    find_package(OpenSSL REQUIRED)
endif()

# Handle GPU options
if(USE_GPU)
    # Define macro
    add_compile_definitions(GPU)

    # link SYCL
    set(SYCL_LIBRARY_DIR /opt/intel/oneapi/compiler/latest/lib)
            #/cmake/IntelSYCL)
    set(SYCL_INCLUDE_DIR /opt/intel/oneapi/compiler/latest/include)
    find_package(IntelSYCL REQUIRED)

    # construct SYCL target list
    set(SYCL_TARGETS "")
    if(NVIDIA_GPU)
        list(APPEND SYCL_TARGETS "nvptx64-nvidia-cuda")
    endif()
    if(AMD_GPU)
        list(APPEND SYCL_TARGETS "amdgcn-amd-amdhsa")
    endif()
    if(INTEL_GPU)
        list(APPEND SYCL_TARGETS "spir64")
    endif()
    # target safety
    if(NOT SYCL_TARGETS)
        message(FATAL_ERROR "Need to specify at leas one GPU target")
    endif()

    # add sycl target list
    string(REPLACE ";" "," SYCL_TARGETS_STR "${SYCL_TARGETS}")
    list(APPEND SYCL_FLAGS "-fsycl-targets=${SYCL_TARGETS_STR}")

    # Nvidia special SYCL flags
    if(NVIDIA_GPU)
        list(APPEND SYCL_FLAGS "-Xsycl-target-backend=nvptx64-nvidia-cuda")
        list(APPEND SYCL_FLAGS "--offload-arch=${NVIDIA_ARCH}")
        list(APPEND SYCL_FLAGS "-Wno-unknown-cuda-version")
    endif()

    # AMD special flags
    if(AMD_GPU)
        list(APPEND SYCL_FLAGS "-Xsycl-target-backend=amdgcn-amd-amdhsa")
        list(APPEND SYCL_FLAGS "--offload-arch=${AMD_ARCH}")
    endif()

    # Refresh the updated flags
    set_property(TARGET IntelSYCL::SYCL_CXX PROPERTY
            INTERFACE_COMPILE_OPTIONS ${SYCL_FLAGS})
    set_property(TARGET IntelSYCL::SYCL_CXX PROPERTY
            INTERFACE_LINK_OPTIONS ${SYCL_FLAGS})

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

# add sycl
if(USE_GPU)
    add_sycl_to_target(TARGET SHALib SOURCES src/sha512.cpp)
    if(CHECK_CORRECTNESS)
        add_sycl_to_target(TARGET CorrectLib SOURCES src/correctness.cpp)
    endif()
    add_sycl_to_target(TARGET PerfLib SOURCES src/performance.cpp)
    add_sycl_to_target(TARGET TargetLib SOURCES src/target_match.cpp)
    add_sycl_to_target(TARGET Task4 SOURCES src/main.cpp)
endif()

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