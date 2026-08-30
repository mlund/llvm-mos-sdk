platform(mega65 COMPLETE HOSTED PARENT mega65-common)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

install(FILES link.ld TYPE LIB)

add_platform_library(mega65-crt0)
merge_libraries(mega65-crt0
  common-init-stack
  common-exit-loop
)

add_platform_object_file(mega65-unmap-basic unmap-basic.o unmap-basic.S)
