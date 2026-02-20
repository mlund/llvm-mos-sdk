#!/bin/sh
# Split a mega65-banked combined PRG into main + bank files and create a D81.
#
# The combined output format (from link.ld OUTPUT_FORMAT) is:
#   Bytes 0-1:         PRG load address ($01 $20 = $2001)
#   Bytes 2-24576:     ram region ($2001-$7FFF, bank 0 + BASIC header)
#   Bytes 24577-45056: ram_fixed region ($8000-$CFFF, fixed code/data, 20KB)
#   Then 15 x 24576:   bank_1 through bank_15
#
# Bank files get a 2-byte PRG header ($00 $20 = $2000). KERNAL LOAD with
# SA=0 reads and discards the header, loading at the address we specify.
#
# Bank sizes are extracted from __bank_N_size symbols in the ELF file via nm.
# Only the actual used portion of each bank is written, saving D81 disk space.
# Banks with size 0 are skipped entirely.
#
# Usage: make-d81.sh <combined.prg> <output-dir> <basename> <c1541>

set -e

PRG="$1"
DIR="$2"
BASE="$3"
C1541="$4"

MAIN_SIZE=45057
BANK_SLOT=24576   # each bank slot in the combined PRG (from FULL() padding)

dd if="$PRG" of="$DIR/${BASE}-main.prg" bs=$MAIN_SIZE count=1 2>/dev/null

# Extract actual bank sizes from linker symbols in the ELF file.
# nm output: "00000002 A __bank_1_size" — we parse the hex value.
ELF="${PRG}.elf"
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
  size=$(nm -B "$ELF" | sed -n "s/^\([0-9a-fA-F]*\) A __bank_${i}_size$/\1/p")
  eval "BANK_${i}_SIZE=$((16#${size:-0}))"
done

# Extract each non-empty bank and prepend a PRG header.
# Banks 1-9 use decimal names, 10-15 use hex (a-f) to match PETSCII filenames.
# c1541 converts lowercase ASCII to PETSCII $41-$5A (standard uppercase).
# Uppercase ASCII 'A'-'F' would map to PETSCII $C1-$C6 (shifted), which is wrong.
WRITE_ARGS=""
SUFFIXES="1 2 3 4 5 6 7 8 9 a b c d e f"
idx=0
for suffix in $SUFFIXES; do
  idx=$((idx + 1))
  eval "bank_size=\$BANK_${idx}_SIZE"
  if [ "$bank_size" -gt 0 ]; then
    start=$((MAIN_SIZE + (idx - 1) * BANK_SLOT + 1))
    # Local filenames use uppercase for readability; D81 names use lowercase
    # so c1541 stores them as standard PETSCII uppercase ($41-$5A).
    UC_SUFFIX=$(echo "$suffix" | tr 'a-f' 'A-F')
    BANKFILE="$DIR/BANK$UC_SUFFIX"
    { printf '\000\040'; tail -c +"$start" "$PRG" | head -c "$bank_size"; } \
      > "$BANKFILE"
    WRITE_ARGS="$WRITE_ARGS -write $BANKFILE bank$suffix"
  fi
done

MAIN_WRITE="-write $DIR/${BASE}-main.prg autoboot.c65"
eval "\"$C1541\" -format \"test,01\" d81 \"$DIR/${BASE}.d81\" $MAIN_WRITE $WRITE_ARGS"
