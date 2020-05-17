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

	ASSERT(pGlobalVmmContext != NULL);

	ULONG CurrentProcessorIndex = KeGetCurrentProcessorIndex();

	PLOCAL_VMM_CONTEXT pCurrentVmmContext = GetLocalVmmContext(
		pGlobalVmmContext, 
		CurrentProcessorIndex
	);

	CaptureMiscRegisterContext(&pCurrentVmmContext->MiscRegisterContext);

	CaptureRegisterContext(&pCurrentVmmContext->RegisterContext);

	IA32_VMX_BASIC_REGISTER VmxCapabilities;
	VmxCapabilities.Flags = ReadMsr(IA32_VMX_BASIC);

	pCurrentVmmContext->pVmcs->RevisionId = VmxCapabilities.VmcsRevisionId;
	pCurrentVmmContext->pVmcs->ShadowVmcsIndicator = 0;
	pCurrentVmmContext->pVmxOn->VmcsIdentifier = VmxCapabilities.VmcsRevisionId;

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
	__debugbreak();
	WriteControlRegister(4, ReadControlRegister(4) | CR4_VMX_ENABLE_FLAG);
	ASSERT(ReadControlRegister(4) & CR4_VMX_ENABLE_FLAG);

    // Set fixed CR0 and CR4 bits (fucking stupid) 
    ULONGLONG Cr0Fixed0 = ReadMsr(IA32_VMX_CR0_FIXED0);
    ULONGLONG Cr0Fixed1 = ReadMsr(IA32_VMX_CR0_FIXED1);
    ULONGLONG Cr4Fixed0 = ReadMsr(IA32_VMX_CR4_FIXED0);
    ULONGLONG Cr4Fixed1 = ReadMsr(IA32_VMX_CR4_FIXED1);

    WriteControlRegister(0, ReadControlRegister(0) | Cr0Fixed0);
    WriteControlRegister(0, ReadControlRegister(0) & Cr0Fixed1);

    WriteControlRegister(4, ReadControlRegister(4) | Cr4Fixed0);
    WriteControlRegister(4, ReadControlRegister(4) & Cr4Fixed1);

	//__debugbreak();
	
	if (VmxOn(&pLocalVmmContext->PhysVmxOn))
	{
		Status = STATUS_UNSUCCESSFUL;
		KdPrintError("VmxOn failed.\n");
		goto Exit;
	}

	if (VmxVmClear(&pLocalVmmContext->PhysVmcs))
	{
		Status = STATUS_UNSUCCESSFUL;
		KdPrintError("VmxVmClear failed.\n");
		VmxVmOff();
		goto Exit;
	}

	if (VmxVmPtrLd(&pLocalVmmContext->PhysVmcs))
	{
		Status = STATUS_UNSUCCESSFUL;
		KdPrintError("VmxVmPtrLd failed.\n");
		VmxVmOff();
		goto Exit;
	}

	VmxSetupVmcs(pLocalVmmContext, GuestRip, GuestRsp);

	if (VmxVmLaunch())
	{
		SIZE_T InstructionError;
		VmxVmRead(VMCS_VM_INSTRUCTION_ERROR, &InstructionError);
		KdPrintError("Failed to launch: 0x%llx\n", InstructionError);
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

Exit:
	return Status;
}

