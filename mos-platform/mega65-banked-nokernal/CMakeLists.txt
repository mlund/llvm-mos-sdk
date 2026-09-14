platform(mega65-banked-nokernal COMPLETE HOSTED PARENT mega65-common)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

install(FILES mapper.h TYPE INCLUDE)
install(FILES README.md DESTINATION mos-platform/mega65-banked-nokernal)

# copy-data because .data lives in ram_high but loads from ram_fixed, which no
# other platform needs and so the parent crt0 does not carry.
add_platform_library(mega65-banked-nokernal-crt0)
merge_libraries(mega65-banked-nokernal-crt0
  common-init-stack
  common-exit-loop
  common-copy-data
)

add_platform_object_file(mega65-banked-nokernal-boot boot.o boot.S)
target_compile_options(mega65-banked-nokernal-boot PRIVATE -mcpu=mos45gs02)
target_link_libraries(mega65-banked-nokernal-boot PRIVATE common-asminc)

add_platform_object_file(mega65-banked-nokernal-load-banks load-banks.o
  load-banks.c)
target_compile_options(mega65-banked-nokernal-load-banks PRIVATE -mcpu=mos45gs02)
target_include_directories(mega65-banked-nokernal-load-banks SYSTEM BEFORE PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../mega65-common
  ${CMAKE_CURRENT_SOURCE_DIR}/../c64
  ${CMAKE_CURRENT_SOURCE_DIR}/../commodore
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/include)
target_link_libraries(mega65-banked-nokernal-load-banks PRIVATE common-asminc)

# The identity char-conv shadows the commodore one, whose startup fragment
# puts the screen into shifted PETSCII through CHROUT -- which is RAM here, so
# calling it hangs before main.
add_platform_library(mega65-banked-nokernal-c
  ../common/c/char-conv.c
  bank-layout.c
  load-banks-floppy.c
  load-banks-hyppo.c
)
target_compile_options(mega65-banked-nokernal-c PUBLIC -mcpu=mos45gs02)
target_include_directories(mega65-banked-nokernal-c BEFORE PUBLIC .
  ${CMAKE_CURRENT_SOURCE_DIR}/../mega65-common
  ${CMAKE_CURRENT_SOURCE_DIR}/../c64
  ${CMAKE_CURRENT_SOURCE_DIR}/../commodore)
target_link_libraries(mega65-banked-nokernal-c PRIVATE common-asminc)
