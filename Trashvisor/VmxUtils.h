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
VmxWrite(
    _In_ SIZE_T VmxField,
    _In_ SIZE_T Value
);
