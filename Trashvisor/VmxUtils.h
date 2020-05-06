#pragma once
#include "Header.h"
#include "VmxCore.h"

NTSTATUS
GetVmxCapability (
);

NTSTATUS
AllocateVmmMemory (
    _Inout_ PGLOBAL_VMM_CONTEXT* ppGlobalVmmContext
);

PLOCAL_VMM_CONTEXT
GetLocalVmmContext (
    _In_ PGLOBAL_VMM_CONTEXT pGlobalVmmContext,
    _In_ ULONG ProcessorIndex
);

VOID
VmxVmWrite (
    _In_ SIZE_T VmxField,
    _In_ SIZE_T Value
);

_Check_return_
UCHAR
VmxOn (
    _In_ PVOID pPhysVmxOnRegion
);

_Check_return_
UCHAR
VmxVmClear (
    _In_ PVOID pPhysVmcs
);

_Check_return_
UCHAR
VmxVmPtrLd (
    _In_ PVOID pPhysVmcs
);

UCHAR
VmxVmResume (
);

UCHAR
VmxVmLaunch (
);

VOID
VmxVmOff (
);
