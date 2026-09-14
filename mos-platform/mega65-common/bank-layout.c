// Copyright 2026 LLVM-MOS Project
// Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
// See https://github.com/llvm-mos/llvm-mos-sdk/blob/main/LICENSE for license
// information.

// The default layout's tables and loader, for a program none of whose files
// include <mapper.h>; a file that does defines them itself, so this is not
// linked. Each banked platform builds it against its own <mapper.h>.
#include <mapper.h>
