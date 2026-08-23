platform(mega65-banked-sd COMPLETE HOSTED PARENT mega65)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

install(FILES
  _ram-banked-sd.ld
TYPE LIB)
install(FILES mapper.h TYPE INCLUDE)
install(FILES README.md DESTINATION mos-platform/mega65-banked-sd)
install(PROGRAMS prg-to-sd.py TYPE BIN)

# copy-data because .data lives in ram_high but loads from ram_fixed, which no
# other platform needs and so the parent crt0 does not carry.
add_platform_library(mega65-banked-sd-crt0)
merge_libraries(mega65-banked-sd-crt0
  common-init-stack
  common-exit-loop
  common-copy-data
)

add_platform_object_file(mega65-banked-sd-boot boot.o boot.S)
target_compile_options(mega65-banked-sd-boot PRIVATE -mcpu=mos45gs02)
target_link_libraries(mega65-banked-sd-boot PRIVATE common-asminc)

add_platform_object_file(mega65-banked-sd-load-banks load-banks-sd.o
  load-banks-sd.c bank-sizes.s)
target_compile_options(mega65-banked-sd-load-banks PRIVATE -mcpu=mos45gs02)
target_include_directories(mega65-banked-sd-load-banks SYSTEM BEFORE PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../mega65
  ${CMAKE_CURRENT_SOURCE_DIR}/../c64
  ${CMAKE_CURRENT_SOURCE_DIR}/../commodore
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/include)
target_link_libraries(mega65-banked-sd-load-banks PRIVATE common-asminc)

# mapper.c is layout-independent: it only tracks the shadow and calls
# __set_bank_asm. The identity char-conv shadows the commodore one, whose
# startup fragment puts the screen into shifted PETSCII through CHROUT --
# which is RAM here, so calling it hangs before main.
add_platform_library(mega65-banked-sd-c
  ../mega65-banked/mapper.c
  ../common/c/char-conv.c
  mapper.s
)
target_compile_options(mega65-banked-sd-c PUBLIC -mcpu=mos45gs02)
target_include_directories(mega65-banked-sd-c BEFORE PUBLIC .)
target_link_libraries(mega65-banked-sd-c PRIVATE common-asminc)
