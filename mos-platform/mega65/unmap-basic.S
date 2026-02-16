; MEGA65 CRT init/fini.
;
; Note: Unlike the C64 CRT, we intentionally leave CPU I/O port ($00/$01)
; and VIC-III ROM bank ($D030) alone. On MEGA65, writing to these registers
; during early startup can trigger hypervisor traps / FPGA reconfiguration
; which resets the FDC, detaching mounted D81 disk images. This matches
; the Calypsi toolchain and MidnightMega's approach.
;
; Applications that need to unmap ROMs for extra RAM should do so
; explicitly after startup.

; intentionally empty — no ROM unmapping on MEGA65
