#pragma once
#include "Header.h"
#include "ArchUtils.h"

typedef struct _VMXON_REGION
{
	ULONG VmcsIdentifier;
} VMXON_REGION, *PVMXON_REGION;

typedef struct VMCS* PVMCS;

typedef struct _LOCAL_VMM_CONTEXT
{
	BOOLEAN VmxLaunched;
	PVMXON_REGION pVmxOn;
	PVMCS pVmcs;
	PHYSICAL_ADDRESS PhysVmxOn;
	PHYSICAL_ADDRESS PhysVmcs;
	MISC_REGISTER_CONTEXT MiscRegisterContext;
	REGISTER_CONTEXT RegisterContext;
} LOCAL_VMM_CONTEXT, *PLOCAL_VMM_CONTEXT;

typedef struct _GLOBAL_VMM_CONTEXT
{
	UINT32 TotalProcessorCount;
	PLOCAL_VMM_CONTEXT pLocalContexts;
	ULONG64 SystemCr3;
} GLOBAL_VMM_CONTEXT, *PGLOBAL_VMM_CONTEXT;

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
VmxBroadcastInit (
	_In_ struct _KDPC *Dpc,
	_In_opt_ PVOID DeferredContext,
	_In_opt_ PVOID SystemArgument1,
	_In_opt_ PVOID SystemArgument2
);

EXTERN_C
NTSTATUS
VmxInitialiseLogicalProcessorStub (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext
);

NTSTATUS
VmxInitialiseLogicalProcessor (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
	_In_ ULONG_PTR GuestRip,
	_In_ ULONG_PTR GuestRsp
);

NTSTATUS
VmxSetupVmcs (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
	_In_ ULONG_PTR GuestRip,
	_In_ ULONG_PTR GuestRsp
);
