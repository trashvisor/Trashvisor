#include "VmxUtils.h"
#include "VmxCore.h"
#include "ArchUtils.h"

_Use_decl_annotations_
VOID
NTAPI
VmxBroadcastInit (
	struct _KDPC* pDpc, 
	PVOID DeferredContext, 
	PVOID SystemArgument1, 
	PVOID SystemArgument2
)
{
	PGLOBAL_VMM_CONTEXT pGlobalVmmContext = (PGLOBAL_VMM_CONTEXT)DeferredContext;

	PLOCAL_VMM_CONTEXT pLocalVmmContext = RetrieveLocalContext(
		KeGetCurrentProcessorNumberEx(NULL),
		pGlobalVmmContext
	);

	// VmxInitialiseProcessor will set to FALSE if unsuccessful
	pLocalVmmContext->VmxActivated = TRUE;

	if (!VmxInitialiseProcessorStub(pLocalVmmContext))
		KdPrintError("VmxBroadcastInit: Failed.\n");

	/*
	if (pLocalVmmContext->VmxActivated)
		_InterlockedIncrement(&pGlobalVmmContext->ActivatedProcessorCount);
		*/
	KdPrintError("Logical processor %u successfully launched.\n", 
		KeGetCurrentProcessorNumberEx(NULL)
	);

	KeSignalCallDpcSynchronize(SystemArgument2);
	KeSignalCallDpcDone(SystemArgument1);
}

_Use_decl_annotations_
VOID
VmxInitialiseProcessor (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	ULONG_PTR GuestRip,
	ULONG_PTR GuestRsp
)
{
	if (!EnableVmx(pLocalVmmContext))
	{
		KdPrintError("VmxInitialiseProcessor failed for processor %u.\n", KeGetCurrentProcessorNumber());
		goto Exit;
	}

	CaptureState(pLocalVmmContext);

	SetupVmcs(pLocalVmmContext, GuestRip, GuestRsp);

	__vmx_vmlaunch();

	SIZE_T Error;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &Error);
	
	KdPrintError("Vmlaunch failed with: %d\n", Error);

Exit:
	pLocalVmmContext->VmxActivated = FALSE;
}

_Use_decl_annotations_
VOID
NTAPI
VmxBroadcastTeardown (
	struct _KDPC* pDpc,
	PVOID DeferredContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2
)
{
	// This should be done using a magic value + a forced vmexit
	// Maybe just use a value with cpuid and magic constants in the GP registers
}
