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

#include "DebugServer2/GDBRemote/Mixins/FileOperationsMixin.h"
#include "DebugServer2/Host/Platform.h"

namespace ds2 {
namespace GDBRemote {

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileOpen(Session &, std::string const &path,
                                             OpenFlags flags, uint32_t mode,
                                             int &fd) {
  static int fileIdx = 0;

  Host::File file(path, flags, mode);
  if (!file.valid()) {
    return file.lastError();
  }

  fd = fileIdx++;
  _openFiles.emplace(fd, std::move(file));

  return kSuccess;
}

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileClose(Session &session, int fd) {
  auto const it = _openFiles.find(fd);
  if (it == _openFiles.end()) {
    return kErrorInvalidHandle;
  }

  _openFiles.erase(it);
  return kSuccess;
}

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileRead(Session &session, int fd,
                                             uint64_t &count, uint64_t offset,
                                             ByteVector &buffer) {
  auto it = _openFiles.find(fd);
  if (it == _openFiles.end()) {
    return kErrorInvalidHandle;
  }

  return it->second.pread(buffer, count, offset);
}

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileWrite(Session &session, int fd,
                                              uint64_t offset,
                                              ByteVector const &buffer,
                                              uint64_t &nwritten) {
  auto it = _openFiles.find(fd);
  if (it == _openFiles.end()) {
    return kErrorInvalidHandle;
  }

  return it->second.pwrite(buffer, nwritten, offset);
}

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileCreateDirectory(Session &,
                                                        std::string const &path,
                                                        uint32_t flags) {
  return Host::File::createDirectory(path, flags);
}

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileExists(Session &,
                                               std::string const &path) {
  return Host::Platform::IsFilePresent(path) ? kSuccess : kErrorNotFound;
}

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileRemove(Session &session,
                                               std::string const &path) {
  return Host::File::unlink(path);
}

template <typename T>
ErrorCode FileOperationsMixin<T>::onFileSetPermissions(Session &session,
                                                       std::string const &path,
                                                       uint32_t mode) {
  return Host::File::chmod(path, mode);
}
} // namespace GDBRemote
} // namespace ds2
