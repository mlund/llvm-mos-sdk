platform(mega65 COMPLETE HOSTED PARENT commodore)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()
install(FILES
    _45E100.h
    _dmagic.h
    _vic3.h
    _vic4.h
    dma.hpp
    mega65.h
TYPE INCLUDE)

install(FILES link.ld TYPE LIB)

add_platform_library(mega65-crt0)
merge_libraries(mega65-crt0
  common-init-stack
  common-exit-loop
)

add_platform_object_file(mega65-basic-header basic-header.o basic-header.S)
add_platform_object_file(mega65-unmap-basic unmap-basic.o unmap-basic.S)

add_platform_library(mega65-c
  filevars.s
  mega65_joy_read.cc
  kernal.S
)
target_compile_options(mega65-c PRIVATE -mcpu=mos45gs02)
target_include_directories(mega65-c BEFORE PUBLIC .
  ${CMAKE_CURRENT_SOURCE_DIR}/../c64
  ${CMAKE_CURRENT_SOURCE_DIR}/../commodore)
