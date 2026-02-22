-- Copyright (c) 2026 LLVM-MOS Project
--
-- Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
-- See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
-- information.
--
-- Splits a mega65-banked combined binary into separate PRG files.
--
-- The combined output format (from link.ld OUTPUT_FORMAT) is:
--   SHORT(0x2001)               2 bytes  — PRG load address
--   FULL(ram)        $2001-$7FFF 24575 bytes — BASIC header + bank 0
--   FULL(ram_fixed)  $8000-$CFFF 20480 bytes — fixed code/data (ROMC cleared)
--   FULL(bank_1)     24576 bytes — bank 1
--   FULL(bank_2)     24576 bytes — bank 2
--   FULL(bank_3)     24576 bytes — bank 3
--   FULL(bank_4)     24576 bytes — bank 4
--   FULL(bank_5)     24576 bytes — bank 5
--   FULL(bank_6)     24576 bytes — bank 6
--   FULL(bank_7)     24576 bytes — bank 7
--   FULL(bank_8)     24576 bytes — bank 8
--   ...
--   FULL(bank_15)    24576 bytes — bank 15
--
-- Bank physical addresses (ROM at $20000-$3FFFF skipped; +$800 offset
-- from 64KB page boundaries avoids KERNAL LOAD corruption bug):
--   Chip RAM:  Bank 1: $10800  Bank 2: $16800
--   Fast RAM:  Bank 3: $40800  Bank 4: $46800  Bank 5: $4C800
--               Bank 6: $52800  Bank 7: $58800
--   Attic RAM: Bank 8: $8000800 ... Bank 15: $8036800
--
-- Output files:
--   <basename>.prg          — main PRG (load addr $2001, bank 0 + fixed)
--   <basename>.bank1.prg    — bank 1 PRG (load addr $2000)
--   ...
--   <basename>.bank15.prg   — bank 15 PRG (load addr $2000)
--
-- Empty banks (all zeros) are skipped.
--
-- Usage: lua split-banks.lua <combined.prg> [output-basename]

local BANK_MAX = 15
local BANK_SIZE = 0x6000   -- 24KB per bank
local RAM_SIZE = 0x5FFF    -- $2001-$7FFF = 24575 bytes
local FIXED_SIZE = 0x5000  -- $8000-$CFFF = 20480 bytes (ROMC cleared)
local HEADER_SIZE = 2      -- SHORT(load_addr)

-- Physical load addresses for each bank (for BLOAD P() parameter).
-- Skips ROM at $20000-$3FFFF.
local BANK_LOAD_ADDR = {
    [1]  = 0x10800,    -- Chip RAM
    [2]  = 0x16800,    -- Chip RAM
    [3]  = 0x40800,    -- Fast RAM
    [4]  = 0x46800,    -- Fast RAM
    [5]  = 0x4C800,    -- Fast RAM
    [6]  = 0x52800,    -- Fast RAM
    [7]  = 0x58800,    -- Fast RAM
    [8]  = 0x8000800,  -- Attic RAM (HyperRAM)
    [9]  = 0x8006800,  -- Attic RAM
    [10] = 0x8010800,  -- Attic RAM
    [11] = 0x8016800,  -- Attic RAM
    [12] = 0x8020800,  -- Attic RAM
    [13] = 0x8026800,  -- Attic RAM
    [14] = 0x8030800,  -- Attic RAM
    [15] = 0x8036800,  -- Attic RAM
}

local function is_empty(data)
    for i = 1, #data do
        if data:byte(i) ~= 0 then
            return false
        end
    end
    return true
end

local function write_prg(filename, load_addr, data)
    local f = assert(io.open(filename, "wb"))
    f:write(string.char(load_addr & 0xFF, (load_addr >> 8) & 0xFF))
    f:write(data)
    f:close()
end

-- Parse arguments
local input_file = arg[1]
if not input_file then
    io.stderr:write("Usage: lua split-banks.lua <combined.prg> [output-basename]\n")
    os.exit(1)
end

local basename = arg[2]
if not basename then
    basename = input_file:gsub("%.prg$", "")
end

-- Read combined binary
local f = assert(io.open(input_file, "rb"))
local data = f:read("*a")
f:close()

local main_size = HEADER_SIZE + RAM_SIZE + FIXED_SIZE

if #data < main_size then
    io.stderr:write(string.format(
        "Error: input file too small (%d bytes, expected at least %d)\n",
        #data, main_size))
    os.exit(1)
end

-- Extract and write main PRG (already has correct load address header)
local main_data = data:sub(1, main_size)
local main_file = basename .. ".prg"
local out = assert(io.open(main_file, "wb"))
out:write(main_data)
out:close()
print(string.format("  %s (%d bytes, load $2001)", main_file, #main_data))

-- Extract and write each bank
local banks_written = 0
for i = 1, BANK_MAX do
    local offset = main_size + (i - 1) * BANK_SIZE + 1 -- Lua strings are 1-indexed
    if offset + BANK_SIZE - 1 <= #data then
        local bank_data = data:sub(offset, offset + BANK_SIZE - 1)
        if not is_empty(bank_data) then
            local phys = BANK_LOAD_ADDR[i]
            local bank_file = string.format("%s.bank%d.prg", basename, i)
            write_prg(bank_file, 0x2000, bank_data)
            print(string.format("  %s (%d bytes, load $2000, BLOAD P($%05X))",
                                bank_file, HEADER_SIZE + #bank_data, phys))
            banks_written = banks_written + 1
        end
    end
end

print(string.format("\nSplit complete: 1 main + %d bank file(s)", banks_written))
