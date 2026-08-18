function(add_banked_example name source)
  add_executable(${name}.prg ${source})
  install_example(${name}.prg)
  if(C1541_COMMAND)
    add_custom_command(TARGET ${name}.prg POST_BUILD
      COMMAND sh ${MAKE_D81_SCRIPT}
        $<TARGET_FILE:${name}.prg>
        $<TARGET_FILE_DIR:${name}.prg>
        ${name}
        ${C1541_COMMAND}
      COMMENT "Splitting PRG and creating D81 for ${name}")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}-main.prg TYPE BIN)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}.d81 TYPE BIN)
  endif()
endfunction()

add_banked_example(hello-banks hello-banks.cc)
add_banked_example(banked-colors banked-colors.cc)
add_banked_example(banked-irq banked-irq.cc)
add_banked_example(banked-plasma banked-plasma.cc)
add_banked_example(banked-fcm banked-fcm.cc)
