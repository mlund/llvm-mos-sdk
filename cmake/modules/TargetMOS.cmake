# TargetMOS.cmake
# CMake pass for targeting MOS.

# Since this is the MOS build, and we've found our own compiler, don't run 
# the command line tests on CMake startup
# https://stackoverflow.com/questions/41589430/cmake-c-compiler-identification-fails
SET(CMAKE_C_COMPILER_WORKS TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_WORKS TRUE CACHE INTERNAL "")
SET(CMAKE_C_COMPILER_FORCED TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_FORCED TRUE CACHE INTERNAL "")
SET(CMAKE_C_COMPILER_ID_RUN TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_ID_RUN TRUE CACHE INTERNAL "")

include(ExternalProject)

ExternalProject_Add(llvm_mos_sdk_mos
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    CMAKE_ARGS 
        -DLLVM_MOS_COMPILE_TARGET=mos
        ${CMAKE_ARGS}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mos
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --verbose
    USES_TERMINAL_CONFIGURE Yes 
    USES_TERMINAL_BUILD Yes 
    INSTALL_COMMAND ""
    BUILD_ALWAYS Yes
    )
