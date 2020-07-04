#pragma once

#define NT_DEVICE_NAME              L"\\Device\\Trash"
#define DOS_DEVICE_NAME             L"\\DosDevices\\Trash"
#define WIN32_DEVICE_NAME           L"\\\\.\\Trash"

#define CODE_LOG_CPUID_PROCESS      0x800

#define IOCTL_LOG_CPUID_PROCESS    CTL_CODE(FILE_DEVICE_UNKNOWN, CODE_LOG_CPUID_PROCESS, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) 
