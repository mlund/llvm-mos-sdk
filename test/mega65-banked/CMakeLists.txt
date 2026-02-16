cmake_minimum_required(VERSION 3.18)

project(test-mega65-banked LANGUAGES C)

include(../test.cmake)

add_mega65_banked_test(bank-call)
add_mega65_banked_disk_test(kernal-load-banks)
add_mega65_banked_disk_test(attic-load-banks)
add_mega65_banked_bload_test(hyppo-load-banks)
