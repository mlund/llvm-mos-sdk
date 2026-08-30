; MEGA65 BASIC header that jumps to _start.
;
; Note: BANK 0 is intentionally omitted. On MEGA65, the BASIC 65 BANK command
; triggers FPGA reconfiguration which resets the FDC, detaching any mounted
; D81 disk images. SYS alone works correctly since the program loads into
; bank 0 at $2001.

.section .basic_header,"aR",@progbits
	.short next_line		; address of next BASIC line
	.short 10			; line number
	.byte 0x9e, 0x20		; SYS token + space
	.mos_addr_asciz _start, 4	; the location of the _start symbol, in 4 decimal
					; ASCII digits
next_line:
	.short 0   			; end of basic program
