#include "Vmcs.h"

_Use_decl_annotations_
VOID
SetupVmcs (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	ULONG_PTR GuestRip,
	ULONG_PTR GuestRsp
)
{
	IA32_VMX_BASIC_REGISTER BasicRegister;
	BasicRegister.Flags = ReadMsr(IA32_VMX_BASIC);
	
	BOOLEAN UseTrueControls = FALSE;

	if (BasicRegister.VmxControls)
		UseTrueControls = TRUE;

	SetupVmcsExecutionCtls(pLocalVmmContext, UseTrueControls);
	SetupVmcsVmEntryCtls(pLocalVmmContext, UseTrueControls);
	SetupVmcsVmExitCtls(pLocalVmmContext, UseTrueControls);

	SetupVmcsHostState(pLocalVmmContext);
	SetupVmcsGuestState(pLocalVmmContext, GuestRip, GuestRsp);
}

_Use_decl_annotations_
VOID
SetupVmcsGuestState (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	ULONG_PTR GuestRip,
	ULONG_PTR GuestRsp
)
{
	PCONTEXT pContext = &pLocalVmmContext->RegisterContext;
	PMISC_CONTEXT pMiscContext = &pLocalVmmContext->MiscRegisterContext;

	__vmx_vmwrite(VMCS_GUEST_CR0, pMiscContext->Cr0.Flags);
	__vmx_vmwrite(VMCS_GUEST_CR3, pMiscContext->Cr3.Flags);
	__vmx_vmwrite(VMCS_GUEST_CR4, pMiscContext->Cr4.Flags);
	__vmx_vmwrite(VMCS_GUEST_DR7, pMiscContext->Dr7.Flags);

	__vmx_vmwrite(VMCS_GUEST_RSP, GuestRsp);
	__vmx_vmwrite(VMCS_GUEST_RIP, GuestRip);
	__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());

	SEGMENT_SETUP Setup;

	GetSegmentSetup(&Setup, pContext->SegCs, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, pContext->SegCs);
	__vmx_vmwrite(VMCS_GUEST_CS_BASE, Setup.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, Setup.AccessRights);

	GetSegmentSetup(&Setup, pContext->SegDs, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, pContext->SegDs);
	__vmx_vmwrite(VMCS_GUEST_DS_BASE, Setup.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, Setup.AccessRights);

	GetSegmentSetup(&Setup, pContext->SegEs, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, pContext->SegEs);
	__vmx_vmwrite(VMCS_GUEST_ES_BASE, Setup.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, Setup.AccessRights);

	GetSegmentSetup(&Setup, pContext->SegFs, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, pContext->SegFs);
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, ReadMsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, Setup.AccessRights);

	GetSegmentSetup(&Setup, pContext->SegGs, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, pContext->SegGs);
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, ReadMsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, Setup.AccessRights);

	GetSegmentSetup(&Setup, pContext->SegSs, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, pContext->SegSs);
	__vmx_vmwrite(VMCS_GUEST_SS_BASE, Setup.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, Setup.AccessRights);

	GetSysSegmentSetup(&Setup, pMiscContext->Tr.Flags, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, pMiscContext->Tr.Flags);
	__vmx_vmwrite(VMCS_GUEST_TR_BASE, Setup.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, Setup.AccessRights);

	GetSysSegmentSetup(&Setup, pMiscContext->Ldtr.Flags, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, pMiscContext->Ldtr.Flags);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, Setup.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, Setup.SegmentLimit);
	__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, Setup.AccessRights);

	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, pMiscContext->Gdtr.Limit);

	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, pMiscContext->Idtr.BaseAddress);
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, pMiscContext->Idtr.Limit);

	__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0);
	__vmx_vmwrite(VMCS_GUEST_INTERRUPT_STATUS, 0);
	__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0);
	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);
}

_Use_decl_annotations_
VOID
SetupVmcsHostState (
	PLOCAL_VMM_CONTEXT pLocalVmmContext
)
{
	PCONTEXT pContext = &pLocalVmmContext->RegisterContext;
	PMISC_CONTEXT pMiscContext = &pLocalVmmContext->MiscRegisterContext;

	__vmx_vmwrite(VMCS_HOST_CR0, pMiscContext->Cr0.Flags);

	KdPrintError("Writing to host cr3: 0x%llx\n", pLocalVmmContext->pGlobalVmmContext->SystemCr3.Flags);

	__vmx_vmwrite(VMCS_HOST_CR3, pLocalVmmContext->pGlobalVmmContext->SystemCr3.Flags);
	__vmx_vmwrite(VMCS_HOST_CR4, pMiscContext->Cr4.Flags);

	__vmx_vmwrite(VMCS_HOST_RSP, &pLocalVmmContext->StackTop);
	__vmx_vmwrite(VMCS_HOST_RIP, VmxVmExitHandlerStub);

	#define MASK_OFF_RPL(x) (x & ~3)
	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, MASK_OFF_RPL(pContext->SegCs));
	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, MASK_OFF_RPL(pContext->SegDs));
	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, MASK_OFF_RPL(pContext->SegEs));
	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, MASK_OFF_RPL(pContext->SegFs));
	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, MASK_OFF_RPL(pContext->SegGs));
	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, MASK_OFF_RPL(pContext->SegSs));
	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, MASK_OFF_RPL(pMiscContext->Tr.Flags));

	SEGMENT_DESCRIPTOR_64 TssDescriptor = GetSysSegmentDescriptor(
		pMiscContext->Tr.Flags,
		pMiscContext->Gdtr.BaseAddress
	);

	__vmx_vmwrite(VMCS_HOST_FS_BASE, ReadMsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, ReadMsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_HOST_TR_BASE, GetSysSegmentBase(TssDescriptor));
	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, pMiscContext->Gdtr.BaseAddress);
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, pMiscContext->Idtr.BaseAddress);
}

