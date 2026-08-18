#!/bin/bash
# Split a mega65-banked combined PRG into main + bank files and create a D81.
#
# The combined output format (from link.ld OUTPUT_FORMAT) is:
#   Bytes 0-1:         PRG load address ($01 $20 = $2001)
#   Bytes 2-24576:     ram region ($2001-$7FFF, bank 0 + BASIC header)
#   Bytes 24577-45056: ram_fixed region ($8000-$CFFF, fixed code/data)
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
#                    [--name <disk-name>] [--no-autoboot]
#
# --name         disk name, and the program's filename when autoboot is off.
#                CBM DOS truncates both to 16 characters.
# --no-autoboot  name the program after the disk rather than autoboot.c65, so
#                it is started with RUN"<name>" instead of on reset. Useful
#                where the C65 autoboot is not wanted or not trusted.

set -e

PRG="$1"
DIR="$2"
BASE="$3"
C1541="$4"

if [ $# -lt 4 ]; then
  echo "make-d81.sh: expected at least 4 arguments" >&2
  exit 1
fi
shift 4

DISK_NAME="$BASE"
AUTOBOOT=1
while [ $# -gt 0 ]; do
  case "$1" in
    --name)
      if [ $# -lt 2 ]; then
        echo "make-d81.sh: --name needs a value" >&2
        exit 1
      fi
      DISK_NAME="$2"
      shift 2
      ;;
    --no-autoboot)
      AUTOBOOT=0
      shift
      ;;
    *)
      echo "make-d81.sh: unknown option $1" >&2
      exit 1
      ;;
  esac
done

RAM_SIZE=24575    # $2001-$7FFF, the bank 0 window
BANK_SLOT=24576   # each bank slot in the combined PRG (from FULL() padding)

# Layout of the combined PRG, from OUTPUT_FORMAT in link.ld:
#   2 bytes load address, RAM_SIZE bytes of the window, __ram_fixed_size bytes
#   of ram_fixed, then one BANK_SLOT per declared bank.
# ram_fixed is emitted as FULL(ram_fixed, 0, __ram_fixed_size), so its length
# varies with the program and has to be read back rather than assumed.
# nm output: "00000002 A __bank_1_size" — we parse the hex value.
ELF="${PRG}.elf"

read_size() {
  nm -B "$ELF" | sed -n "s/^\([0-9a-fA-F]*\) A $1\$/\1/p"
}

RAM_FIXED_HEX=$(read_size __ram_fixed_size)
if [ -z "$RAM_FIXED_HEX" ]; then
  echo "make-d81.sh: __ram_fixed_size not found in $ELF" >&2
  exit 1
fi
MAIN_SIZE=$((2 + RAM_SIZE + 16#$RAM_FIXED_HEX))

dd if="$PRG" of="$DIR/${BASE}-main.prg" bs=$MAIN_SIZE count=1 2>/dev/null
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
  size=$(read_size "__bank_${i}_size")
  eval "BANK_${i}_SIZE=$((16#${size:-0}))"
done

# Remove stale bank files from previous builds to prevent leftover files
# (from a different MAIN_SIZE or bank layout) from corrupting the D81.
# Named per-program: several targets share one build directory, so an
# unqualified BANK* would delete another program's slices mid-build.
rm -f "$DIR/${BASE}"-BANK[0-9A-F]

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
    BANKFILE="$DIR/${BASE}-BANK$UC_SUFFIX"
    { printf '\000\040'; tail -c +"$start" "$PRG" | head -c "$bank_size"; } \
      > "$BANKFILE"
    WRITE_ARGS="$WRITE_ARGS -write $BANKFILE bank$suffix"
  fi
done

DISK_NAME=$(printf '%.16s' "$DISK_NAME")
if [ "$AUTOBOOT" -eq 1 ]; then
  PRG_NAME=autoboot.c65
else
  # Lowercase for the same reason as the bank files: c1541 maps it to standard
  # PETSCII uppercase, which is what the directory match expects.
  PRG_NAME=$(printf '%s' "$DISK_NAME" | tr 'A-Z' 'a-z')
fi
MAIN_WRITE="-write $DIR/${BASE}-main.prg $PRG_NAME"
eval "\"$C1541\" -format \"$DISK_NAME,01\" d81 \"$DIR/${BASE}.d81\" $MAIN_WRITE $WRITE_ARGS"
