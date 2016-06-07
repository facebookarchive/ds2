//
// Copyright (c) 2014-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#ifndef __DebugServer2_Host_Windows_ExtraWrappers_h
#define __DebugServer2_Host_Windows_ExtraWrappers_h

#include "DebugServer2/Base.h"

#include <cstdarg>
#include <cstdio>
#include <windows.h>

#define DS2_EXCEPTION_UNCAUGHT_COM 0x800706BA
#define DS2_EXCEPTION_UNCAUGHT_USER 0xE06D7363
#define DS2_EXCEPTION_UNCAUGHT_WINRT 0x40080201

// Some APIs are not exposed when building for UAP.
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

// clang-format off
extern "C" {

#define UNLEN 256

WINBASEAPI HANDLE WINAPI CreateRemoteThread(
  _In_  HANDLE                 hProcess,
  _In_  LPSECURITY_ATTRIBUTES  lpThreadAttributes,
  _In_  SIZE_T                 dwStackSize,
  _In_  LPTHREAD_START_ROUTINE lpStartAddress,
  _In_  LPVOID                 lpParameter,
  _In_  DWORD                  dwCreationFlags,
  _Out_ LPDWORD                lpThreadId
);

WINBASEAPI BOOL WINAPI GetVersionExA(
  _Inout_  LPOSVERSIONINFOA lpVersionInfo
);
WINBASEAPI BOOL WINAPI GetVersionExW(
  _Inout_  LPOSVERSIONINFOW lpVersionInfo
);
#define GetVersionEx GetVersionExW

WINBASEAPI UINT WINAPI GetWindowsDirectoryA(
  _Out_ LPSTR  lpBuffer,
  _In_  UINT   uSize
);
WINBASEAPI UINT WINAPI GetWindowsDirectoryW(
  _Out_ LPWSTR lpBuffer,
  _In_  UINT   uSize
);
#define GetWindowsDirectory GetWindowsDirectoryW

#define EnumProcesses K32EnumProcesses
WINBASEAPI BOOL WINAPI EnumProcesses(
  _Out_ DWORD *pProcessIds,
  _In_  DWORD cb,
  _Out_ DWORD *pBytesReturned
);

#define EnumProcessModules    K32EnumProcessModules
#define EnumProcessModulesEx  K32EnumProcessModulesEx
WINBASEAPI BOOL WINAPI EnumProcessModules(
  _In_  HANDLE  hProcess,
  _Out_ HMODULE *lphModule,
  _In_  DWORD   cb,
  _Out_ LPDWORD lpcbNeeded
);

#define GetModuleBaseNameA  K32GetModuleBaseNameA
#define GetModuleBaseNameW  K32GetModuleBaseNameW
WINBASEAPI DWORD WINAPI GetModuleBaseNameA(
  _In_     HANDLE  hProcess,
  _In_opt_ HMODULE hModule,
  _Out_    LPSTR   lpBaseName,
  _In_     DWORD   nSize
);
WINBASEAPI DWORD WINAPI GetModuleBaseNameW(
  _In_     HANDLE  hProcess,
  _In_opt_ HMODULE hModule,
  _Out_    LPWSTR  lpBaseName,
  _In_     DWORD   nSize
);
#define GetModuleBaseName GetModuleBaseNameW

#define GetModuleFileNameExA  K32GetModuleFileNameExA
#define GetModuleFileNameExW  K32GetModuleFileNameExW
WINBASEAPI DWORD WINAPI GetModuleFileNameExA(
  _In_     HANDLE  hProcess,
  _In_opt_ HMODULE hModule,
  _Out_    LPSTR   lpFilename,
  _In_     DWORD   nSize
);
WINBASEAPI DWORD WINAPI GetModuleFileNameExW(
  _In_     HANDLE  hProcess,
  _In_opt_ HMODULE hModule,
  _Out_    LPWSTR  lpFilename,
  _In_     DWORD   nSize
);
#define GetModuleFileNameEx GetModuleFileNameExW

WINBASEAPI HMODULE WINAPI GetModuleHandleA(
  _In_opt_ LPCSTR lpModuleName
);
WINBASEAPI HMODULE WINAPI GetModuleHandleW(
  _In_opt_ LPCWSTR lpModuleName
);
#define GetModuleHandle GetModuleHandleW

WINBASEAPI BOOL WINAPI OpenProcessToken(
  _In_  HANDLE  ProcessHandle,
  _In_  DWORD   DesiredAccess,
  _Out_ PHANDLE TokenHandle
);

WINBASEAPI BOOL WINAPI GetTokenInformation(
  _In_      HANDLE                  TokenHandle,
  _In_      TOKEN_INFORMATION_CLASS TokenInformationClass,
  _Out_opt_ LPVOID                  TokenInformation,
  _In_      DWORD                   TokenInformationLength,
  _Out_     PDWORD                  ReturnLength
);

WINBASEAPI BOOL WINAPI LookupAccountSidA(
  _In_opt_  LPCSTR        lpSystemName,
  _In_      PSID          lpSid,
  _Out_opt_ LPSTR         lpName,
  _Inout_   LPDWORD       cchName,
  _Out_opt_ LPSTR         lpReferencedDomainName,
  _Inout_   LPDWORD       cchReferencedDomainName,
  _Out_     PSID_NAME_USE peUse
);
WINBASEAPI BOOL WINAPI LookupAccountSidW(
  _In_opt_  LPCWSTR       lpSystemName,
  _In_      PSID          lpSid,
  _Out_opt_ LPWSTR        lpName,
  _Inout_   LPDWORD       cchName,
  _Out_opt_ LPWSTR        lpReferencedDomainName,
  _Inout_   LPDWORD       cchReferencedDomainName,
  _Out_     PSID_NAME_USE peUse
);
#define LookupAccountSid LookupAccountSidW

WINBASEAPI BOOL WINAPI CopySid(
  _In_  DWORD nDestinationSidLength,
  _Out_ PSID  pDestinationSid,
  _In_  PSID  pSourceSid
);

WINBASEAPI DWORD WINAPI GetLengthSid(
  _In_ PSID pSid
);

WINBASEAPI LPWCH WINAPI GetEnvironmentStringsW(void);

WINBASEAPI BOOL WINAPI FreeEnvironmentStringsW(
  _In_  LPWCH lpszEnvironmentBlock
);

typedef struct PROC_THREAD_ATTRIBUTE_LIST *LPPROC_THREAD_ATTRIBUTE_LIST;

typedef struct _STARTUPINFOW {
    DWORD  cb;
    LPWSTR lpReserved;
    LPWSTR lpDesktop;
    LPWSTR lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    WORD   wShowWindow;
    WORD   cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

typedef struct _STARTUPINFOEXW {
    STARTUPINFOW                 StartupInfo;
    LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
} STARTUPINFOEXW, *LPSTARTUPINFOEXW;

typedef struct _STARTUPINFOA {
    DWORD  cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    WORD   wShowWindow;
    WORD   cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

WINBASEAPI BOOL WINAPI CreateProcessA(
  _In_opt_     LPCSTR lpApplicationName,
  _Inout_opt_  LPSTR lpCommandLine,
  _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
  _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
  _In_         BOOL bInheritHandles,
  _In_         DWORD dwCreationFlags,
  _In_opt_     LPVOID lpEnvironment,
  _In_opt_     LPCSTR lpCurrentDirectory,
  _In_         LPSTARTUPINFOA lpStartupInfo,
  _Out_        LPPROCESS_INFORMATION lpProcessInformation
);
WINBASEAPI BOOL WINAPI CreateProcessW(
  _In_opt_     LPCWSTR lpApplicationName,
  _Inout_opt_  LPWSTR lpCommandLine,
  _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
  _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
  _In_         BOOL bInheritHandles,
  _In_         DWORD dwCreationFlags,
  _In_opt_     LPVOID lpEnvironment,
  _In_opt_     LPCWSTR lpCurrentDirectory,
  _In_         LPSTARTUPINFOW lpStartupInfo,
  _Out_        LPPROCESS_INFORMATION lpProcessInformation
);
#define CreateProcess CreateProcessW

WINBASEAPI HANDLE WINAPI CreateThread(
  _In_opt_   LPSECURITY_ATTRIBUTES lpThreadAttributes,
  _In_       SIZE_T dwStackSize,
  _In_       LPTHREAD_START_ROUTINE lpStartAddress,
  _In_opt_   LPVOID lpParameter,
  _In_       DWORD dwCreationFlags,
  _Out_opt_  LPDWORD lpThreadId
);

WINBASEAPI VOID WINAPI ExitThread(
  _In_  DWORD dwExitCode
);

WINBASEAPI DWORD WINAPI ResumeThread(
  _In_  HANDLE hThread
);

WINBASEAPI BOOL WINAPI TerminateThread(
  _Inout_ HANDLE hThread,
  _In_    DWORD  dwExitCode
);

WINBASEAPI DWORD WINAPI SuspendThread(
  _In_ HANDLE hThread
);

WINBASEAPI BOOL WINAPI GetThreadContext(
  _In_     HANDLE hThread,
  _Inout_  LPCONTEXT lpContext
);

WINBASEAPI BOOL WINAPI SetThreadContext(
  _In_  HANDLE hThread,
  _In_  const CONTEXT *lpContext
);

WINBASEAPI int WINAPI GetThreadPriority(
  _In_  HANDLE hThread
);

WINBASEAPI BOOL WINAPI SetThreadPriority(
  _In_  HANDLE hThread,
  _In_  int nPriority
);

WINBASEAPI DWORD WINAPI GetProcessId(
  _In_ HANDLE Process
);

WINBASEAPI DWORD WINAPI GetThreadId(
  _In_  HANDLE Thread
);

WINBASEAPI HANDLE WINAPI OpenProcess(
  _In_  DWORD dwDesiredAccess,
  _In_  BOOL bInheritHandle,
  _In_  DWORD dwProcessId
);

WINBASEAPI HANDLE WINAPI OpenThread(
  _In_  DWORD dwDesiredAccess,
  _In_  BOOL bInheritHandle,
  _In_  DWORD dwThreadId
);

WINBASEAPI BOOL WINAPI GetExitCodeProcess(
  _In_  HANDLE  hProcess,
  _Out_ LPDWORD lpExitCode
);

WINBASEAPI BOOL WINAPI GetExitCodeThread(
  _In_   HANDLE hThread,
  _Out_  LPDWORD lpExitCode
);

WINBASEAPI BOOL WINAPI ReadProcessMemory(
  _In_   HANDLE hProcess,
  _In_   LPCVOID lpBaseAddress,
  _Out_  LPVOID lpBuffer,
  _In_   SIZE_T nSize,
  _Out_  SIZE_T *lpNumberOfBytesRead
);

WINBASEAPI BOOL WINAPI WriteProcessMemory(
  _In_   HANDLE hProcess,
  _In_   LPVOID lpBaseAddress,
  _In_   LPCVOID lpBuffer,
  _In_   SIZE_T nSize,
  _Out_  SIZE_T *lpNumberOfBytesWritten
);

WINBASEAPI BOOL WINAPI WaitForDebugEvent(
  _Out_  LPDEBUG_EVENT lpDebugEvent,
  _In_   DWORD dwMilliseconds
);

WINBASEAPI BOOL WINAPI ContinueDebugEvent(
  _In_  DWORD dwProcessId,
  _In_  DWORD dwThreadId,
  _In_  DWORD dwContinueStatus
);

WINBASEAPI BOOL WINAPI TerminateProcess(
  _In_  HANDLE hProcess,
  _In_  UINT uExitCode
);

WINBASEAPI BOOL WINAPI DebugActiveProcess(
  _In_  DWORD dwProcessId
);

WINBASEAPI BOOL WINAPI DebugActiveProcessStop(
  _In_ DWORD dwProcessId
);

WINBASEAPI LPVOID WINAPI VirtualAllocEx(
  _In_     HANDLE hProcess,
  _In_opt_ LPVOID lpAddress,
  _In_     SIZE_T dwSize,
  _In_     DWORD  flAllocationType,
  _In_     DWORD  flProtect
);

WINBASEAPI BOOL WINAPI VirtualFreeEx(
  _In_ HANDLE hProcess,
  _In_ LPVOID lpAddress,
  _In_ SIZE_T dwSize,
  _In_ DWORD  dwFreeType
);

typedef struct tagTHREADENTRY32 {
  DWORD dwSize;
  DWORD cntUsage;
  DWORD th32ThreadID;
  DWORD th32OwnerProcessID;
  LONG  tpBasePri;
  LONG  tpDeltaPri;
  DWORD dwFlags;
} THREADENTRY32, *PTHREADENTRY32, *LPTHREADENTRY32;

#define TH32CS_SNAPTHREAD 0x00000004
WINBASEAPI HANDLE WINAPI CreateToolhelp32Snapshot(
  _In_  DWORD dwFlags,
  _In_  DWORD th32ProcessID
);

WINBASEAPI BOOL WINAPI Thread32First(
  _In_     HANDLE hSnapshot,
  _Inout_  LPTHREADENTRY32 lpte
);

WINBASEAPI BOOL WINAPI Thread32Next(
  _In_   HANDLE hSnapshot,
  _Out_  LPTHREADENTRY32 lpte
);

typedef LONG (WINAPI *PTOP_LEVEL_EXCEPTION_FILTER)(
    _In_ struct _EXCEPTION_POINTERS *ExceptionInfo
    );

typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;

WINBASEAPI LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(
  _In_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
);
} // extern "C"
// clang-format on

// The following code allows us to pick some symbols directly from kernel32.dll
// (against which we cannot link when building for WinStore-ARM for instance.
// The functions are declared in the above block (see Thread32First for
// instance), but these declarations are then hidden with macros below.
// Basically, we just
//     GetProcAddress(GetModuleHandle("kernel32"), funcName)
// and call that.
//
// Given that we have declarations for these functions above, we can also
// type-check these calls and make sure both the return type and the arguments
// type (see `decltype` in `DO_K32_CALL`) are matching.
//
// If we need more functions that are only available in kernel32.dll, we just
// need to add their prototype in the above `extern "C" {}` block, and add a
// one line `#define` below, in this form:
//     #define FunctionName(...) DO_K32_CALL(FunctionName, __VA_ARGS__)

static inline FARPROC GetK32Proc(_In_ LPCSTR procName) {
  static HMODULE kernel32Base = nullptr;
  if (kernel32Base == nullptr) {
    kernel32Base = GetModuleHandle(L"kernel32");
  }

  return GetProcAddress(kernel32Base, procName);
}

template <typename ProcPtrType, typename... ArgTypes>
auto CallK32Proc(char const *name, ArgTypes... args)
    -> decltype(((ProcPtrType) nullptr)(args...)) {
  auto procAddress = reinterpret_cast<ProcPtrType>(GetK32Proc(name));
  return procAddress(args...);
}

#define DO_K32_CALL(FUNC, ...) CallK32Proc<decltype(&FUNC)>(#FUNC, __VA_ARGS__)

#define CreateToolhelp32Snapshot(...)                                          \
  DO_K32_CALL(CreateToolhelp32Snapshot, __VA_ARGS__)
#define Thread32First(...) DO_K32_CALL(Thread32First, __VA_ARGS__)
#define Thread32Next(...) DO_K32_CALL(Thread32Next, __VA_ARGS__)

#endif

#endif // !__DebugServer2_Host_Windows_ExtraWrappers_h
