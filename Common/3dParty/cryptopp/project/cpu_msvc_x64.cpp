/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Crypto++ 8.9.0's cpu.cpp calls CPUID64 and XGETBV64 on MSVC x64, even with
// CRYPTOPP_DISABLE_ASM. Upstream implements them in x64dll.asm, which needs
// tables that are only built with the assembly enabled; these are the same
// operations through compiler intrinsics.

#if defined(_M_X64)

#include <intrin.h>
#include <immintrin.h>

extern "C" unsigned long long __fastcall XGETBV64(unsigned int num)
{
    return _xgetbv(num);
}

extern "C" unsigned long long __fastcall CPUID64(unsigned int func, unsigned int subfunc, unsigned int* output)
{
    __cpuidex(reinterpret_cast<int*>(output), static_cast<int>(func), static_cast<int>(subfunc));
    return 0;
}

#endif