_Use_decl_annotations_
VOID
VmxSetupVmcs (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	ULONG_PTR GuestRip,
	ULONG_PTR GuestRsp
)
{
	// Guest-state
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

	SEGMENT_DESCRIPTOR_64 TssDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->MiscRegisterContext.Tr,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	SEGMENT_DESCRIPTOR_64 LdtDescriptor = GetDescriptorFromSelector(
		pLocalVmmContext->MiscRegisterContext.Ldtr,
		pLocalVmmContext->MiscRegisterContext.Gdtr
	);

	VmxVmWrite(VMCS_GUEST_CS_SELECTOR, pLocalVmmContext->RegisterContext.SegCs.Flags);
	VmxVmWrite(VMCS_GUEST_DS_SELECTOR, pLocalVmmContext->RegisterContext.SegDs.Flags);
	VmxVmWrite(VMCS_GUEST_ES_SELECTOR, pLocalVmmContext->RegisterContext.SegEs.Flags);
	VmxVmWrite(VMCS_GUEST_FS_SELECTOR, pLocalVmmContext->RegisterContext.SegFs.Flags);
	VmxVmWrite(VMCS_GUEST_GS_SELECTOR, pLocalVmmContext->RegisterContext.SegGs.Flags);
	VmxVmWrite(VMCS_GUEST_SS_SELECTOR, pLocalVmmContext->RegisterContext.SegSs.Flags);
	VmxVmWrite(VMCS_GUEST_TR_SELECTOR, pLocalVmmContext->MiscRegisterContext.Tr.Flags);
	VmxVmWrite(VMCS_GUEST_LDTR_SELECTOR, pLocalVmmContext->MiscRegisterContext.Ldtr.Flags);

	VmxVmWrite(VMCS_GUEST_CS_BASE, GetBaseFromSegDescriptor(SegCsDescriptor));
	VmxVmWrite(VMCS_GUEST_DS_BASE, GetBaseFromSegDescriptor(SegDsDescriptor));
	VmxVmWrite(VMCS_GUEST_ES_BASE, GetBaseFromSegDescriptor(SegEsDescriptor));
	VmxVmWrite(VMCS_GUEST_FS_BASE, GetBaseFromSegDescriptor(SegFsDescriptor));
	VmxVmWrite(VMCS_GUEST_GS_BASE, GetBaseFromSegDescriptor(SegGsDescriptor));
	VmxVmWrite(VMCS_GUEST_SS_BASE, GetBaseFromSegDescriptor(SegSsDescriptor));
	VmxVmWrite(VMCS_GUEST_TR_BASE, GetBaseFromSegDescriptor(TssDescriptor));
	VmxVmWrite(VMCS_GUEST_LDTR_BASE, GetBaseFromSegDescriptor(LdtDescriptor));

	VmxVmWrite(VMCS_GUEST_CS_LIMIT, GetLimitFromSegDescriptor(SegCsDescriptor));
	VmxVmWrite(VMCS_GUEST_DS_LIMIT, GetLimitFromSegDescriptor(SegDsDescriptor));
	VmxVmWrite(VMCS_GUEST_ES_LIMIT, GetLimitFromSegDescriptor(SegEsDescriptor));
	VmxVmWrite(VMCS_GUEST_FS_LIMIT, GetLimitFromSegDescriptor(SegFsDescriptor));
	VmxVmWrite(VMCS_GUEST_GS_LIMIT, GetLimitFromSegDescriptor(SegGsDescriptor));
	VmxVmWrite(VMCS_GUEST_SS_LIMIT, GetLimitFromSegDescriptor(SegSsDescriptor));
	VmxVmWrite(VMCS_GUEST_TR_LIMIT, GetLimitFromSegDescriptor(TssDescriptor));
	VmxVmWrite(VMCS_GUEST_LDTR_LIMIT, GetLimitFromSegDescriptor(LdtDescriptor));

	VmxVmWrite(VMCS_GUEST_CS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegCsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_DS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegDsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_ES_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegEsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_FS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegFsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_GS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegGsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_SS_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(SegSsDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_TR_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(TssDescriptor).Flags);
	VmxVmWrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, GetAccessRightsFromSegDescriptor(LdtDescriptor).Flags);

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

	// Guest non-register state
	VmxVmWrite(VMCS_GUEST_VMCS_LINK_POINTER, 0xffffffffffffffffULL);

	// Host-state area
	VmxVmWrite(VMCS_HOST_CR0, pMiscRegisterContext->Cr0.Flags);
	VmxVmWrite(VMCS_HOST_CR3, pLocalVmmContext->pGlobalVmmContext->SystemCr3.Flags);
	VmxVmWrite(VMCS_HOST_CR4, pMiscRegisterContext->Cr4.Flags);

	VmxVmWrite(VMCS_HOST_RSP, GuestRsp);
	VmxVmWrite(VMCS_HOST_RIP, GuestRip);

	VmxVmWrite(VMCS_HOST_CS_SELECTOR, pLocalVmmContext->RegisterContext.SegCs.Flags & ~HOST_SELECTOR_RPL_MASK);
	VmxVmWrite(VMCS_HOST_DS_SELECTOR, pLocalVmmContext->RegisterContext.SegDs.Flags & ~HOST_SELECTOR_RPL_MASK);
	VmxVmWrite(VMCS_HOST_ES_SELECTOR, pLocalVmmContext->RegisterContext.SegEs.Flags & ~HOST_SELECTOR_RPL_MASK);
	VmxVmWrite(VMCS_HOST_FS_SELECTOR, pLocalVmmContext->RegisterContext.SegFs.Flags & ~HOST_SELECTOR_RPL_MASK);
	VmxVmWrite(VMCS_HOST_GS_SELECTOR, pLocalVmmContext->RegisterContext.SegGs.Flags & ~HOST_SELECTOR_RPL_MASK);
	VmxVmWrite(VMCS_HOST_SS_SELECTOR, pLocalVmmContext->RegisterContext.SegSs.Flags & ~HOST_SELECTOR_RPL_MASK);
	VmxVmWrite(VMCS_HOST_TR_SELECTOR, pLocalVmmContext->MiscRegisterContext.Tr.Flags & ~HOST_SELECTOR_RPL_MASK);

	VmxVmWrite(VMCS_HOST_FS_BASE, GetBaseFromSegDescriptor(SegFsDescriptor));
	VmxVmWrite(VMCS_HOST_GS_BASE, GetBaseFromSegDescriptor(SegGsDescriptor));
	VmxVmWrite(VMCS_HOST_TR_BASE, GetBaseFromSegDescriptor(TssDescriptor));
	VmxVmWrite(VMCS_HOST_GDTR_BASE, pLocalVmmContext->MiscRegisterContext.Gdtr.BaseAddress);
	VmxVmWrite(VMCS_HOST_IDTR_BASE, pLocalVmmContext->MiscRegisterContext.Idtr.BaseAddress);

	VmxVmWrite(VMCS_HOST_SYSENTER_CS, pLocalVmmContext->MiscRegisterContext.SysEnterCsMsr.Flags);
	VmxVmWrite(VMCS_HOST_SYSENTER_EIP, pLocalVmmContext->MiscRegisterContext.SysEnterEipMsr);
	VmxVmWrite(VMCS_HOST_SYSENTER_ESP, pLocalVmmContext->MiscRegisterContext.SysEnterEspMsr);
	
	// Pinbased/ProcBased/VmEntry/VmExit controls
	IA32_VMX_BASIC_REGISTER VmxCapabilities;
	VmxCapabilities.Flags = ReadMsr(IA32_VMX_BASIC);

	IA32_VMX_PINBASED_CTLS_REGISTER PinbasedCtlsRegister;
	IA32_VMX_ENTRY_CTLS_REGISTER EntryCtlsRegister;
	IA32_VMX_EXIT_CTLS_REGISTER ExitCtlsRegister;
	IA32_VMX_PROCBASED_CTLS_REGISTER ProcbasedCtlsRegister;
	IA32_VMX_PROCBASED_CTLS2_REGISTER ProcbasedCtls2Register;

	if (VmxCapabilities.VmxControls)
	{
		PinbasedCtlsRegister.Flags = ReadMsr(IA32_VMX_TRUE_PINBASED_CTLS);
		EntryCtlsRegister.Flags = ReadMsr(IA32_VMX_TRUE_ENTRY_CTLS);
		ExitCtlsRegister.Flags = ReadMsr(IA32_VMX_TRUE_EXIT_CTLS);
		ProcbasedCtlsRegister.Flags = ReadMsr(IA32_VMX_TRUE_PROCBASED_CTLS);
	}
	else
	{
		PinbasedCtlsRegister.Flags = ReadMsr(IA32_VMX_PINBASED_CTLS);
		EntryCtlsRegister.Flags = ReadMsr(IA32_VMX_ENTRY_CTLS);
		ExitCtlsRegister.Flags = ReadMsr(IA32_VMX_EXIT_CTLS);
		ProcbasedCtlsRegister.Flags = ReadMsr(IA32_VMX_PROCBASED_CTLS);
	}

	ProcbasedCtls2Register.Flags = ReadMsr(IA32_VMX_PROCBASED_CTLS2);

	UINT32 PinbasedCtlsToSet = 0;

	UINT32 ProcbasedCtlsToSet = IA32_VMX_PROCBASED_CTLS_RDTSC_EXITING_FLAG
		| IA32_VMX_PROCBASED_CTLS_MOV_DR_EXITING_FLAG
		| IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG;

	// Apparently hide from PT isn't supported by my processor? Ok then...
	UINT32 ProcbasedCtrls2ToSet = IA32_VMX_PROCBASED_CTLS2_RDRAND_EXITING_FLAG
		| IA32_VMX_PROCBASED_CTLS2_RDSEED_EXITING_FLAG
		| IA32_VMX_PROCBASED_CTLS2_ENABLE_VM_FUNCTIONS_FLAG
		| IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG;
		//| IA32_VMX_PROCBASED_CTLS2_CONCEAL_VMX_FROM_PT_FLAG

	UINT32 VmExitCtlsToSet = IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG;
		//| IA32_VMX_EXIT_CTLS_CONCEAL_VMX_FROM_PT_FLAG;

	UINT32 VmEntryCtlsToSet = IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG;
		//| IA32_VMX_ENTRY_CTLS_CONCEAL_VMX_FROM_PT_FLAG;

	VmxVmWrite(
		VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
		RetrieveAllowedControls(
			PinbasedCtlsRegister.Flags,
			PinbasedCtlsToSet
		)
	);

	VmxVmWrite(
		VMCS_CTRL_VMENTRY_CONTROLS, 
		RetrieveAllowedControls(
			EntryCtlsRegister.Flags,
			VmEntryCtlsToSet
		)
	);

	VmxVmWrite(
		VMCS_CTRL_VMEXIT_CONTROLS,
		RetrieveAllowedControls(
			ExitCtlsRegister.Flags,
			VmExitCtlsToSet
		)
	);

	VmxVmWrite(
		VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		RetrieveAllowedControls(
			ProcbasedCtlsRegister.Flags, 
			ProcbasedCtlsToSet
		)
	);

	VmxVmWrite(
		VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, 
		RetrieveAllowedControls(
			ProcbasedCtls2Register.Flags,
			ProcbasedCtrls2ToSet
		)
	);

}
