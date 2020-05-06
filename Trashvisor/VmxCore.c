#include "VmxUtils.h"
#include "VmxCore.h"
#include "ArchUtils.h"

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

	// Enable CR4.VmxE
	WriteControlRegister(4, ReadControlRegister(4) | CR4_VMX_ENABLE_BIT);
	ASSERT(ReadControlRegister(4) & CR4_VMX_ENABLE_BIT);

    // Set fixed CR0 and CR4 bits (fucking stupid) 
    ULONGLONG Cr0Fixed0 = ReadMsr(IA32_VMX_CR0_FIXED0);
    ULONGLONG Cr0Fixed1 = ReadMsr(IA32_VMX_CR0_FIXED1);
    ULONGLONG Cr4Fixed0 = ReadMsr(IA32_VMX_CR4_FIXED0);
    ULONGLONG Cr4Fixed1 = ReadMsr(IA32_VMX_CR4_FIXED1);

    WriteControlRegister(0, ReadControlRegister(0) | Cr0Fixed0);
    WriteControlRegister(0, ReadControlRegister(0) & Cr0Fixed1);

    WriteControlRegister(4, ReadControlRegister(4) | Cr0Fixed0);
    WriteControlRegister(4, ReadControlRegister(4) & Cr0Fixed1);
	
	if (VmxOn(&pLocalVmmContext->PhysVmxOn))
	{
		KdPrintError("VmxOn failed.\n");
		goto Exit;
	}

	if (VmxVmClear(&pLocalVmmContext->PhysVmcs))
	{
		KdPrintError("VmxVmClear failed.\n");
		VmxVmOff();
		goto Exit;
	}

	if (VmxVmPtrLd(&pLocalVmmContext->PhysVmcs))
	{
		KdPrintError("VmxVmPtrLd failed.\n");
		VmxVmOff();
		goto Exit;
	}

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
	// Guest state
	VmxVmWrite(VMCS_GUEST_RSP, GuestRsp);
	VmxVmWrite(VMCS_GUEST_RIP, GuestRip);
	VmxVmWrite(VMCS_GUEST_RFLAGS, pLocalVmmContext->RegisterContext.EFlags);

	SEGMENT_DESCRIPTOR_64 SegCsDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->RegisterContext.SegCs,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	SEGMENT_DESCRIPTOR_64 SegDsDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->RegisterContext.SegDs,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	SEGMENT_DESCRIPTOR_64 SegEsDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->RegisterContext.SegEs,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	SEGMENT_DESCRIPTOR_64 SegFsDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->RegisterContext.SegFs,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	SEGMENT_DESCRIPTOR_64 SegGsDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->RegisterContext.SegGs,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	SEGMENT_DESCRIPTOR_64 SegSsDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->RegisterContext.SegSs,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	VmxVmWrite(VMCS_GUEST_CS_SELECTOR, pLocalVmmContext->RegisterContext.SegCs.Flags);
	VmxVmWrite(VMCS_GUEST_DS_SELECTOR, pLocalVmmContext->RegisterContext.SegDs.Flags);
	VmxVmWrite(VMCS_GUEST_ES_SELECTOR, pLocalVmmContext->RegisterContext.SegEs.Flags);
	VmxVmWrite(VMCS_GUEST_FS_SELECTOR, pLocalVmmContext->RegisterContext.SegFs.Flags);
	VmxVmWrite(VMCS_GUEST_GS_SELECTOR, pLocalVmmContext->RegisterContext.SegGs.Flags);
	VmxVmWrite(VMCS_GUEST_SS_SELECTOR, pLocalVmmContext->RegisterContext.SegSs.Flags);

	VmxVmWrite(VMCS_GUEST_CS_BASE, GetBaseFromSegDescriptor(SegCsDescriptor));
	VmxVmWrite(VMCS_GUEST_DS_BASE, GetBaseFromSegDescriptor(SegDsDescriptor));
	VmxVmWrite(VMCS_GUEST_ES_BASE, GetBaseFromSegDescriptor(SegEsDescriptor));
	VmxVmWrite(VMCS_GUEST_FS_BASE, GetBaseFromSegDescriptor(SegFsDescriptor));
	VmxVmWrite(VMCS_GUEST_GS_BASE, GetBaseFromSegDescriptor(SegGsDescriptor));
	VmxVmWrite(VMCS_GUEST_SS_BASE, GetBaseFromSegDescriptor(SegSsDescriptor));

	VmxVmWrite(VMCS_GUEST_CS_LIMIT, GetLimitFromSegDescriptor(SegCsDescriptor));
	VmxVmWrite(VMCS_GUEST_DS_LIMIT, GetLimitFromSegDescriptor(SegDsDescriptor));
	VmxVmWrite(VMCS_GUEST_ES_LIMIT, GetLimitFromSegDescriptor(SegEsDescriptor));
	VmxVmWrite(VMCS_GUEST_FS_LIMIT, GetLimitFromSegDescriptor(SegFsDescriptor));
	VmxVmWrite(VMCS_GUEST_GS_LIMIT, GetLimitFromSegDescriptor(SegGsDescriptor));
	VmxVmWrite(VMCS_GUEST_SS_LIMIT, GetLimitFromSegDescriptor(SegSsDescriptor));

	VmxVmWrite(VMCS_GUEST_CS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegCsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_DS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegDsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_ES_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegEsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_FS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegFsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_GS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegGsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_SS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegSsDescriptor).Flags);
	
	PMISC_REGISTER_CONTEXT pMiscRegisterContext = &pLocalVmmContext->MiscRegisterContext;

	VmxVmWrite(VMCS_GUEST_DR7, pMiscRegisterContext->Dr7.Flags);
	VmxVmWrite(VMCS_GUEST_CR0, pMiscRegisterContext->Cr0.Flags);
	VmxVmWrite(VMCS_GUEST_CR3, pMiscRegisterContext->Cr3.Flags);
	VmxVmWrite(VMCS_GUEST_CR4, pMiscRegisterContext->Cr4.Flags);

	VmxVmWrite(VMCS_GUEST_DEBUGCTL, pMiscRegisterContext->DebugCtlMsr.Flags);
	VmxVmWrite(VMCS_GUEST_SYSENTER_CS, pMiscRegisterContext->SysEnterCsMsr.Flags);
	VmxVmWrite(VMCS_GUEST_SYSENTER_ESP, pMiscRegisterContext->SysEnterEspMsr);
	VmxVmWrite(VMCS_GUEST_SYSENTER_EIP, pMiscRegisterContext->SysEnterEipMsr);
	VmxVmWrite(VMCS_GUEST_PERF_GLOBAL_CTRL, pMiscRegisterContext->PerfGlobalCtrlMsr.Flags);
	VmxVmWrite(VMCS_GUEST_EFER, pMiscRegisterContext->EferMsr.Flags);
	VmxVmWrite(VMCS_GUEST_RTIT_CTL, pMiscRegisterContext->RtitCtlMSr.Flags);
}
