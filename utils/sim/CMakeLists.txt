add_executable(mos-sim fake6502.c mos-sim.c)
install(TARGETS mos-sim)

# Addressing-mode checks that drive the emulator core directly. Kept out of the
# default build, and not installed; they supply their own memory callbacks, so
# they must not link mos-sim.c.
#
# Run with `cmake --build <dir> --target check-fake6502`. Not registered with
# ctest: enable_testing() would reserve the target name "test", which this
# project already uses for its emulator-driven suites.
add_executable(test-fake6502 EXCLUDE_FROM_ALL fake6502.c test-fake6502.c)
add_custom_target(check-fake6502
  COMMAND test-fake6502
  COMMENT "Checking emulator addressing modes")
add_dependencies(check-fake6502 test-fake6502)
