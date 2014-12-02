##
## Copyright (c) 2014, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

cmake_minimum_required(VERSION 2.8)

add_subdirectory(Tools)

project(DebugServer2)

set(OS_NAME ${CMAKE_SYSTEM_NAME})

if (NOT ARCH_NAME)
  execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH_NAME)
endif ()

string(STRIP "${ARCH_NAME}" ARCH_NAME)
string(TOUPPER "${ARCH_NAME}" ARCH_NAME)
if (ARCH_NAME MATCHES "^ARM")
  set(ARCH_NAME "ARM") # Normalize ARMHF -> ARM
elseif (ARCH_NAME MATCHES "I386")
  set(ARCH_NAME "X86")
elseif (ARCH_NAME MATCHES "AMD64")
  set(ARCH_NAME "X86_64")
endif ()

message("Building for target ${OS_NAME}-${ARCH_NAME}")

set(ARCHITECTURE_COMMON_SOURCES
    Sources/Architecture/RegisterLayout.cpp
    )

set(ARCHITECTURE_ARM_SOURCES
    Sources/Architecture/ARM/ARMBranchInfo.cpp
    Sources/Architecture/ARM/ThumbBranchInfo.cpp
    Sources/Architecture/ARM/SoftwareBreakpointManager.cpp
    Sources/Architecture/ARM/RegistersDescriptors.cpp
    )

set(ARCHITECTURE_ARM64_SOURCES
    ${ARCHITECTURE_ARM_SOURCES}
    )

set(ARCHITECTURE_X86_SOURCES
    Sources/Architecture/X86/SoftwareBreakpointManager.cpp
    Sources/Architecture/X86/RegistersDescriptors.cpp
    )

set(ARCHITECTURE_X86_64_SOURCES
    ${ARCHITECTURE_X86_SOURCES}
    Sources/Architecture/X86_64/RegistersDescriptors.cpp
    )

set(HOST_COMMON_SOURCES
    Sources/Host/Common/Channel.cpp
    Sources/Host/Common/QueueChannel.cpp
    Sources/Host/Common/Socket.cpp
    )

set(HOST_POSIX_SOURCES
    Sources/Host/POSIX/Platform.cpp
    Sources/Host/POSIX/PTrace.cpp
    Sources/Host/POSIX/AsyncProcessWaiter.cpp
    Sources/Host/POSIX/ProcessSpawner.cpp
    )

set(HOST_Linux_SOURCES
    ${HOST_POSIX_SOURCES}
    Sources/Host/Linux/ProcFS.cpp
    Sources/Host/Linux/Platform.cpp
    Sources/Host/Linux/PTrace.cpp
    Sources/Host/Linux/${ARCH_NAME}/PTrace${ARCH_NAME}.cpp
    )

set(HOST_Windows_SOURCES
    Sources/Host/Windows/Platform.cpp
    )

set(TARGET_COMMON_SOURCES
    Sources/Target/ProcessBase.cpp
    Sources/Target/ThreadBase.cpp
    )

set(TARGET_POSIX_SOURCES
    Sources/Target/POSIX/ELFProcess.cpp
    Sources/Target/POSIX/Process.cpp
    Sources/Target/POSIX/Thread.cpp
    )

set(TARGET_Linux_SOURCES
    ${TARGET_POSIX_SOURCES}
    Sources/Target/Linux/Process.cpp
    Sources/Target/Linux/${ARCH_NAME}/Thread${ARCH_NAME}.cpp
    Sources/Target/Linux/${ARCH_NAME}/Process${ARCH_NAME}.cpp
    Sources/Target/Linux/Thread.cpp
    )

set(GDB_SOURCES
    Sources/GDB/ByteCodeInterpreter.cpp
    )

set(GDBREMOTE_SOURCES
    Sources/GDBRemote/Structures.cpp
    Sources/GDBRemote/PacketProcessor.cpp
    Sources/GDBRemote/ProtocolInterpeter.cpp
    Sources/GDBRemote/ProtocolHelpers.cpp

    Sources/GDBRemote/SessionBase.cpp
    Sources/GDBRemote/Session.cpp

    Sources/GDBRemote/DummySessionDelegateImpl.cpp
    Sources/GDBRemote/PlatformSessionImpl.cpp
    Sources/GDBRemote/DebugSessionImpl.cpp
    Sources/GDBRemote/SlaveSessionImpl.cpp
    )

set(SUPPORT_COMMON_SOURCES
    Sources/BreakpointManager.cpp
    Sources/CPUTypes.cpp
    Sources/ErrorCodes.cpp
    Sources/Log.cpp
    Sources/OptParse.cpp
    Sources/MessageQueue.cpp
    )

set(SUPPORT_POSIX_SOURCES
    Sources/Support/POSIX/ELFSupport.cpp
    Sources/Support/POSIX/FaultHandler.cpp
    )

set(SUPPORT_Linux_SOURCES
    ${SUPPORT_POSIX_SOURCES}
    )

set(HOST_SOURCES ${HOST_COMMON_SOURCES} ${HOST_${OS_NAME}_SOURCES})
set(TARGET_SOURCES ${TARGET_COMMON_SOURCES} ${TARGET_${OS_NAME}_SOURCES})
set(ARCHITECTURE_SOURCES ${ARCHITECTURE_COMMON_SOURCES} ${ARCHITECTURE_${ARCH_NAME}_SOURCES})
set(SUPPORT_SOURCES ${SUPPORT_COMMON_SOURCES} ${SUPPORT_${OS_NAME}_SOURCES})

set(DEBUGSERVER2_SOURCES
    ${HOST_SOURCES}
    ${TARGET_SOURCES}
    ${ARCHITECTURE_SOURCES}
    ${SUPPORT_SOURCES}
    ${GDB_SOURCES}
    ${GDBREMOTE_SOURCES}
    Main/main.cpp
    Main/SessionThread.cpp
    )

add_executable(ds2 ${DEBUGSERVER2_SOURCES})

include_directories(Headers)
include_directories(Main)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-unused-parameter")
if (STATIC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif ()

if (WIN32)
  # Some constructs from Windows API headers won't compile properly
  # with our standard warnings set.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces -Wno-missing-field-initializers -Wno-tautological-compare")
endif ()

include(CheckFunctionExists)
foreach (FUNC gettid personality posix_openpt wait4)
  string(TOUPPER "${FUNC}" UCFUNC)
  CHECK_FUNCTION_EXISTS(${FUNC} HAVE_${UCFUNC})
  if (HAVE_${UCFUNC})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_${UCFUNC}")
  endif ()
endforeach ()

include(FindThreads)
target_link_libraries(ds2 ${CMAKE_THREAD_LIBS_INIT})
