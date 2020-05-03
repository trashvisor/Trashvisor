#include "VmxUtils.h"

_Use_decl_annotations_
VOID
VmxBroadcastInit (
	struct _KDPC* Dpc,
	PVOID DeferredContext,
	PVOID SystemArgument1,
	PVOID SystemArgument2
)
{
	PGLOBAL_VMM_CONTEXT pGlobalVmmContext = (PGLOBAL_VMM_CONTEXT)DeferredContext;

	ULONG CurrentProcessorIndex = KeGetCurrentProcessorIndex();

	PLOCAL_VMM_CONTEXT pCurrentVmmContext = GetLocalVmmContext(
		pGlobalVmmContext, 
		CurrentProcessorIndex
	);

	if (!NT_SUCCESS(VmxInitialiseLogicalProcessorStub(pCurrentVmmContext)))
	{
		KdPrintError(
			"VmxBroadcastInit failed: VmxInitialiseLogicalProcessor could not complete for %u",
			CurrentProcessorIndex
		);
	}

	KeSignalCallDpcSynchronize(SystemArgument2);
	KeSignalCallDpcDone(SystemArgument1);
}

_Use_decl_annotations_
NTSTATUS
VmxInitialiseLogicalProcessor (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	ULONG_PTR GuestRip,
	ULONG_PTR GuestRsp
)
{
	NTSTATUS Status = STATUS_SUCCESS;

    // Set fixed CR0 and CR4 bits (fucking stupid) 
    ULONGLONG Cr0Fixed0 = ReadMsr(IA32_VMX_CR0_FIXED0);
    ULONGLONG Cr0Fixed1 = ReadMsr(IA32_VMX_CR0_FIXED1);
    ULONGLONG Cr4Fixed0 = ReadMsr(IA32_VMX_CR4_FIXED0);
    ULONGLONG Cr4Fixed1 = ReadMsr(IA32_VMX_CR4_FIXED1);

	WriteControlRegister(4, ReadControlRegister(4) | CR4_VMX_ENABLE_BIT);
	assert(ReadControlRegister(4) & CR4_VMX_ENABLE_BIT);

    WriteControlRegister(0, ReadControlRegister(0) | Cr0Fixed0);
    WriteControlRegister(0, ReadControlRegister(0) & Cr0Fixed1);

    WriteControlRegister(4, ReadControlRegister(4) | Cr0Fixed0);
    WriteControlRegister(4, ReadControlRegister(4) & Cr0Fixed1);

Exit:
	return Status;
}

_Use_decl_annotations_
NTSTATUS
VmxSetupVmcs (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	ULONG_PTR GuestRip,
	ULONG_PTR GuestRsp
)
{
	VmxWrite()
}
