cmake_minimum_required(VERSION 3.18)

project(test-mega65 LANGUAGES C)

include(../test.cmake)

# MEGA65 test via xmega65 emulator. The test binary signals pass/fail
# through the xemu control register at $D6CF.
function(add_mega65_test name)
  set(source_dir ".")
  if(ARGC GREATER 1)
    set(source_dir ${ARGV1})
  endif()
  if(NOT XMEGA65_COMMAND)
    return()
  endif()
  add_executable(${name}.prg ${source_dir}/${name}.c)
  add_test(NAME test-${name} COMMAND
    ${XMEGA65_COMMAND} -headless -sleepless -testing
    -prg $<TARGET_FILE:${name}.prg> -prgmode 65)
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 30)
endfunction()

# MEGA65 HDOS test: virtualizes SD card file access via Hyppo hypervisor.
# Creates an hdos directory next to the test binary; the caller populates
# it with test data files (see add_custom_command examples).
function(add_mega65_hdos_test name)
  set(source_dir ".")
  if(ARGC GREATER 1)
    set(source_dir ${ARGV1})
  endif()
  if(NOT XMEGA65_COMMAND)
    return()
  endif()
  add_executable(${name}.prg ${source_dir}/${name}.c)

  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      $<TARGET_FILE_DIR:${name}.prg>/${name}_hdos
    COMMENT "Creating HDOS directory for ${name}")

  add_test(NAME test-${name} COMMAND
    ${XMEGA65_COMMAND} -headless -sleepless -testing -fastboot
    -hdosvirt -hdosdir $<TARGET_FILE_DIR:${name}.prg>/${name}_hdos
    -prg $<TARGET_FILE:${name}.prg> -prgmode 65)
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 30)
endfunction()

# MEGA65 disk test: creates a D81 with test data, injects PRG via -prg,
# and mounts D81 on device 8.  Uses -prg + -prgexit instead of -testing
# because KERNAL disk I/O writes to $D6CF (the FPGA reconfiguration
# register), which conflicts with xemu's -testing exit protocol.
# On success the test returns from main() -> BASIC READY -> xemu exits 0.
# On failure the test loops forever -> CTest timeout -> failure.
# The test must link with save-basic.o for clean return to BASIC.
function(add_mega65_disk_test name)
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
  target_link_libraries(${name}.prg PRIVATE -l:save-basic.o)

  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${C1541_COMMAND} -format "test,01" d81
      $<TARGET_FILE_DIR:${name}.prg>/${name}.d81
    COMMENT "Creating D81 for ${name}")

  add_test(NAME test-${name} COMMAND
    ${XMEGA65_COMMAND} -headless -sleepless
    -prg $<TARGET_FILE:${name}.prg> -prgexit
    -8 $<TARGET_FILE_DIR:${name}.prg>/${name}.d81)
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 30)
endfunction()

add_mega65_test(kernal)
add_mega65_test(kernal-far)

add_mega65_disk_test(disk)
if(TARGET disk.prg)
  # Generate PRG test file: 2-byte load address ($7101 LE) + 8 data bytes.
  # Avoids $00 bytes which CMake string(ASCII) cannot represent.
  # c1541 stores this as-is; KERNAL LOAD with SA=0 uses the file header address.
  string(ASCII 1 113 222 173 190 239 202 254 186 190 _disk_test_data)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/TEST.PRG" "${_disk_test_data}")
  # Add test data file to the D81 after add_mega65_disk_test creates it
  add_custom_command(TARGET disk.prg POST_BUILD
    COMMAND ${C1541_COMMAND} -attach
      $<TARGET_FILE_DIR:disk.prg>/disk.d81
      -write "${CMAKE_CURRENT_BINARY_DIR}/TEST.PRG" "test"
    COMMENT "Adding test data file to disk.d81")
endif()

add_mega65_hdos_test(hyppo)
if(TARGET hyppo.prg)
  # Generate 8-byte test payload: DE AD BE EF CA FE BA BE
  string(ASCII 222 173 190 239 202 254 186 190 _hyppo_test_data)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/TEST.BIN" "${_hyppo_test_data}")
  add_custom_command(TARGET hyppo.prg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_BINARY_DIR}/TEST.BIN"
      "$<TARGET_FILE_DIR:hyppo.prg>/hyppo_hdos/TEST.BIN"
    COMMENT "Copying TEST.BIN to hyppo hdos directory")
endif()

add_mega65_test(hyppo-system)

add_mega65_hdos_test(hyppo-dir)
if(TARGET hyppo-dir.prg)
  # Create two small text files for findfile/readdir tests
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/FILE1.TXT" "hello")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/FILE2.TXT" "world")
  add_custom_command(TARGET hyppo-dir.prg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_BINARY_DIR}/FILE1.TXT"
      "$<TARGET_FILE_DIR:hyppo-dir.prg>/hyppo-dir_hdos/FILE1.TXT"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_BINARY_DIR}/FILE2.TXT"
      "$<TARGET_FILE_DIR:hyppo-dir.prg>/hyppo-dir_hdos/FILE2.TXT"
    COMMENT "Copying test files to hyppo-dir hdos directory")
endif()

add_mega65_hdos_test(hyppo-fileio)
if(TARGET hyppo-fileio.prg)
  # Reuse same 8-byte test payload: DE AD BE EF CA FE BA BE
  string(ASCII 222 173 190 239 202 254 186 190 _fileio_test_data)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/FILEIO_TEST.BIN" "${_fileio_test_data}")
  add_custom_command(TARGET hyppo-fileio.prg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_CURRENT_BINARY_DIR}/FILEIO_TEST.BIN"
      "$<TARGET_FILE_DIR:hyppo-fileio.prg>/hyppo-fileio_hdos/TEST.BIN"
    COMMENT "Copying TEST.BIN to hyppo-fileio hdos directory")
endif()
