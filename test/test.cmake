include(CTest)

add_library(test-lib-emutest ${CMAKE_CURRENT_SOURCE_DIR}/../test-lib-emutest.c)
target_include_directories(test-lib-emutest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

function(add_emutest_test name binext source_dir libretro_core)
  add_executable(${name}.${binext} ${source_dir}/${name}.c)
  target_link_libraries(${name}.${binext} test-lib-emutest)
  get_property(libretro_shared_lib VARIABLE PROPERTY ${libretro_core})
  add_test(NAME test-${name} COMMAND ${EMUTEST_COMMAND} -T
    -L ${libretro_shared_lib}
    -r $<TARGET_FILE:${name}.${binext}>
    -t ${CMAKE_CURRENT_SOURCE_DIR}/../emutest.lua)
endfunction()

function(add_common_compile_test target type)
  add_executable(${target} ${target}.c)
  add_test(NAME ${target}-${type} COMMAND ${CMAKE_CTEST_COMMAND}
    --build-and-test ${CMAKE_CURRENT_SOURCE_DIR}/..
                     ${CMAKE_CURRENT_BINARY_DIR}/${target}
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-target ${target}
    --build-options
      -DLLVM_MOS=${LLVM_MOS}
      -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
      -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
    )
endfunction()

# negative (failure) compilation test
function(add_compile_test target)
  add_common_compile_test(${target} compile)
endfunction()

# negative (failure) compilation test
function(add_no_compile_test target)
  add_common_compile_test(${target} no-compile)
  set_property(TEST ${target}-no-compile PROPERTY WILL_FAIL YES)
endfunction()

function(add_vcs_test name)
  set(source_dir ".")
  if(ARGC GREATER 1)
    set(source_dir ${ARGV1})
  endif()
  add_emutest_test(${name} a26 ${source_dir} LIBRETRO_STELLA_CORE)
endfunction()

function(add_nes_test name)
  set(source_dir ".")
  if(ARGC GREATER 1)
    set(source_dir ${ARGV1})
  endif()
  add_emutest_test(${name} nes ${source_dir} LIBRETRO_MESEN_CORE)
endfunction()

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
# On success the test returns from main() → BASIC READY → xemu exits 0.
# On failure the test loops forever → CTest timeout → failure.
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

# MEGA65 banked test: extracts the main PRG portion, creates a D81 with it
# as AUTOBOOT.C65, and boots from D81 (no -prg injection).
# The mega65-banked linker emits all bank sections (bank 0 + fixed + banks 1-7),
# producing a ~217KB binary. The test injects bank code at runtime, so the
# empty bank sections are not needed — only the main portion is put on disk.
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
  set(main_prg_size 40961)
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
