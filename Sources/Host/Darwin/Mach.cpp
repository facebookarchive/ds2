//
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#include "DebugServer2/Host/Darwin/Mach.h"
#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Utils/Log.h"

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <limits>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_info.h>
#include <sys/types.h>

namespace ds2 {
namespace Host {
namespace Darwin {

task_t Mach::getMachTask(ProcessId pid) {
  task_t self = mach_task_self();
  task_t task;

  kern_return_t kret = task_for_pid(self, pid, &task);
  if (kret != KERN_SUCCESS) {
    return TASK_NULL;
  }

  return task;
}

thread_t Mach::getMachThread(ProcessThreadId const &ptid) {
  thread_t *thread_list;
  mach_msg_type_number_t thread_count;

  mach_port_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL)
    return THREAD_NULL;

  kern_return_t kret = task_threads(task, &thread_list, &thread_count);
  if (kret != KERN_SUCCESS) {
    return THREAD_NULL;
  }

  // need to have a real way to get the correct thread
  thread_t thread = thread_list[0];

  vm_deallocate(mach_task_self(), (vm_address_t)thread_list,
                thread_count * sizeof(thread_t));

  return thread;
}

ErrorCode Mach::readMemory(ProcessThreadId const &ptid, Address const &address,
                           void *buffer, size_t length, size_t *count) {
  mach_vm_size_t curr_bytes_read = 0;

  mach_port_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL)
    return kErrorProcessNotFound;

  kern_return_t kret =
      mach_vm_read_overwrite((vm_map_t)task, address, length,
                             (mach_vm_address_t)buffer, &curr_bytes_read);
  if (kret != KERN_SUCCESS)
    return kErrorInvalidAddress;

  if (count != nullptr) {
    *count = curr_bytes_read;
  }

  return kSuccess;
}

ErrorCode Mach::writeMemory(ProcessThreadId const &ptid, Address const &address,
                            void const *buffer, size_t length, size_t *count) {
  ErrorCode error = kSuccess;

  // The mach debugging APIs do not allow writing to pages mapped without write
  // permissions; remap them before writing.
  // TODO: Write in multipage ?
  task_t task = getMachTask(ptid.pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

  mach_msg_type_number_t infoCount = VM_REGION_SUBMAP_INFO_COUNT_64;
  vm_region_submap_info_data_64_t regionInfo;
  mach_vm_address_t start = address.value();
  mach_vm_size_t size;
  natural_t depth = 1024;

  kern_return_t kret =
      mach_vm_region_recurse(task, &start, &size, &depth,
                             (vm_region_recurse_info_t)&regionInfo, &infoCount);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidAddress;
  }

  // The combinason of VM_PROT_COPY/VM_PROT_READ is need to override the
  // write protection.
  kret = mach_vm_protect(task, address.value(), length, FALSE,
                         VM_PROT_WRITE | VM_PROT_COPY | VM_PROT_READ);
  if (kret != KERN_SUCCESS) {
    return kErrorInvalidAddress;
  }

  kret = mach_vm_write((vm_map_t)task, address.value(), (vm_offset_t)buffer,
                       length);
  if (kret != KERN_SUCCESS) {
    error = kErrorUnknown;
  } else if (count != nullptr) {
    *count = length;
  }

  kret = mach_vm_protect(task, address.value(), length, FALSE,
                         regionInfo.protection);
  if (kret != KERN_SUCCESS && error == kSuccess) {
    error = kErrorUnknown;
  }

  return error;
}

ErrorCode Mach::suspend(ProcessThreadId const &ptid) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  kern_return_t kret = thread_suspend(thread);
  if (kret != KERN_SUCCESS) {
    return kErrorUnknown;
  }

  return kSuccess;
}

ErrorCode Mach::step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                     int signal, Address const &address) {
  return kErrorUnsupported;
}

ErrorCode Mach::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       int signal, Address const &address) {
  return kErrorUnsupported;
}

ErrorCode Mach::getProcessDylbInfo(ProcessId pid, Address &address) {
  task_t task = getMachTask(pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

  task_dyld_info_data_t dyldInfo;
  mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
  kern_return_t kret =
      task_info(task, TASK_DYLD_INFO, (task_info_t)&dyldInfo, &count);
  if (kret != KERN_SUCCESS) {
    return kErrorUnknown;
  }

  address = dyldInfo.all_image_info_addr;

  return kSuccess;
}

ErrorCode Mach::getProcessMemoryRegion(ProcessId pid, Address const &address,
                                       MemoryRegionInfo &region) {
  task_t task = getMachTask(pid);
  if (task == TASK_NULL) {
    return kErrorProcessNotFound;
  }

  mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
  vm_region_submap_info_data_64_t regionInfo;
  mach_vm_address_t start = address.value();
  mach_vm_size_t size;
  natural_t depth = 1024;
  kern_return_t kret =
      mach_vm_region_recurse(task, &start, &size, &depth,
                             (vm_region_recurse_info_t)&regionInfo, &count);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "unable to retreive region (0x%llx) info: %s", start,
           mach_error_string(kret));
    return kErrorUnknown;
  }

  region.start = start;
  region.length = size;

  if ((regionInfo.protection & VM_PROT_READ) == VM_PROT_READ)
    region.protection |= ds2::kProtectionRead;
  if ((regionInfo.protection & VM_PROT_WRITE) == VM_PROT_WRITE)
    region.protection |= ds2::kProtectionWrite;
  if ((regionInfo.protection & VM_PROT_EXECUTE) == VM_PROT_EXECUTE)
    region.protection |= ds2::kProtectionExecute;

  return kSuccess;
}

ErrorCode Mach::getThreadInfo(ProcessThreadId const &ptid,
                              thread_basic_info_t info) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  unsigned int thread_info_count = THREAD_BASIC_INFO_COUNT;
  kern_return_t kret = thread_info(thread, THREAD_BASIC_INFO,
                                   (thread_info_t)info, &thread_info_count);
  if (kret != KERN_SUCCESS) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

ErrorCode
Mach::getThreadIdentifierInfo(ProcessThreadId const &ptid,
                              thread_identifier_info_data_t *threadInfo) {
  thread_t thread = getMachThread(ptid);
  if (thread == THREAD_NULL) {
    return kErrorProcessNotFound;
  }

  unsigned int thread_info_count = THREAD_IDENTIFIER_INFO_COUNT;
  kern_return_t kret =
      thread_info(thread, THREAD_IDENTIFIER_INFO, (thread_info_t)threadInfo,
                  &thread_info_count);
  if (kret != KERN_SUCCESS) {
    return kErrorProcessNotFound;
  }

  return kSuccess;
}
} // namespace Darwin
} // namespace Host
} // namespace ds2
