cmake_minimum_required(VERSION 3.18)

project(test-mega65-banked LANGUAGES C)

include(../test.cmake)

# MEGA65 banked test: extracts the main PRG portion, creates a D81 with it
# as AUTOBOOT.C65, and boots from D81 (no -prg injection).
# The mega65-banked linker emits all bank sections (bank 0 + fixed + banks 1-7),
# producing a ~217KB binary. The test injects bank code at runtime, so the
# empty bank sections are not needed -- only the main portion is put on disk.
function(add_mega65_banked_test name)
  set(source_dir ".")
  if(ARGC GREATER 1)
    set(source_dir ${ARGV1})
  endif()
  if(NOT XMEGA65_COMMAND)
    return()
  endif()
  if(NOT C1541_COMMAND)
    message(STATUS "c1541 not found, skipping banked test: test-${name}")
    return()
  endif()
  add_executable(${name}.prg ${source_dir}/${name}.c)
  set(main_prg_size 45057)
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND dd if=$<TARGET_FILE:${name}.prg>
      of=$<TARGET_FILE_DIR:${name}.prg>/${name}-main.prg
      bs=${main_prg_size} count=1
    COMMENT "Extracting main PRG (${main_prg_size} bytes)")
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${C1541_COMMAND} -format "test,01" d81
      $<TARGET_FILE_DIR:${name}.prg>/${name}.d81
      -write $<TARGET_FILE_DIR:${name}.prg>/${name}-main.prg "autoboot.c65"
    COMMENT "Creating D81 with autoboot for ${name}")
  add_test(NAME test-${name} COMMAND
    ${XMEGA65_COMMAND} -headless -sleepless -testing
    -8 $<TARGET_FILE_DIR:${name}.prg>/${name}.d81)
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 30)
endfunction()

# Like add_mega65_banked_test, but also splits the combined PRG into bank
# files and creates a D81 disk image with main PRG as AUTOBOOT.C65 and
# bank files for KERNAL LOAD at startup. Boots from D81 (no -prg injection).
function(add_mega65_banked_disk_test name)
  set(source_dir ".")
  if(ARGC GREATER 1)
    set(source_dir ${ARGV1})
  endif()
  if(NOT XMEGA65_COMMAND)
    return()
  endif()
  if(NOT C1541_COMMAND)
    message(STATUS "c1541 not found, skipping disk test: test-${name}")
    return()
  endif()
  add_executable(${name}.prg ${source_dir}/${name}.c)

  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND sh ${MAKE_D81_SCRIPT}
      $<TARGET_FILE:${name}.prg>
      $<TARGET_FILE_DIR:${name}.prg>
      ${name}
      ${C1541_COMMAND}
    COMMENT "Splitting PRG and creating D81 for ${name}")

  add_test(NAME test-${name} COMMAND
    ${XMEGA65_COMMAND} -headless -sleepless -testing
    -8 $<TARGET_FILE_DIR:${name}.prg>/${name}.d81)
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 30)
endfunction()

# MEGA65 banked SD-load test: splits combined PRG with split-banks.lua,
# strips PRG headers from bank files, puts them on the HDOS virtual SD card,
# and autoboots the main PRG from D81. The C test loads banks via Hyppo
# mega65_h_loadfile (no KERNAL calls, no BASIC loader).
function(add_mega65_banked_bload_test name)
  set(source_dir ".")
  if(ARGC GREATER 1)
    set(source_dir ${ARGV1})
  endif()
  if(NOT XMEGA65_COMMAND)
    return()
  endif()
  if(NOT C1541_COMMAND)
    message(STATUS "c1541 not found, skipping bload test: test-${name}")
    return()
  endif()
  if(NOT LUA_COMMAND)
    message(STATUS "lua not found, skipping bload test: test-${name}")
    return()
  endif()

  add_executable(${name}.prg ${source_dir}/${name}.c)

  # Split combined PRG using split-banks.lua.
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${LUA_COMMAND} ${SPLIT_BANKS_SCRIPT}
      $<TARGET_FILE:${name}.prg>
      $<TARGET_FILE_DIR:${name}.prg>/${name}
    COMMENT "Splitting PRG with split-banks.lua for ${name}")

  # Create HDOS directory and strip 2-byte PRG headers from bank files.
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      $<TARGET_FILE_DIR:${name}.prg>/${name}_hdos
    COMMAND dd if=$<TARGET_FILE_DIR:${name}.prg>/${name}.bank1.prg
      of=$<TARGET_FILE_DIR:${name}.prg>/${name}_hdos/BANK1.BIN bs=1 skip=2
    COMMAND dd if=$<TARGET_FILE_DIR:${name}.prg>/${name}.bank2.prg
      of=$<TARGET_FILE_DIR:${name}.prg>/${name}_hdos/BANK2.BIN bs=1 skip=2
    COMMENT "Creating HDOS directory with raw bank files for ${name}")

  # Create D81 with the main PRG as AUTOBOOT.C65.
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${C1541_COMMAND} -format "test,01" d81
      $<TARGET_FILE_DIR:${name}.prg>/${name}.d81
      -write $<TARGET_FILE_DIR:${name}.prg>/${name}.prg "autoboot.c65"
    COMMENT "Creating D81 with autoboot for ${name}")

  # Boot from D81, with HDOS virtual SD for bank files.
  add_test(NAME test-${name} COMMAND
    ${XMEGA65_COMMAND} -headless -sleepless -testing
    -8 $<TARGET_FILE_DIR:${name}.prg>/${name}.d81
    -hdosvirt -hdosdir $<TARGET_FILE_DIR:${name}.prg>/${name}_hdos)
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 60)
endfunction()

add_mega65_banked_test(bank-call)
add_mega65_banked_disk_test(kernal-load-banks)
add_mega65_banked_disk_test(attic-load-banks)
add_mega65_banked_disk_test(banked-dma)
add_mega65_banked_disk_test(banked-dma-loop)
add_mega65_banked_bload_test(hyppo-load-banks)
