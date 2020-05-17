#pragma once
#include "Header.h"
#include "VmxCore.h"

_Check_return_
NTSTATUS
GetVmxCapability (
);

_Check_return_
NTSTATUS
AllocateVmmMemory (
    _Inout_ PGLOBAL_VMM_CONTEXT* ppGlobalVmmContext
);

_Check_return_
PLOCAL_VMM_CONTEXT
GetLocalVmmContext (
    _In_ PGLOBAL_VMM_CONTEXT pGlobalVmmContext,
    _In_ ULONG ProcessorIndex
);

UINT32
RetrieveAllowedControls (
    _In_ UINT64 ControlFlags,
    _In_ UINT32 ControlsToSet
);

VOID
VmxVmWrite (
    _In_ SIZE_T VmxField,
    _In_ SIZE_T Value
);

VOID
VmxVmRead (
    _In_ SIZE_T VmxField,
    _Out_ PSIZE_T pRet
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
