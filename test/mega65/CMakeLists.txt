cmake_minimum_required(VERSION 3.18)

project(test-mega65 LANGUAGES C)

include(../test.cmake)

add_mega65_test(kernal)

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
