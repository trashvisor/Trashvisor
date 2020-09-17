#pragma once

#define MAX_PATH_LENGTH             256

#define NT_DEVICE_NAME              L"\\Device\\Trash"
#define DOS_DEVICE_NAME             L"\\DosDevices\\Trash"
#define WIN32_DEVICE_NAME           L"\\\\.\\Trash"

#define CODE_LOG_CPUID_PROCESS      0x800
#define CODE_LOG_EPT_HOOK           0x801

#define IOCTL_LOG_CPUID_PROCESS    CTL_CODE(FILE_DEVICE_UNKNOWN, CODE_LOG_CPUID_PROCESS, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) 
#define IOCTL_ADD_EPT_HOOK         CTL_CODE(FILE_DEVICE_UNKNOWN, CODE_LOG_EPT_HOOK,      METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct _INFO_IOCTL_CPUID_PROCESS
{
    WCHAR ProcessName[MAX_PATH_LENGTH];
    WCHAR FilePath[MAX_PATH_LENGTH];

    UINT32 LogCount;
} INFO_IOCTL_CPUID_PROCESS, *PINFO_IOCTL_CPUID_PROCESS;

typedef struct _INFO_IOCTL_EPT_HOOK_INFO
{
    CHAR   ModifiedPage[0x1000];
    UINT64 VirtualAddress;
    UINT64 ProcessId;
} INFO_IOCTL_EPT_HOOK_INFO, *PINFO_IOCTL_EPT_HOOK_INFO;
