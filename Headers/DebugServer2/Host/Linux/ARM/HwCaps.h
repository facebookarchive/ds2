//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#pragma once

namespace ds2 {
namespace Host {
namespace Linux {
namespace ARM {

enum /* HwCaps */ {
  HWCAP_SWP = (1 << 0),
  HWCAP_HALF = (1 << 1),
  HWCAP_THUMB = (1 << 2),
  HWCAP_26BIT = (1 << 3),
  HWCAP_FAST_MULT = (1 << 4),
  HWCAP_FPA = (1 << 5),
  HWCAP_VFP = (1 << 6),
  HWCAP_EDSP = (1 << 7),
  HWCAP_JAVA = (1 << 8),
  HWCAP_IWMMXT = (1 << 9),
  HWCAP_CRUNCH = (1 << 10),
  HWCAP_THUMBEE = (1 << 11),
  HWCAP_NEON = (1 << 12),
  HWCAP_VFPv3 = (1 << 13),
  HWCAP_VFPv3D16 = (1 << 14), /* also set for VFPv4-D16 */
  HWCAP_TLS = (1 << 15),
  HWCAP_VFPv4 = (1 << 16),
  HWCAP_IDIVA = (1 << 17),
  HWCAP_IDIVT = (1 << 18),
  HWCAP_VFPD32 = (1 << 19), /* set if VFP has 32 regs (not 16) */
  HWCAP_IDIV = (HWCAP_IDIVA | HWCAP_IDIVT),
  HWCAP_LPAE = (1 << 20)
};
}
} // namespace Linux
} // namespace Host
} // namespace ds2
