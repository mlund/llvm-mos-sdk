# TargetMOS.cmake
# CMake pass for targeting MOS.

# Since this is the MOS build, and we've found our own compiler, don't run 
# the command line tests on CMake startup
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

include(ExternalProject)

ExternalProject_Add(llvm_mos_sdk_mos
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    CMAKE_ARGS 
        -DLLVM_MOS_COMPILE_TARGET=mos
        ${CMAKE_ARGS}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/mos
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    USES_TERMINAL_CONFIGURE Yes 
    USES_TERMINAL_BUILD Yes 
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
    )
