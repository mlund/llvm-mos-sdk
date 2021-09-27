# TargetMOS.cmake
# CMake pass for targeting MOS.

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
