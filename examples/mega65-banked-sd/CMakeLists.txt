# Each example becomes a directory to copy onto the SD card: the main PRG, one
# file per bank, and whatever ASSETS names.
function(add_banked_sd_example name source)
  cmake_parse_arguments(arg "" "" "ASSETS" ${ARGN})
  add_executable(${name}.prg ${source} ${arg_UNPARSED_ARGUMENTS})
  install_example(${name}.prg)
  if(NOT PYTHON3_COMMAND)
    return()
  endif()
  set(asset_flags "")
  foreach(asset ${arg_ASSETS})
    list(APPEND asset_flags --asset ${CMAKE_CURRENT_SOURCE_DIR}/${asset})
  endforeach()
  add_custom_command(TARGET ${name}.prg POST_BUILD
    COMMAND ${PYTHON3_COMMAND} ${PRG_TO_SD_SCRIPT}
      $<TARGET_FILE:${name}.prg>
      $<TARGET_FILE_DIR:${name}.prg>/${name}_sd
      ${name}
      ${asset_flags}
    COMMENT "Building SD card directory for ${name}")
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${name}_sd DESTINATION bin)
endfunction()

add_banked_sd_example(banked-colors banked-colors.cc)
add_banked_sd_example(sd-asset sd-asset.c ASSETS stripes.bin)
