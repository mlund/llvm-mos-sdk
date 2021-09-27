# TargetHost.cmake
# Build programs that must be run on this host

include(ExternalProject)

# Run the superbuild-style rebuild of this project, with the compiler
# suite for local builds
ExternalProject_Add(llvm_mos_sdk_host
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    CMAKE_ARGS 
        -DLLVM_MOS_COMPILE_TARGET=host
        ${CMAKE_ARGS}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/host
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --verbose
    USES_TERMINAL_CONFIGURE Yes 
    USES_TERMINAL_BUILD Yes 
    INSTALL_COMMAND ""
    BUILD_ALWAYS Yes
    )
