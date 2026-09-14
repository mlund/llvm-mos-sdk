cmake_minimum_required(VERSION 3.18)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

add_no_compile_test(bank-too-big)
add_no_compile_test(bank-too-big-16)
add_no_compile_test(bank-window-invalid)
add_no_compile_test(bank-tail-no-room)
add_no_compile_test(bank-kb-too-big)
add_no_compile_test(bank-too-big-per-bank)
add_no_compile_test(bank-cross-megabyte)
add_no_compile_test(banked-call-r-in-bank)
