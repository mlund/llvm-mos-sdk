platform(mega65-banked COMPLETE HOSTED PARENT mega65-common)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

install(FILES mapper.h TYPE INCLUDE)
install(FILES README.md DESTINATION mos-platform/mega65-banked)

add_platform_library(mega65-banked-crt0)
merge_libraries(mega65-banked-crt0
  common-init-stack
  common-exit-loop
)

add_platform_library(mega65-banked-c
  ../mega65-common/bank-layout.c
  load-banks-kernal.S
)
target_compile_options(mega65-banked-c PUBLIC -mcpu=mos45gs02)
target_link_libraries(mega65-banked-c PRIVATE common-asminc)
target_include_directories(mega65-banked-c BEFORE PUBLIC .
  ${CMAKE_CURRENT_SOURCE_DIR}/../mega65-common
  ${CMAKE_CURRENT_SOURCE_DIR}/../commodore)
