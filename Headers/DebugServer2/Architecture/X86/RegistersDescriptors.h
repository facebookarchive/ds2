// !!! WARNING !!! THIS FILE WAS AUTOGENERATED !!! DO NOT MODIFY !!!
// !!! WARNING !!! MODIFY INSTEAD: Definitions/X86.json

#pragma once

#include "DebugServer2/Architecture/RegisterLayout.h"

namespace ds2 {
namespace Architecture {
namespace X86 {

enum /* dwarf_reg */ {
  reg_dwarf_eax = 0,
  reg_dwarf_ebx = 3,
  reg_dwarf_ecx = 1,
  reg_dwarf_edx = 2,
  reg_dwarf_esi = 6,
  reg_dwarf_edi = 7,
  reg_dwarf_ebp = 5,
  reg_dwarf_esp = 4,
  reg_dwarf_eip = 8,
  reg_dwarf_st0 = 33,
  reg_dwarf_st1 = 34,
  reg_dwarf_st2 = 35,
  reg_dwarf_st3 = 36,
  reg_dwarf_st4 = 37,
  reg_dwarf_st5 = 38,
  reg_dwarf_st6 = 39,
  reg_dwarf_st7 = 40,
  reg_dwarf_ymm0 = 17,
  reg_dwarf_ymm1 = 18,
  reg_dwarf_ymm2 = 19,
  reg_dwarf_ymm3 = 20,
  reg_dwarf_ymm4 = 21,
  reg_dwarf_ymm5 = 22,
  reg_dwarf_ymm6 = 23,
  reg_dwarf_ymm7 = 24,
  reg_dwarf_xmm0 = 17,
  reg_dwarf_xmm1 = 18,
  reg_dwarf_xmm2 = 19,
  reg_dwarf_xmm3 = 20,
  reg_dwarf_xmm4 = 21,
  reg_dwarf_xmm5 = 22,
  reg_dwarf_xmm6 = 23,
  reg_dwarf_xmm7 = 24,
};

enum /* gdb_reg */ {
  reg_gdb_eax = 0,
  reg_gdb_ebx = 1,
  reg_gdb_ecx = 2,
  reg_gdb_edx = 3,
  reg_gdb_esi = 4,
  reg_gdb_edi = 5,
  reg_gdb_ebp = 6,
  reg_gdb_esp = 7,
  reg_gdb_eip = 8,
  reg_gdb_eflags = 17,
  reg_gdb_cs = 18,
  reg_gdb_ss = 19,
  reg_gdb_ds = 20,
  reg_gdb_es = 21,
  reg_gdb_fs = 22,
  reg_gdb_gs = 23,
  reg_gdb_st0 = 24,
  reg_gdb_st1 = 25,
  reg_gdb_st2 = 26,
  reg_gdb_st3 = 27,
  reg_gdb_st4 = 28,
  reg_gdb_st5 = 29,
  reg_gdb_st6 = 30,
  reg_gdb_st7 = 31,
  reg_gdb_fctrl = 32,
  reg_gdb_fstat = 33,
  reg_gdb_ftag = 34,
  reg_gdb_fiseg = 35,
  reg_gdb_fioff = 36,
  reg_gdb_foseg = 37,
  reg_gdb_fooff = 38,
  reg_gdb_fop = 39,
  reg_gdb_mxcsr = 48,
  reg_gdb_ymm0 = 40,
  reg_gdb_ymm1 = 41,
  reg_gdb_ymm2 = 42,
  reg_gdb_ymm3 = 43,
  reg_gdb_ymm4 = 44,
  reg_gdb_ymm5 = 45,
  reg_gdb_ymm6 = 46,
  reg_gdb_ymm7 = 47,
  reg_gdb_xmm0 = 40,
  reg_gdb_orig_eax = 57,
};

enum /* ehframe_reg */ {
  reg_ehframe_eax = 0,
  reg_ehframe_ebx = 3,
  reg_ehframe_ecx = 1,
  reg_ehframe_edx = 2,
  reg_ehframe_esi = 6,
  reg_ehframe_edi = 7,
  reg_ehframe_ebp = 5,
  reg_ehframe_esp = 4,
  reg_ehframe_eip = 8,
  reg_ehframe_st0 = 33,
  reg_ehframe_st1 = 34,
  reg_ehframe_st2 = 35,
  reg_ehframe_st3 = 36,
  reg_ehframe_st4 = 37,
  reg_ehframe_st5 = 38,
  reg_ehframe_st6 = 39,
  reg_ehframe_st7 = 40,
  reg_ehframe_ymm0 = 17,
  reg_ehframe_ymm1 = 18,
  reg_ehframe_ymm2 = 19,
  reg_ehframe_ymm3 = 20,
  reg_ehframe_ymm4 = 21,
  reg_ehframe_ymm5 = 22,
  reg_ehframe_ymm6 = 23,
  reg_ehframe_ymm7 = 24,
  reg_ehframe_xmm0 = 17,
  reg_ehframe_xmm1 = 18,
  reg_ehframe_xmm2 = 19,
  reg_ehframe_xmm3 = 20,
  reg_ehframe_xmm4 = 21,
  reg_ehframe_xmm5 = 22,
  reg_ehframe_xmm6 = 23,
  reg_ehframe_xmm7 = 24,
};

enum /* lldb_reg */ {
  reg_lldb_eax = 0,
  reg_lldb_ebx = 1,
  reg_lldb_ecx = 2,
  reg_lldb_edx = 3,
  reg_lldb_esi = 4,
  reg_lldb_edi = 5,
  reg_lldb_ebp = 6,
  reg_lldb_esp = 7,
  reg_lldb_eip = 8,
  reg_lldb_ax = 16,
  reg_lldb_bx = 17,
  reg_lldb_cx = 18,
  reg_lldb_dx = 19,
  reg_lldb_si = 20,
  reg_lldb_di = 21,
  reg_lldb_bp = 22,
  reg_lldb_sp = 23,
  reg_lldb_ah = 24,
  reg_lldb_bh = 25,
  reg_lldb_ch = 26,
  reg_lldb_dh = 27,
  reg_lldb_al = 28,
  reg_lldb_bl = 29,
  reg_lldb_cl = 30,
  reg_lldb_dl = 31,
  reg_lldb_eflags = 9,
  reg_lldb_cs = 10,
  reg_lldb_ss = 11,
  reg_lldb_ds = 12,
  reg_lldb_es = 13,
  reg_lldb_fs = 14,
  reg_lldb_gs = 15,
  reg_lldb_st0 = 42,
  reg_lldb_st1 = 43,
  reg_lldb_st2 = 44,
  reg_lldb_st3 = 45,
  reg_lldb_st4 = 46,
  reg_lldb_st5 = 47,
  reg_lldb_st6 = 48,
  reg_lldb_st7 = 49,
  reg_lldb_fctrl = 32,
  reg_lldb_fstat = 33,
  reg_lldb_ftag = 34,
  reg_lldb_fiseg = 35,
  reg_lldb_fioff = 36,
  reg_lldb_foseg = 37,
  reg_lldb_fooff = 38,
  reg_lldb_fop = 39,
  reg_lldb_mxcsr = 40,
  reg_lldb_mxcsrmask = 41,
  reg_lldb_ymm0 = 58,
  reg_lldb_ymm1 = 59,
  reg_lldb_ymm2 = 60,
  reg_lldb_ymm3 = 61,
  reg_lldb_ymm4 = 62,
  reg_lldb_ymm5 = 63,
  reg_lldb_ymm6 = 64,
  reg_lldb_ymm7 = 65,
  reg_lldb_xmm0 = 50,
  reg_lldb_xmm1 = 51,
  reg_lldb_xmm2 = 52,
  reg_lldb_xmm3 = 53,
  reg_lldb_xmm4 = 54,
  reg_lldb_xmm5 = 55,
  reg_lldb_xmm6 = 56,
  reg_lldb_xmm7 = 57,
};

extern LLDBDescriptor const LLDB;
extern GDBDescriptor const GDB;
} // namespace X86
} // namespace Architecture
} // namespace ds2
