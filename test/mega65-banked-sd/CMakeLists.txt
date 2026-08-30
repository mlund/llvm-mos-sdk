cmake_minimum_required(VERSION 3.18)

project(test-mega65-banked-sd LANGUAGES C)

include(../test.cmake)

# Splits the image into the directory that would go on the card, then serves
# that directory as the SD card so the CRT's own bank loading runs. The main
# PRG is injected rather than loaded, because starting it is the launcher's
# job and not the platform's; SYS reaches the entry point at its load address.
function(add_mega65_banked_sd_test name)
  if(NOT XMEGA65_COMMAND)
    return()
  endif()
  if(NOT PYTHON3_COMMAND)
    message(STATUS "python3 not found, skipping banked-sd test: test-${name}")
    return()
  endif()
  add_executable(${name}.prg ${name}.c)
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${PYTHON3_COMMAND} ${PRG_TO_SD_SCRIPT}
      $<TARGET_FILE:${name}.prg>
      $<TARGET_FILE_DIR:${name}.prg>/${name}_sd
      ${name}
    COMMENT "Building SD card directory for ${name}")
  string(TOUPPER ${name} upper)
  add_test(NAME test-${name} COMMAND
    ${XMEGA65_COMMAND} -headless -sleepless -testing -fastboot
    -hdosvirt -hdosdir $<TARGET_FILE_DIR:${name}.prg>/${name}_sd
    -prg $<TARGET_FILE_DIR:${name}.prg>/${name}_sd/${upper}.PRG
    -prgmode 65 -prgtest "SYS8192")
  set_tests_properties(test-${name} PROPERTIES TIMEOUT 30)
endfunction()

# Asserts a byte budget on the linked image, which is how many bank slots the
# link reserved without reading the linker script.
function(add_mega65_banked_sd_size_test name max)
  if(NOT PYTHON3_COMMAND)
    return()
  endif()
  add_executable(${name}.prg ${name}.c)
  add_test(NAME test-${name} COMMAND
    ${PYTHON3_COMMAND} ${SIZECHECK_SCRIPT} --max ${max}
    $<TARGET_FILE:${name}.prg>)
endfunction()

add_mega65_banked_sd_test(boot-map)
add_mega65_banked_sd_test(zp-extent)
add_mega65_banked_sd_test(data-init)
add_mega65_banked_sd_test(bank-window)
add_mega65_banked_sd_test(bank-call)
add_mega65_banked_sd_test(bank-args)
add_mega65_banked_sd_test(bank-nested)
add_mega65_banked_sd_test(bank-to-bank)
add_mega65_banked_sd_test(bank-range)
add_mega65_banked_sd_test(bank-interrupts)
add_mega65_banked_sd_test(sd-load-banks)

# One bank slot plus the window and the fixed region, and nothing for the
# fourteen banks this program does not declare.
add_mega65_banked_sd_size_test(bank-count 70000)

add_test(NAME test-bank-tables-sd
  COMMAND ${PYTHON3_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/check-bank-tables.py)

add_test(NAME test-bank-report-sd
  COMMAND ${PYTHON3_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/check-bank-report.py)
