#pragma once
#include "Extern.h"
#include "Shared.h"
#include <ntimage.h>

typedef struct _SEGMENT_SETUP
{
    UINT64 BaseAddress;
    UINT32 SegmentLimit;
    UINT32 AccessRights;
} SEGMENT_SETUP, *PSEGMENT_SETUP;

BOOLEAN
IsVmxSupported (
);

VOID
KdPrintError (
    _In_ LPCSTR Message,
    _In_ ...
);

ULONGLONG
ReadMsr (
    _In_ ULONG Msr
);

UINT16
_str (
);

UINT16
_sldt (
);

SEGMENT_DESCRIPTOR_32
GetSegmentDescriptor (
    _In_ UINT16 Selector,
    _In_ UINT64 GdtrBase
);

SEGMENT_DESCRIPTOR_64
GetSysSegmentDescriptor (
    _In_ UINT16 Seledctor,
    _In_ UINT64 GdtrBase
);

UINT32
GetSegmentBase (
    _In_ SEGMENT_DESCRIPTOR_32 SegmentDescriptor
);

UINT64
GetSysSegmentBase (
    _In_ SEGMENT_DESCRIPTOR_64 SysSegment
);

VOID
HypervisorBreak (
);

typedef struct _PEB_LDR_DATA
{
    UINT32		Length;
    UCHAR		Initialised;
    PVOID		SsHandle;
    LIST_ENTRY	InLoadOrderModuleList;
    LIST_ENTRY	InMemOrderModuleList;
    LIST_ENTRY	InInitOrderModuleList;
    PVOID		EntryInProgress;
    UCHAR		ShutDownInProgress;
    PVOID		ShutDownThreadId;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _LDR_MODULE
{
    LIST_ENTRY              InLoadOrderModuleList;
    LIST_ENTRY              InMemOrderModuleList;
    LIST_ENTRY              InInitializationOrderModuleList;
    PVOID                   BaseAddress;
    PVOID                   EntryPoint;
    ULONG                   SizeOfImage;
    UNICODE_STRING          FullDllName;
    UNICODE_STRING          BaseDllName;
    ULONG                   Flags;
    SHORT                   LoadCount;
    SHORT                   TlsIndex;
    LIST_ENTRY              HashTableEntry;
    ULONG                   TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment
} KAPC_ENVIRONMENT, *PKAPC_ENVIRONMENT;

NTSYSAPI
PIMAGE_NT_HEADERS
RtlImageNtHeader (
    _In_ PVOID BaseAddress
);

NTSYSAPI
VOID
KeInitializeApc (
    _In_ PKAPC pAPC,
    _In_ PKTHREAD pThread,
    _In_ KAPC_ENVIRONMENT TargetEnvironment,
    _In_ PVOID KernelRoutine,
    _In_ PVOID NormalRoutine,
    _In_ KPROCESSOR_MODE ApcMode,
    _In_ PVOID pNormalContext
);

NTSYSAPI
KeInsertQueueApc (
    _In_ PKAPC pAPC,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2,
    _In_ KPRIORITY Increment
);
