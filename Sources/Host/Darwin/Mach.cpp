//
// Copyright (c) 2015, Corentin Derbois <cderbois@gmail.com>
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Mach"

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

task_t Mach::getMachTask(pid_t pid) {
  kern_return_t kret;
  task_t self = mach_task_self();
  task_t task;

  kret = task_for_pid(self, pid, &task);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "Fail to get the task of the pid: %s",
           mach_error_string(kret));
    return TASK_NULL;
  }

  return task;
}

thread_t Mach::getMachThread(pid_t pid, pid_t tid) {
  kern_return_t kret;
  mach_port_t task;
  thread_t thread;
  thread_t *thread_list;
  mach_msg_type_number_t thread_count;

  task = getMachTask(pid);
  if (task == TASK_NULL)
    return THREAD_NULL;

  kret = task_threads(task, &thread_list, &thread_count);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "Unable to retrieve the threads: %s",
           mach_error_string(kret));
    return THREAD_NULL;
  }

  // need to have a real way to get the correct thread
  thread = thread_list[0];

  vm_deallocate(mach_task_self(), (vm_address_t)thread_list,
                thread_count * sizeof(thread_t));

  return thread;
}

ErrorCode Mach::readMemory(ProcessThreadId const &ptid, Address const &address,
                           void *buffer, size_t length, size_t *count) {
  mach_vm_size_t curr_bytes_read = 0;
  kern_return_t kret;
  mach_port_t task;

  task = getMachTask(ptid.pid);
  if (task == TASK_NULL)
    return kErrorProcessNotFound;

  kret = mach_vm_read_overwrite((vm_map_t)task, address, length,
                                (mach_vm_address_t)buffer, &curr_bytes_read);
  if (kret != KERN_SUCCESS)
    return kErrorInvalidAddress;

  *count = curr_bytes_read;

  return kSuccess;
}

ErrorCode Mach::writeMemory(ProcessThreadId const &ptid, Address const &address,
                            void const *buffer, size_t length, size_t *count) {
  return kErrorUnsupported;
}

ErrorCode Mach::suspend(ProcessThreadId const &ptid) {
  return kErrorUnsupported;
}

ErrorCode Mach::step(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                     int signal, Address const &address) {
  return kErrorUnsupported;
}

ErrorCode Mach::resume(ProcessThreadId const &ptid, ProcessInfo const &pinfo,
                       int signal, Address const &address) {
  return kErrorUnsupported;
}

ErrorCode Mach::getThreadInfo(pid_t pid, pid_t tid, thread_basic_info_t info) {
  thread_t thread;

  thread = getMachThread(pid, tid);
  if (thread == THREAD_NULL)
    return kErrorProcessNotFound;

  kern_return_t kret;
  unsigned int thread_info_count = THREAD_BASIC_INFO_COUNT;

  kret = thread_info(thread, THREAD_BASIC_INFO, (thread_info_t)info,
                     &thread_info_count);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "Unable to retrive thread basic info :%s",
           mach_error_string(kret));
    return kErrorProcessNotFound;
  }

  return kSuccess;
}

ErrorCode
Mach::getThreadIdentifierInfo(pid_t pid, pid_t tid,
                              thread_identifier_info_data_t *threadInfo) {
  thread_t thread;

  thread = getMachThread(pid, tid);
  if (thread == THREAD_NULL)
    return kErrorProcessNotFound;

  kern_return_t kret;
  unsigned int thread_info_count = THREAD_IDENTIFIER_INFO_COUNT;

  kret = thread_info(thread, THREAD_IDENTIFIER_INFO, (thread_info_t)threadInfo,
                     &thread_info_count);
  if (kret != KERN_SUCCESS) {
    DS2LOG(Error, "Unable to retrive thread basic info :%s",
           mach_error_string(kret));
    return kErrorProcessNotFound;
  }

  return kSuccess;
}
}
}
}
