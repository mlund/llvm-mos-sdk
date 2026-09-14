cmake_minimum_required(VERSION 3.18)

set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON)

add_compile_test(bank-fits)
add_compile_test(bank-fits-16)
add_compile_test(bank-tail-fits)
add_compile_test(banked-call-r-in-fixed)
add_compile_test(loader-sd)
