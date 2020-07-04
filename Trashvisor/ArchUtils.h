#pragma once
#include "Extern.h"

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
_str(
);

UINT16
_sldt(
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
HypervisorLoop (
);
