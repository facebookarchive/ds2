##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

cmake_minimum_required(VERSION 3.1.0)
cmake_policy(SET CMP0054 NEW)

project(DebugServer2)

set(OS_NAME ${CMAKE_SYSTEM_NAME})
set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})

string(STRIP "${ARCH_NAME}" ARCH_NAME)
string(TOUPPER "${ARCH_NAME}" ARCH_NAME)
if (ARCH_NAME MATCHES "^ARM64" OR ARCH_NAME MATCHES "AARCH64")
  set(ARCH_NAME "ARM64")
elseif (ARCH_NAME MATCHES "^ARM")
  set(ARCH_NAME "ARM") # Normalize ARMHF -> ARM
elseif (ARCH_NAME MATCHES "I386")
  set(ARCH_NAME "X86")
elseif (ARCH_NAME MATCHES "AMD64")
  set(ARCH_NAME "X86_64")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Global options (these apply for other subprojects like JSObjects) and must
  # be set before we include subdirectories.
  if (STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  endif ()

  if (PIE)
    set(COMMON_FLAGS "${COMMON_FLAGS} -fpie")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
  endif ()

  if ("${ARCH_NAME}" STREQUAL "X86")
    set(COMMON_FLAGS "${COMMON_FLAGS} -m32")
  endif ()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if ("${CMAKE_GENERATOR}" MATCHES "ARM")
    set(ARCH_NAME "ARM")
  elseif ("${CMAKE_GENERATOR}" MATCHES "Win64")
    set(ARCH_NAME "X86_64")
  else ()
    set(ARCH_NAME "X86")
  endif ()
endif ()

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
    Sources/Architecture/ARM64/RegistersDescriptors.cpp
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
    Sources/Host/Linux/ExtraWrappers.cpp
    Sources/Host/Linux/ProcFS.cpp
    Sources/Host/Linux/Platform.cpp
    Sources/Host/Linux/PTrace.cpp
    Sources/Host/Linux/${ARCH_NAME}/PTrace${ARCH_NAME}.cpp
    )

set(HOST_FreeBSD_SOURCES
    ${HOST_POSIX_SOURCES}
    Sources/Host/FreeBSD/ProcStat.cpp
    Sources/Host/FreeBSD/Platform.cpp
    Sources/Host/FreeBSD/PTrace.cpp
    Sources/Host/FreeBSD/${ARCH_NAME}/PTrace${ARCH_NAME}.cpp
    )

set(HOST_Windows_SOURCES
    Sources/Host/Windows/Platform.cpp
    Sources/Host/Windows/ProcessSpawner.cpp
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
    Sources/Target/Linux/${ARCH_NAME}/Process${ARCH_NAME}.cpp
    Sources/Target/Linux/${ARCH_NAME}/Thread${ARCH_NAME}.cpp
    Sources/Target/Linux/Process.cpp
    Sources/Target/Linux/Thread.cpp
    )

set(TARGET_FreeBSD_SOURCES
    ${TARGET_POSIX_SOURCES}
    Sources/Target/FreeBSD/Process.cpp
    Sources/Target/FreeBSD/${ARCH_NAME}/Thread${ARCH_NAME}.cpp
    Sources/Target/FreeBSD/${ARCH_NAME}/Process${ARCH_NAME}.cpp
    Sources/Target/FreeBSD/Thread.cpp
    )

set(TARGET_Windows_SOURCES
    Sources/Target/Windows/${ARCH_NAME}/Process${ARCH_NAME}.cpp
    Sources/Target/Windows/${ARCH_NAME}/Thread${ARCH_NAME}.cpp
    Sources/Target/Windows/Process.cpp
    Sources/Target/Windows/Thread.cpp
    )

set(GDB_SOURCES
    Sources/GDB/ByteCodeInterpreter.cpp
    )

set(GDBREMOTE_SOURCES
    Sources/GDBRemote/Structures.cpp
    Sources/GDBRemote/PacketProcessor.cpp
    Sources/GDBRemote/ProtocolInterpreter.cpp
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
    Sources/MessageQueue.cpp
    Sources/SessionThread.cpp
    Sources/Support/Stringify.cpp
    Sources/Utils/Log.cpp
    Sources/Utils/OptParse.cpp
    Sources/main.cpp
    )

set(SUPPORT_POSIX_SOURCES
    Sources/Support/POSIX/ELFSupport.cpp
    Sources/Support/POSIX/FaultHandler.cpp
    Sources/Support/POSIX/Stringify.cpp
    )

set(SUPPORT_Linux_SOURCES
    ${SUPPORT_POSIX_SOURCES}
    )

set(SUPPORT_FreeBSD_SOURCES
    ${SUPPORT_POSIX_SOURCES}
    )

set(SUPPORT_Windows_SOURCES
    Sources/Support/Windows/Stringify.cpp
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
    )

add_executable(ds2 ${DEBUGSERVER2_SOURCES})
target_include_directories(ds2 PUBLIC Headers)
set_property(TARGET ds2 PROPERTY CXX_STANDARD 11)

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" OR "${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
  include(CheckIncludeFile)
  CHECK_INCLUDE_FILE(sys/personality.h HAVE_SYS_PERSONALITY_H)
  if (HAVE_SYS_PERSONALITY_H)
    target_compile_definitions(ds2 PRIVATE HAVE_SYS_PERSONALITY_H)
  endif ()
endif ()

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
  target_compile_definitions(ds2 PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX CINTERFACE)
  target_compile_definitions(ds2 PRIVATE WINVER=_WIN32_WINNT_WIN6 _WIN32_WINNT=_WIN32_WINNT_WIN6)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(ds2 PRIVATE -Wall -Wextra -Wno-unused-parameter)
  # This should be enabled by default but clang has a couple of additional
  # warnings we need to take care of first.
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(ds2 PRIVATE -Werror)
  endif ()
  if (WIN32)
    target_compile_options(ds2 PRIVATE -Wno-missing-braces -Wno-missing-field-initializers -Wno-tautological-compare)
  endif ()
  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "5.1")
    target_compile_options(ds2 PRIVATE -Wsuggest-override)
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  target_compile_options(ds2 PRIVATE /W3 /DSTRICT /wd4244 /wd4996)
  target_compile_options(ds2 PRIVATE /MP)
endif ()

add_subdirectory(Tools/JSObjects "${CMAKE_CURRENT_BINARY_DIR}/JSObjects")
target_link_libraries(ds2 jsobjects)

if (${ENABLE_LOGCAT})
  target_link_libraries(ds2 log)
  target_compile_definitions(ds2 PRIVATE ENABLE_LOGCAT)
endif ()

if ("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
  target_link_libraries(ds2 util procstat)
endif ()

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
  target_link_libraries(ds2 psapi shlwapi ws2_32)
endif ()

include(FindThreads)
target_link_libraries(ds2 ${CMAKE_THREAD_LIBS_INIT})
