//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_FreeBSD_ExtraWrappers_h
#define __DebugServer2_Host_FreeBSD_ExtraWrappers_h

#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

// We use ds2_snprintf and ds2_vsnprintf in ds2 code to make sure we don't use
// the bogus vsnprintf provided in the MSVC runtime. The following two defines
// allow us to avoid #ifdef conditionals accross the code.
// See Headers/DebugServer2/Host/Windows/ExtraWrappers.h.
#define ds2_snprintf snprintf
#define ds2_vsnprintf vsnprintf

#endif // !__DebugServer2_Host_FreeBSD_ExtraWrappers_h
