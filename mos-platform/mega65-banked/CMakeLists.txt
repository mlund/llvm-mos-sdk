platform(mega65-banked COMPLETE HOSTED PARENT mega65-common)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

install(FILES
  _ram-banked.ld
TYPE LIB)
install(FILES mapper.h TYPE INCLUDE)
install(FILES README.md DESTINATION mos-platform/mega65-banked)
install(PROGRAMS prg-to-d81.py TYPE BIN)

add_platform_library(mega65-banked-crt0)
merge_libraries(mega65-banked-crt0
  common-init-stack
  common-exit-loop
)

add_platform_object_file(mega65-banked-load-banks load-banks.o load-banks.c)
add_platform_object_file(mega65-banked-load-banks-kernal load-banks-kernal.o load-banks-kernal.S)
target_compile_options(mega65-banked-load-banks-kernal PRIVATE -mcpu=mos45gs02)
target_link_libraries(mega65-banked-load-banks-kernal PRIVATE common-asminc)

add_platform_library(mega65-banked-c
  mapper.s
)
target_compile_options(mega65-banked-c PUBLIC -mcpu=mos45gs02)
target_link_libraries(mega65-banked-c PRIVATE common-asminc)
