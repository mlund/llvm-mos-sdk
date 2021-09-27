cmake_minimum_required(VERSION 3.10)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_VERBOSE_MAKEFILE ON)

option(LLVM_MOS_BOOTSTRAP_COMPILER "Should I download a working LLVM-MOS compiler for use with this SDK?" On)
set (LLVM_MOS_PROJECT_DESCRIPTION "Software development kit targeting common 8-bit computers with the MOS 65xx processor")
set (LLVM_MOS_PROJECT_URL "https://www.llvm-mos.org")

if (NOT DEFINED LLVM_MOS_COMPILE_TARGET)
    include(TargetHost)
    include(TargetMOS)
    project(llvm-mos-sdk 
    LANGUAGES C CXX ASM
    DESCRIPTION "${LLVM_MOS_PROJECT_DESCRIPTION}"
    HOMEPAGE_URL "${LLVM_MOS_PROJECT_URL}")
else()
if(${LLVM_MOS_COMPILE_TARGET} STREQUAL "mos")
    # Change compiler settings before asserting the project
    include(BootstrapCompiler)
    include(DetectCompiler)
    detect_compiler(_result)
    if (NOT result)
        message("failed to detect LLVM-MOS compiler; compilation may not work")
    endif()
    project(llvm-mos-sdk-mos 
        LANGUAGES C CXX ASM
        DESCRIPTION "${LLVM_MOS_PROJECT_DESCRIPTION} (MOS target)"
        HOMEPAGE_URL "${LLVM_MOS_PROJECT_URL}")
    include(Platform/llvm-mos)
    add_subdirectory(common/lib)
elseif(${LLVM_MOS_COMPILE_TARGET} STREQUAL "host")
    project(llvm-mos-sdk-host 
    LANGUAGES C CXX ASM
    DESCRIPTION "${LLVM_MOS_PROJECT_DESCRIPTION} (host tools)"
    HOMEPAGE_URL "${LLVM_MOS_PROJECT_URL}")
    add_subdirectory(utils/sim/sim)
endif()


endif()

include(CMakePrintSystemInformation)