_Use_decl_annotations_
VOID
SetupVmcsExecutionCtls (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	BOOLEAN UseTrue
)
{
	IA32_VMX_PINBASED_CTLS_REGISTER PinBasedRegister;
	IA32_VMX_PROCBASED_CTLS_REGISTER ProcBasedRegister;
	IA32_VMX_PROCBASED_CTLS2_REGISTER ProcBased2Register;
	
	if (UseTrue)
	{
		ProcBasedRegister.Flags = ReadMsr(IA32_VMX_TRUE_PROCBASED_CTLS);
		PinBasedRegister.Flags = ReadMsr(IA32_VMX_TRUE_PINBASED_CTLS);
	}
	else
	{
		ProcBasedRegister.Flags = ReadMsr(IA32_VMX_PROCBASED_CTLS);
		PinBasedRegister.Flags = ReadMsr(IA32_VMX_PINBASED_CTLS);
	}
	
	ProcBased2Register.Flags = ReadMsr(IA32_VMX_PROCBASED_CTLS2);

	ULONG32 PinBasedCtlsDesired = 0;
	ULONG32 ProcBasedCtlsDesired = IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_FLAG
		| IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG;

	ULONG32 ProcBasedCtls2Desired = IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_FLAG
		| IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG
		| IA32_VMX_PROCBASED_CTLS2_ENABLE_INVPCID_FLAG;

	__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
		EnforceRequiredBits(PinBasedRegister.Flags, PinBasedCtlsDesired));

	__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		EnforceRequiredBits(ProcBasedRegister.Flags, ProcBasedCtlsDesired));

	__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		EnforceRequiredBits(ProcBased2Register.Flags, ProcBasedCtls2Desired));
	
	__vmx_vmwrite(VMCS_CTRL_EXCEPTION_BITMAP, 0);
	__vmx_vmwrite(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK, 0);
	__vmx_vmwrite(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH, 0);

	__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, 0);

	__vmx_vmwrite(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0);
	__vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0);
	__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, pLocalVmmContext->MiscRegisterContext.Cr0.Flags);
	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, pLocalVmmContext->MiscRegisterContext.Cr4.Flags);

	__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, pLocalVmmContext->PhysMsrBitmap);
}

_Use_decl_annotations_
VOID
SetupVmcsVmExitCtls (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	BOOLEAN UseTrue
)
{
	IA32_VMX_EXIT_CTLS_REGISTER ExitCtlsRegister;

	if (UseTrue)
		ExitCtlsRegister.Flags = ReadMsr(IA32_VMX_TRUE_EXIT_CTLS);
	else
		ExitCtlsRegister.Flags = ReadMsr(IA32_VMX_EXIT_CTLS);

	ULONG32 ExitCtlsDesired = IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG;

	__vmx_vmwrite(VMCS_CTRL_VMEXIT_CONTROLS,
		EnforceRequiredBits(ExitCtlsRegister.Flags, ExitCtlsDesired));

	__vmx_vmwrite(VMCS_CTRL_VMEXIT_MSR_STORE_COUNT, 0);
	__vmx_vmwrite(VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT, 0);
}

_Use_decl_annotations_
VOID
SetupVmcsVmEntryCtls (
	PLOCAL_VMM_CONTEXT pLocalVmmContext,
	BOOLEAN UseTrue
)
{
	IA32_VMX_ENTRY_CTLS_REGISTER EntryCtlsRegister;

	if (UseTrue)
		EntryCtlsRegister.Flags = ReadMsr(IA32_VMX_TRUE_ENTRY_CTLS);
	else
		EntryCtlsRegister.Flags = ReadMsr(IA32_VMX_ENTRY_CTLS);

	ULONG32 EntryCtlsDesired = IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG;

	__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		EnforceRequiredBits(EntryCtlsRegister.Flags, EntryCtlsDesired));

	__vmx_vmwrite(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, 0);
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, 0);
}
