//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_GDBRemote_Mixins_FileOperationsMixin_h
#define __DebugServer2_GDBRemote_Mixins_FileOperationsMixin_h

#include "DebugServer2/GDBRemote/DummySessionDelegateImpl.h"
#include "DebugServer2/Host/File.h"
#include "DebugServer2/Host/ProcessSpawner.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Thread.h"

#include <utility>

namespace ds2 {
namespace GDBRemote {

template <typename T> class FileOperationsMixin : public T {
protected:
  std::unordered_map<int, Host::File> _openFiles;

public:
  template <typename... Args>
  explicit FileOperationsMixin(Args &&... args)
      : T(std::forward<Args>(args)...) {}

public:
  ErrorCode onFileOpen(Session &session, std::string const &path,
                       uint32_t flags, uint32_t mode, int &fd) override;
  ErrorCode onFileClose(Session &session, int fd) override;
  ErrorCode onFileRead(Session &session, int fd, uint64_t &count,
                       uint64_t offset, ByteVector &buffer) override;
#if 0
    // more F packets:
    // https://sourceware.org/gdb/onlinedocs/gdb/List-of-Supported-Calls.html#List-of-Supported-Calls
    virtual ErrorCode onGetCurrentTime(Session &session, TimeValue &tv);

    virtual ErrorCode onFileIsATTY(Session &session, int fd);
    virtual ErrorCode onFileRename(Session &session,
            std::string const &oldPath, std::string const &newPath);

    virtual ErrorCode onFileGetStat(Session &session, std::string const &path,
            FileStat &stat);
    virtual ErrorCode onFileGetStat(Session &session, int fd,
            FileStat &stat);

    virtual ErrorCode onFileSeek(Session &session, int fd,
            int64_t offset, int whence, int64_t &newOffset);
#endif
};
}
}

#include "../Sources/GDBRemote/Mixins/FileOperationsMixin.hpp"

#endif // !__DebugServer2_GDBRemote_Mixins_FileOperationsMixin_h
