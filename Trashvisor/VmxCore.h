#pragma once
#include "VmxUtils.h"
#include "Vmcs.h"

VOID
NTAPI
VmxBroadcastInit (
    _In_ struct _KDPC* pDpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
);

BOOLEAN
VmxInitialiseProcessorStub (
    _In_ PLOCAL_VMM_CONTEXT pLocalVmmContext
);

VOID
VmxInitialiseProcessor (
    _In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
    _In_ ULONG_PTR GuestRip,
    _In_ ULONG_PTR GuestRsp
);

VOID
NTAPI
VmxBroadcastTeardown (
    _In_ struct _KDPC* pDpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
);
