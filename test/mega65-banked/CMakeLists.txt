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

# Like add_mega65_banked_test, but saves main_ram on exit and asserts on bytes
# the test wrote, rather than relying only on the one byte the $D6CF exit code
# carries. Extra arguments are ADDR=BYTES checks, passed through to
# dumpcheck.py; main_ram is indexed by physical address, so zero page and
# ram_fixed are at their own addresses in the dump.
function(add_mega65_banked_dump_test name)
  if(NOT XMEGA65_COMMAND)
    return()
  endif()
  if(NOT C1541_COMMAND)
    message(STATUS "c1541 not found, skipping dump test: test-${name}")
    return()
  endif()
  if(NOT PYTHON3_COMMAND)
    message(STATUS "python3 not found, skipping dump test: test-${name}")
    return()
  endif()
  add_executable(${name}.prg ${name}.c)
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND sh ${MAKE_D81_SCRIPT}
      $<TARGET_FILE:${name}.prg>
      $<TARGET_FILE_DIR:${name}.prg>
      ${name}
      ${C1541_COMMAND}
    COMMENT "Splitting PRG and creating D81 for ${name}")
  add_test(NAME test-${name} COMMAND
    ${PYTHON3_COMMAND} ${DUMPCHECK_SCRIPT}
      --emulator ${XMEGA65_COMMAND}
      --d81 $<TARGET_FILE_DIR:${name}.prg>/${name}.d81
      --dump $<TARGET_FILE_DIR:${name}.prg>/${name}.dump
      ${ARGN})
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 30)
endfunction()

# Builds a program and asserts the output is no larger than a budget. Used to
# pin down how many bank slots a link actually reserved, which is otherwise
# only visible by reading the linker script.
function(add_mega65_banked_size_test name max_bytes)
  if(NOT PYTHON3_COMMAND)
    message(STATUS "python3 not found, skipping size test: test-${name}")
    return()
  endif()
  add_executable(${name}.prg ${name}.c)
  add_test(NAME test-${name} COMMAND
    ${PYTHON3_COMMAND} ${SIZECHECK_SCRIPT}
      --max ${max_bytes} $<TARGET_FILE:${name}.prg>)
endfunction()

add_subdirectory(no-compile)
add_subdirectory(compile)

add_mega65_banked_test(bank-call)
# Tracer bullet: get_bank() reads 0 at boot, sentinel proves the program ran.
add_mega65_banked_dump_test(bank-probe FB=00 FC=A5)
# A bank switch must preserve the caller's I flag: clear stays clear, set
# stays set, through both set_bank and the banked_call trampoline. Only bit 2
# of P is recorded.
add_mega65_banked_dump_test(bank-interrupts FB=00 FC=04 FD=00 FE=A5)
add_mega65_banked_disk_test(kernal-load-banks)
add_mega65_banked_disk_test(attic-load-banks)
add_mega65_banked_disk_test(banked-dma)
add_mega65_banked_disk_test(banked-dma-loop)
add_mega65_banked_bload_test(hyppo-load-banks)
# An out-of-range bank id is masked to its low four bits: set_bank(0x11)
# maps bank 1, rather than indexing past bank_map_table into the MAP registers.
add_mega65_banked_dump_test(bank-range FB=5A FC=A5)
# One declared bank: main image plus a single 24 KB slot, not fifteen -- and
# the main image carries only the ram_fixed bytes actually used, not the whole
# 20 KB region. Budget is 24575 (bank 0 window) + 24576 (one bank) + slack.
add_mega65_banked_size_test(bank-count 55000)

# The bank layout is encoded in three files that nothing but a SYNC comment
# keeps in agreement. Recomputes each bank's physical base from every form and
# compares. Needs no emulator and no build.
if(PYTHON3_COMMAND)
  add_test(NAME test-bank-tables COMMAND
    ${PYTHON3_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/check-bank-tables.py)
endif()

# banked_call nests, and get_bank() reports the live bank at each depth:
# 0 -> 1 -> 2 -> back to 1 -> back to 0.
add_mega65_banked_dump_test(bank-nested C000=00 C001=01 C002=02 C003=01 C004=00 C005=A5)

# Code executes from attic RAM banks 8-15, each reporting its own bank id.
add_mega65_banked_dump_test(bank-attic-call
  C000=08 C001=09 C002=0A C003=0B C004=0C C005=0D C006=0E C007=0F C008=A5)
