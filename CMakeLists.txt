##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0054 NEW)

project(DebugServer2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(OS_NAME ${CMAKE_SYSTEM_NAME})
set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})

string(STRIP "${OS_NAME}" OS_NAME)
if (OS_NAME MATCHES "WindowsStore")
  set(OS_NAME "Windows")
endif ()

string(STRIP "${ARCH_NAME}" ARCH_NAME)
string(TOUPPER "${ARCH_NAME}" ARCH_NAME)
if (ARCH_NAME MATCHES "^ARM64" OR ARCH_NAME MATCHES "AARCH64")
  set(ARCH_NAME "ARM64")
elseif (ARCH_NAME MATCHES "^ARM")
  set(ARCH_NAME "ARM") # Normalize ARMHF -> ARM
elseif (ARCH_NAME MATCHES "I[3-6]86")
  set(ARCH_NAME "X86")
elseif (ARCH_NAME MATCHES "AMD64")
  set(ARCH_NAME "X86_64")
endif ()

if ("${OS_NAME}" MATCHES "Linux" OR "${OS_NAME}" MATCHES "Android")
  include(CheckIncludeFile)
  CHECK_INCLUDE_FILE(sys/personality.h HAVE_SYS_PERSONALITY_H)

  include(CheckSymbolExists)
  set(CMAKE_REQUIRED_DEFINITIONS "-D_XOPEN_SOURCE=600")
  CHECK_SYMBOL_EXISTS(posix_openpt "stdlib.h;fcntl.h" HAVE_POSIX_OPENPT)
  CHECK_SYMBOL_EXISTS(gettid "unistd.h" HAVE_GETTID)
  CHECK_SYMBOL_EXISTS(process_vm_readv "sys/uio.h" HAVE_PROCESS_VM_READV)
  CHECK_SYMBOL_EXISTS(process_vm_writev "sys/uio.h" HAVE_PROCESS_VM_WRITEV)
  CHECK_SYMBOL_EXISTS(tgkill "signal.h" HAVE_TGKILL)
  set(CMAKE_REQUIRED_DEFINITIONS)

  include(CheckTypeSize)
  set(CMAKE_EXTRA_INCLUDE_FILES unistd.h sys/ptrace.h)
  CHECK_TYPE_SIZE("enum __ptrace_request" ENUM_PTRACE_REQUEST)
  set(CMAKE_EXTRA_INCLUDE_FILES)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Global options (these apply for other subprojects like JSObjects) and must
  # be set before we include subdirectories.
  if (STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  else ()
    # Export symbols, so that we can symbolicate backtraces
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
  endif ()

  if (SANITIZER)
    set(COMMON_FLAGS "${COMMON_FLAGS} -g -fno-omit-frame-pointer")
  endif ()

  if (SANITIZER STREQUAL "asan")
    set(COMMON_FLAGS "${COMMON_FLAGS} -fsanitize=address")
  elseif (SANITIZER STREQUAL "ubsan")
    set(COMMON_FLAGS "${COMMON_FLAGS} -fsanitize=integer -fsanitize=undefined")
  elseif (SANITIZER STREQUAL "tsan")
    set(COMMON_FLAGS "${COMMON_FLAGS} -fsanitize=thread")
  endif ()

  if (COVERAGE)
    set(COMMON_FLAGS "${COMMON_FLAGS} -g -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
  endif ()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if ("${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" MATCHES "x64")
    set(ARCH_NAME "X86_64")
  elseif ("${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" MATCHES "X86")
    set(ARCH_NAME "X86")
  elseif ("${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" MATCHES "ARM")
    set(ARCH_NAME "ARM")
  endif()
endif ()

set(ARCHITECTURE_COMMON_SOURCES
    Sources/Architecture/RegisterLayout.cpp
    )

set(ARCHITECTURE_ARM_SOURCES
    Sources/Architecture/ARM/ARMBranchInfo.cpp
    Sources/Architecture/ARM/ThumbBranchInfo.cpp
    Sources/Architecture/ARM/RegistersDescriptors.cpp
    Sources/Architecture/ARM/SoftwareSingleStep.cpp
    )

set(ARCHITECTURE_ARM64_SOURCES
    ${ARCHITECTURE_ARM_SOURCES}
    Sources/Architecture/ARM64/RegistersDescriptors.cpp
    )

set(ARCHITECTURE_X86_SOURCES
    Sources/Architecture/X86/RegistersDescriptors.cpp
    )

set(ARCHITECTURE_X86_64_SOURCES
    ${ARCHITECTURE_X86_SOURCES}
    Sources/Architecture/X86_64/RegistersDescriptors.cpp
    )

set(HOST_COMMON_SOURCES
    Sources/Host/Common/Channel.cpp
    Sources/Host/Common/Platform.cpp
    Sources/Host/Common/QueueChannel.cpp
    Sources/Host/Common/Socket.cpp
    )

set(HOST_POSIX_SOURCES
    Sources/Host/POSIX/File.cpp
    Sources/Host/POSIX/Platform.cpp
    Sources/Host/POSIX/PTrace.cpp
    Sources/Host/POSIX/ProcessSpawner.cpp
    )

set(HOST_Linux_SOURCES
    ${HOST_POSIX_SOURCES}
    Sources/Host/Linux/ProcFS.cpp
    Sources/Host/Linux/Platform.cpp
    Sources/Host/Linux/PTrace.cpp
    Sources/Host/Linux/${ARCH_NAME}/PTrace${ARCH_NAME}.cpp
    )

set(HOST_Android_SOURCES ${HOST_Linux_SOURCES})

set(HOST_Darwin_SOURCES
    ${HOST_POSIX_SOURCES}
    Sources/Host/Darwin/LibProc.cpp
    Sources/Host/Darwin/Platform.cpp
    Sources/Host/Darwin/PTrace.cpp
    Sources/Host/Darwin/Mach.cpp
    Sources/Host/Darwin/${ARCH_NAME}/Mach${ARCH_NAME}.cpp
    Sources/Host/Darwin/${ARCH_NAME}/PTrace${ARCH_NAME}.cpp
    )

set(HOST_FreeBSD_SOURCES
    ${HOST_POSIX_SOURCES}
    Sources/Host/FreeBSD/ProcStat.cpp
    Sources/Host/FreeBSD/Platform.cpp
    Sources/Host/FreeBSD/PTrace.cpp
    Sources/Host/FreeBSD/${ARCH_NAME}/PTrace${ARCH_NAME}.cpp
    )

set(HOST_Windows_SOURCES
    Sources/Host/Windows/File.cpp
    Sources/Host/Windows/Platform.cpp
    Sources/Host/Windows/ProcessSpawner.cpp
    )

set(TARGET_COMMON_SOURCES
    Sources/Target/Common/ProcessBase.cpp
    Sources/Target/Common/${ARCH_NAME}/ProcessBase${ARCH_NAME}.cpp
    Sources/Target/Common/ThreadBase.cpp
    )

set(TARGET_POSIX_ELF_SOURCES
    Sources/Target/POSIX/ELFProcess.cpp
    )

set(TARGET_POSIX_SOURCES
    Sources/Target/POSIX/Process.cpp
    Sources/Target/POSIX/Thread.cpp
    )

set(TARGET_Linux_SOURCES
    ${TARGET_POSIX_ELF_SOURCES}
    ${TARGET_POSIX_SOURCES}
    Sources/Target/Linux/Process.cpp
    Sources/Target/Linux/${ARCH_NAME}/Process${ARCH_NAME}.cpp
    Sources/Target/Linux/Thread.cpp
    )

set(TARGET_Android_SOURCES ${TARGET_Linux_SOURCES})

set(TARGET_Darwin_SOURCES
    ${TARGET_POSIX_SOURCES}
    Sources/Target/Darwin/MachOProcess.cpp
    Sources/Target/Darwin/Process.cpp
    Sources/Target/Darwin/${ARCH_NAME}/Process${ARCH_NAME}.cpp
    Sources/Target/Darwin/${ARCH_NAME}/Thread${ARCH_NAME}.cpp
    Sources/Target/Darwin/Thread.cpp
    )

set(TARGET_FreeBSD_SOURCES
    ${TARGET_POSIX_ELF_SOURCES}
    ${TARGET_POSIX_SOURCES}
    Sources/Target/FreeBSD/Process.cpp
    Sources/Target/FreeBSD/${ARCH_NAME}/Process${ARCH_NAME}.cpp
    Sources/Target/FreeBSD/Thread.cpp
    )

set(TARGET_Windows_SOURCES
    Sources/Target/Windows/Process.cpp
    Sources/Target/Windows/Thread.cpp
    Sources/Target/Windows/${ARCH_NAME}/Thread${ARCH_NAME}.cpp
    )

set(GDBREMOTE_SOURCES
    Sources/GDBRemote/DebugSessionImpl.cpp
    Sources/GDBRemote/DummySessionDelegateImpl.cpp
    Sources/GDBRemote/PacketProcessor.cpp
    Sources/GDBRemote/PlatformSessionImpl.cpp
    Sources/GDBRemote/ProtocolInterpreter.cpp
    Sources/GDBRemote/Session.cpp
    Sources/GDBRemote/SessionBase.cpp
    Sources/GDBRemote/SlaveSessionImpl.cpp
    Sources/GDBRemote/Structures.cpp
    )

set(UTILS_COMMON_SOURCES
    Sources/Utils/Backtrace.cpp
    Sources/Utils/Log.cpp
    Sources/Utils/OptParse.cpp
    Sources/Utils/Paths.cpp
    Sources/Utils/Stringify.cpp
    Sources/main.cpp
    )

set(SUPPORT_POSIX_ELF_SOURCES
    Sources/Support/POSIX/ELFSupport.cpp
    )

set(UTILS_POSIX_SOURCES
    Sources/Utils/POSIX/Daemon.cpp
    Sources/Utils/POSIX/FaultHandler.cpp
    Sources/Utils/POSIX/Stringify.cpp
    )

set(UTILS_Linux_SOURCES
    ${SUPPORT_POSIX_ELF_SOURCES}
    ${UTILS_POSIX_SOURCES}
    )

set(UTILS_Android_SOURCES ${UTILS_Linux_SOURCES})

set(UTILS_Darwin_SOURCES
    ${UTILS_POSIX_SOURCES}
    )

set(UTILS_FreeBSD_SOURCES
    ${SUPPORT_POSIX_ELF_SOURCES}
    ${UTILS_POSIX_SOURCES}
    )

set(UTILS_Windows_SOURCES
    Sources/Utils/Windows/Daemon.cpp
    Sources/Utils/Windows/FaultHandler.cpp
    Sources/Utils/Windows/Stringify.cpp
    )

set(CORE_COMMON_SOURCES
    Sources/Core/BreakpointManager.cpp
    Sources/Core/HardwareBreakpointManager.cpp
    Sources/Core/SoftwareBreakpointManager.cpp
    Sources/Core/CPUTypes.cpp
    Sources/Core/ErrorCodes.cpp
    Sources/Core/MessageQueue.cpp
    Sources/Core/SessionThread.cpp
    )

set(CORE_ARM_SOURCES
    Sources/Core/ARM/HardwareBreakpointManager.cpp
    Sources/Core/ARM/SoftwareBreakpointManager.cpp
    )

set(CORE_X86_SOURCES
    Sources/Core/X86/HardwareBreakpointManager.cpp
    Sources/Core/X86/SoftwareBreakpointManager.cpp
    )

set(CORE_ARM64_SOURCES ${CORE_ARM_SOURCES})
set(CORE_X86_64_SOURCES ${CORE_X86_SOURCES})

set(HOST_SOURCES ${HOST_COMMON_SOURCES} ${HOST_${OS_NAME}_SOURCES})
set(TARGET_SOURCES ${TARGET_COMMON_SOURCES} ${TARGET_${OS_NAME}_SOURCES})
set(ARCHITECTURE_SOURCES ${ARCHITECTURE_COMMON_SOURCES} ${ARCHITECTURE_${ARCH_NAME}_SOURCES})
set(UTILS_SOURCES ${UTILS_COMMON_SOURCES} ${UTILS_${OS_NAME}_SOURCES})
set(CORE_SOURCES ${CORE_COMMON_SOURCES} ${CORE_${ARCH_NAME}_SOURCES})

set(DEBUGSERVER2_SOURCES
    ${HOST_SOURCES}
    ${TARGET_SOURCES}
    ${ARCHITECTURE_SOURCES}
    ${UTILS_SOURCES}
    ${GDBREMOTE_SOURCES}
    ${CORE_SOURCES}
    )

if (MSVC_IDE OR XCODE)
  file(GLOB_RECURSE headers
    Headers/DebugServer2/*.h)
  set(DEBUGSERVER2_SOURCES ${DEBUGSERVER2_SOURCES} ${headers})
endif()

if (LIBRARY)
  add_library(ds2 SHARED ${DEBUGSERVER2_SOURCES})
else ()
  add_executable(ds2 ${DEBUGSERVER2_SOURCES})
endif ()
target_include_directories(ds2 PUBLIC Headers)
set_property(TARGET ds2 PROPERTY CXX_STANDARD 11)
set_property(TARGET ds2 PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ds2 PROPERTY CXX_STANDARD_REQUIRED ON)

execute_process(COMMAND git rev-parse --short HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE DS2_GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
set_property(SOURCE Sources/main.cpp APPEND PROPERTY COMPILE_DEFINITIONS
             DS2_GIT_HASH="${DS2_GIT_HASH}")

if ("${OS_NAME}" MATCHES "Windows")
  target_compile_definitions(ds2 PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX CINTERFACE)
  target_compile_definitions(ds2 PRIVATE WINVER=_WIN32_WINNT_WIN6
                                         _WIN32_WINNT=_WIN32_WINNT_WIN6)
endif ()

if ("${OS_NAME}" MATCHES "Linux" OR "${OS_NAME}" MATCHES "Android")
  foreach (CHECK SYS_PERSONALITY_H GETTID POSIX_OPENPT TGKILL
           PROCESS_VM_READV PROCESS_VM_WRITEV
           ENUM_PTRACE_REQUEST)
    if (HAVE_${CHECK})
      target_compile_definitions(ds2 PRIVATE HAVE_${CHECK})
    endif ()
  endforeach ()
elseif ("${OS_NAME}" MATCHES "Windows")
  set(CMAKE_EXTRA_INCLUDE_FILES windows.h)

  include(CheckFunctionExists)
  foreach (FUNC CreateRemoteThread GetVersionExA GetVersionExW
      GetWindowsDirectoryA GetWindowsDirectoryW EnumProcesses
      K32EnumProcessModules GetModuleBaseNameA GetModuleBaseNameW
      GetModuleFileNameExA GetModuleFileNameExW GetModuleHandleA
      GetModuleHandleW OpenProcessToken GetTokenInformation LookupAccountSidA
      LookupAccountSidW CopySid GetLengthSid GetEnvironmentStringsW
      FreeEnvironmentStringsW CreateProcessA CreateProcessW CreateThread
      ExitThread ResumeThread TerminateThread SuspendThread GetThreadContext
      SetThreadContext GetThreadPriority SetThreadPriority GetProcessId
      GetThreadId OpenProcess OpenThread GetExitCodeProcess GetExitCodeThread
      ReadProcessMemory WriteProcessMemory FlushInstructionCache
      WaitForDebugEventEx ContinueDebugEvent TerminateProcess
      DebugActiveProcess DebugActiveProcessStop VirtualAllocEx VirtualFreeEx
      VirtualQueryEx CreateToolhelp32Snapshot Thread32First Thread32Next
      SetUnhandledExceptionFilter)
    CHECK_FUNCTION_EXISTS(${FUNC} HAVE_${FUNC})
    if (HAVE_${CHECK})
      target_compile_definitions(ds2 PRIVATE HAVE_${FUNC})
    endif ()
  endforeach()

  include(CheckTypeSize)
  foreach (TYPE LPPROC_THREAD_ATTRIBUTE_LIST "struct _STARTUPINFOW"
      "struct _STARTUPINFOEXW" "struct _STARTUPINFOA"
      "struct _PROCESS_INFORMATION" "struct tagTHREADENTRY32"
      PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER)
    string(REPLACE " " "_" SANITIZED_TYPE ${TYPE})
    CHECK_TYPE_SIZE(${TYPE} ${SANITIZED_TYPE})
    if (HAVE_${SANITIZED_TYPE})
      target_compile_definitions(ds2 PRIVATE HAVE_${SANITIZED_TYPE})
    endif ()
  endforeach ()

  set(CMAKE_EXTRA_INCLUDE_FILES)
endif ()

if (TIZEN)
  target_compile_definitions(ds2 PRIVATE __TIZEN__)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
    ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND
    NOT "${CMAKE_CXX_SIMULATE_ID}" MATCHES "MSVC"))
  include(CheckCCompilerFlag)
  include(CheckCXXCompilerFlag)

  function(enable_warning TARGET FLAG)
    string(REGEX REPLACE "[-/ ]" "" TAG "${FLAG}")

    CHECK_C_COMPILER_FLAG("${FLAG}" ${TAG}_FLAG_AVAILABLE_C)
    if (${TAG}_FLAG_AVAILABLE_C)
      target_compile_options("${TARGET}" PRIVATE
          $<$<COMPILE_LANGUAGE:C>:${FLAG}>)
    endif ()

    CHECK_CXX_COMPILER_FLAG("${FLAG}" "${TAG}_FLAG_AVAILABLE_CXX")
    if (${TAG}_FLAG_AVAILABLE_CXX)
      target_compile_options("${TARGET}" PRIVATE
          $<$<COMPILE_LANGUAGE:CXX>:${FLAG}>)
    endif ()
  endfunction()

  function(enable_warning_version TARGET FLAG COMPILER_ID MIN_VERSION)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "${COMPILER_ID}" AND
        NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS "${MIN_VERSION}")
      enable_warning("${TARGET}" "${FLAG}")
    endif ()
  endfunction()

  enable_warning(ds2 -Wall)
  enable_warning(ds2 -Wextra)
  enable_warning(ds2 -Werror)

  # Disable `-Wunused-parameter` to help with callbacks and delegates.
  enable_warning(ds2 -Wno-unused-parameter)

  # Extra warnings.
  enable_warning(ds2 -Wcomma)
  enable_warning(ds2 -Wdeprecated)
  enable_warning(ds2 -Wextra-semi)
  enable_warning(ds2 -Wfloat-equal)
  enable_warning(ds2 -Winconsistent-missing-override)
  enable_warning(ds2 -Wmissing-prototypes)
  enable_warning(ds2 -Wnon-virtual-dtor)
  enable_warning(ds2 -Wsuggest-override)

  # `-Wunused-but-set-parameter` seems to be buggy in gcc pre-6.0 when dealing
  # with `nullptr_t`.
  enable_warning_version(ds2 -Wno-unused-but-set-parameter GNU 6.0)

  # -Wshadow on clang pre-4.0 is too restrictive and annoying.
  enable_warning_version(ds2 -Wshadow Clang 4.0)
  enable_warning_version(ds2 -Wshadow AppleClang 8.1)
  enable_warning_version(ds2 -Wcomma Clang 3.9)
  enable_warning_version(ds2 -Wcomma AppleClang 8.1)

  # Some Windows headers won't build without these.
  if ("${OS_NAME}" MATCHES "Windows")
    enable_warning(ds2 -Wno-missing-braces)
    enable_warning(ds2 -Wno-missing-field-initializers)
    enable_warning(ds2 -Wno-tautological-compare)
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  target_compile_options(ds2 PRIVATE /W3 /DSTRICT /wd4244 /wd4996)
  target_compile_options(ds2 PRIVATE /MP)
endif ()

add_subdirectory(Tools/JSObjects "${CMAKE_CURRENT_BINARY_DIR}/JSObjects")
target_link_libraries(ds2 jsobjects)

if ("${OS_NAME}" MATCHES "Android")
  target_link_libraries(ds2 log)
elseif ("${OS_NAME}" MATCHES "Linux" AND NOT TIZEN)
  target_link_libraries(ds2 dl)
endif ()

if ("${OS_NAME}" MATCHES "FreeBSD")
  target_link_libraries(ds2 util procstat)
endif ()

if ("${OS_NAME}" MATCHES "Windows")
  target_link_libraries(ds2 shlwapi ws2_32)
  if ("${CMAKE_LINKER}" MATCHES "lld")
    target_link_libraries(ds2 advapi32)
  endif()
  if ("${CMAKE_SYSTEM_NAME}" MATCHES "WindowsStore")
    target_link_libraries(ds2 onecore)
  else ()
    target_link_libraries(ds2 psapi)
  endif ()
endif ()

if (COVERAGE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(ds2 gcov)
  endif ()
endif ()

include(FindThreads)
if (STATIC AND DEFINED CMAKE_THREAD_LIBS_INIT AND
    (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  target_link_libraries(ds2 -Wl,--whole-archive ${CMAKE_THREAD_LIBS_INIT}
                            -Wl,--no-whole-archive)
else ()
  target_link_libraries(ds2 ${CMAKE_THREAD_LIBS_INIT})
endif ()

install(TARGETS ds2 DESTINATION bin)
