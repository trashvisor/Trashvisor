#include "VmxUtils.h"
#include "ArchUtils.h"
#include "VmxHost.h"

_Must_inspect_result_
PGLOBAL_VMM_CONTEXT
AllocateGlobalVmmContext (
)
{

	PHYSICAL_ADDRESS HighestAcceptableAddress;
	HighestAcceptableAddress.QuadPart = MAXULONG64;

	PGLOBAL_VMM_CONTEXT pGlobalVmmContext = (PGLOBAL_VMM_CONTEXT)ExAllocatePoolWithTag(
		NonPagedPoolNx,
		sizeof(GLOBAL_VMM_CONTEXT),
		'MMVG'
	);

	if (pGlobalVmmContext == NULL)
	{
		KdPrintError("ALlocateGlobalVmmContext: Could not allocate pGlobalVmmContext memory.\n");
		goto Exit;
	}

	pGlobalVmmContext->LogicalProcessorCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

	ASSERT(pGlobalVmmContext->LogicalProcessorCount > 0);

	PLOCAL_VMM_CONTEXT *ppLocalVmmContexts = (PLOCAL_VMM_CONTEXT *)ExAllocatePoolWithTag(
		NonPagedPoolNx,
		sizeof(PLOCAL_VMM_CONTEXT) * pGlobalVmmContext->LogicalProcessorCount,
		'VMLP'
	);

	pGlobalVmmContext->ppLocalVmmContexts = ppLocalVmmContexts;

	for (ULONG i = 0u; i < pGlobalVmmContext->LogicalProcessorCount; i++)
	{
		PLOCAL_VMM_CONTEXT pCurrentVmmContext = AllocateLocalVmmContext(pGlobalVmmContext);

		if (pCurrentVmmContext == NULL)
			goto Exit;

		ppLocalVmmContexts[i] = pCurrentVmmContext;
	}

	pGlobalVmmContext->SystemCr3.Flags = __readcr3();

Exit:
	return pGlobalVmmContext;
}

_Must_inspect_result_
_Use_decl_annotations_
PLOCAL_VMM_CONTEXT
AllocateLocalVmmContext (
	PGLOBAL_VMM_CONTEXT pGlobalVmmContext	
)
{
	PHYSICAL_ADDRESS HighestPhysicalAddress;
	HighestPhysicalAddress.QuadPart = MAXULONG64;

	PLOCAL_VMM_CONTEXT pLocalVmmContext = (PLOCAL_VMM_CONTEXT)MmAllocateContiguousMemory(
		sizeof(LOCAL_VMM_CONTEXT),
		HighestPhysicalAddress
	);

	if (pLocalVmmContext == NULL)
	{
		KdPrintError("AllocateLocalVmmContext: Allocate failed.\n");
		goto Exit;
	}

	RtlZeroMemory(
		pLocalVmmContext,
		sizeof(LOCAL_VMM_CONTEXT)
	);

	pLocalVmmContext->pVmcs = AllocateVmcs();

	if (pLocalVmmContext->pVmcs == NULL)
		goto Exit;

	pLocalVmmContext->pVmxOn = AllocateVmxOn();

	if (pLocalVmmContext->pVmxOn == NULL)
		goto Exit;

	pLocalVmmContext->PhysVmcs = (PVOID)MmGetPhysicalAddress(pLocalVmmContext->pVmcs).QuadPart;
	pLocalVmmContext->PhysVmxOn = (PVOID)MmGetPhysicalAddress(pLocalVmmContext->pVmxOn).QuadPart;
	pLocalVmmContext->PhysMsrBitmap = (PVOID)MmGetPhysicalAddress(pLocalVmmContext->MsrBitmap).QuadPart;

	pLocalVmmContext->pGlobalVmmContext = pGlobalVmmContext;

Exit:
	return pLocalVmmContext;
}

_Must_inspect_result_
_Use_decl_annotations_
PVMCS
AllocateVmcs (
)
{
	PHYSICAL_ADDRESS HighestPhysicalAddress;
	HighestPhysicalAddress.QuadPart = MAXULONG64;

	PVMCS pVmcs = (PVMCS)MmAllocateContiguousMemory(
		sizeof(VMCS),
		HighestPhysicalAddress
	);

	if (pVmcs == NULL)
	{
		KdPrintError("AllocateVmcs: Allocate failed.\n");
		goto Exit;
	}

	RtlZeroMemory(
		pVmcs,
		sizeof(VMCS)
	);
	
	IA32_VMX_BASIC_REGISTER VmxBasicMsr;
	VmxBasicMsr.Flags = ReadMsr(IA32_VMX_BASIC);

	pVmcs->ShadowVmcsIndicator = 0;
	pVmcs->RevisionId = VmxBasicMsr.VmcsRevisionId;

Exit:
	return pVmcs;
}

_Must_inspect_result_
_Use_decl_annotations_
PVMXON
AllocateVmxOn (
)
{
	PHYSICAL_ADDRESS HighestPhysicalAddress;
	HighestPhysicalAddress.QuadPart = MAXULONG64;

	PVMXON pVmxOn = (PVMXON)MmAllocateContiguousMemory(
		sizeof(VMXON),
		HighestPhysicalAddress
	);

	if (pVmxOn == NULL)
	{
		KdPrintError("AllocateVmxOn: Allocate failed.\n");
		goto Exit;
	}

	RtlZeroMemory(
		pVmxOn,
		sizeof(VMXON)
	);

	IA32_VMX_BASIC_REGISTER VmxBasicMsr;
	VmxBasicMsr.Flags = ReadMsr(IA32_VMX_BASIC);

	pVmxOn->RevisionId = VmxBasicMsr.VmcsRevisionId;

Exit:
	return pVmxOn;
}

_Use_decl_annotations_
PLOCAL_VMM_CONTEXT
RetrieveLocalContext(
	ULONG ProcessorNumber,
	PGLOBAL_VMM_CONTEXT pGlobalVmmContext
)
{
	ASSERT(ProcessorNumber < pGlobalVmmContext->LogicalProcessorCount);

	return pGlobalVmmContext->ppLocalVmmContexts[ProcessorNumber];
}

_Use_decl_annotations_
VOID
CaptureMiscContext (
	PMISC_CONTEXT pMiscContext
)
{
	pMiscContext->Cr0.Flags = __readcr0();
	pMiscContext->Cr3.Flags = __readcr3();
	pMiscContext->Cr4.Flags = __readcr4();
	pMiscContext->Cr4.OsXsave = 1;
	
	pMiscContext->Dr7.Flags = __readdr(7);

	_sgdt(&pMiscContext->Gdtr.Limit);
	__sidt(&pMiscContext->Idtr.Limit);

	pMiscContext->Tr.Flags = _str();
	pMiscContext->Ldtr.Flags = _sldt();
}

_Use_decl_annotations_
BOOLEAN
EnableVmx (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext
)
{
	BOOLEAN bStatus = TRUE;

	ULONGLONG Cr0Fixed0 = ReadMsr(IA32_VMX_CR0_FIXED0);
	ULONGLONG Cr0Fixed1 = ReadMsr(IA32_VMX_CR0_FIXED1);
	ULONGLONG Cr4Fixed0 = ReadMsr(IA32_VMX_CR4_FIXED0);
	ULONGLONG Cr4Fixed1 = ReadMsr(IA32_VMX_CR4_FIXED1);

	CR0 Cr0;
	CR4 Cr4;

	Cr0.Flags = __readcr0();
	Cr4.Flags = __readcr4();

	Cr0.Flags |= Cr0Fixed0;
	Cr0.Flags &= Cr0Fixed1;

	Cr4.Flags |= Cr4Fixed0;
	Cr4.Flags &= Cr4Fixed1;

	Cr4.VmxEnable = 1;

	__writecr0(Cr0.Flags);
	__writecr4(Cr4.Flags);

	if (__vmx_on(&pLocalVmmContext->PhysVmxOn))
	{
		KdPrintError("EnableVmx: VmxOn failed.\n");
		bStatus = FALSE;
		goto Exit;
	}

	if (__vmx_vmclear(&pLocalVmmContext->PhysVmcs))
	{
		KdPrintError("EnableVmx: VmxVmClear failed.\n");
		bStatus = FALSE;
		goto Exit;
	}

	if (__vmx_vmptrld(&pLocalVmmContext->PhysVmcs))
	{
		KdPrintError("EnableVmx: VmxVmPtrLd failed.\n");
		bStatus = FALSE;
		goto Exit;
	}

Exit:
	return bStatus;
}

_Use_decl_annotations_
VOID
CaptureState (
	PLOCAL_VMM_CONTEXT pLocalVmmContext
)
{
	CaptureMiscContext(&pLocalVmmContext->MiscRegisterContext);
	RtlCaptureContext(&pLocalVmmContext->RegisterContext);
}

_Use_decl_annotations_
VOID
GetSegmentSetup (
	PSEGMENT_SETUP pSegmentSetup,
	UINT16 Selector,
	UINT64 GdtrBase
)
{
	SEGMENT_DESCRIPTOR_32 SegmentDesc = GetSegmentDescriptor(Selector, GdtrBase);

	pSegmentSetup->BaseAddress = GetSegmentBase(SegmentDesc);
	pSegmentSetup->SegmentLimit = GetSegmentLimit(Selector);
	pSegmentSetup->AccessRights = GetAccessRights(SegmentDesc);
}

_Use_decl_annotations_
VOID
GetSysSegmentSetup (
	PSEGMENT_SETUP pSegmentSetup,
	UINT16 Selector,
	UINT64 GdtrBase
)
{
	SEGMENT_DESCRIPTOR_64 SegmentDesc = GetSysSegmentDescriptor(Selector, GdtrBase);

	pSegmentSetup->BaseAddress = GetSysSegmentBase(SegmentDesc);
	pSegmentSetup->SegmentLimit = GetSegmentLimit(Selector);
	pSegmentSetup->AccessRights = GetSysAccessRights(SegmentDesc);
}

_Use_decl_annotations_
UINT32
GetAccessRights (
	SEGMENT_DESCRIPTOR_32 SegmentDesc
)
{
	VMCS_ACCESS_RIGHTS AccessRights;
	AccessRights.Flags = 0;
	
	AccessRights.SegmentType = SegmentDesc.Type;
	AccessRights.S = SegmentDesc.DescriptorType;
	AccessRights.DPL = SegmentDesc.DescriptorPrivilegeLevel;
	AccessRights.P = SegmentDesc.Present;
	AccessRights.Reserved1 = 0;
	AccessRights.AVL = SegmentDesc.System;
	AccessRights.L = SegmentDesc.LongMode;
	AccessRights.DB = SegmentDesc.DefaultBig;
	AccessRights.G = SegmentDesc.Granularity;
	AccessRights.Unusable = !AccessRights.P;

	return AccessRights.Flags;
}

_Use_decl_annotations_
UINT32
GetSysAccessRights (
	SEGMENT_DESCRIPTOR_64 SegmentDesc
)
{
	VMCS_ACCESS_RIGHTS AccessRights;
	AccessRights.Flags = 0;

	AccessRights.SegmentType = SegmentDesc.Type;
	AccessRights.S = SegmentDesc.DescriptorType;
	AccessRights.DPL = SegmentDesc.DescriptorPrivilegeLevel;
	AccessRights.P = SegmentDesc.Present;
	AccessRights.Reserved1 = 0;
	AccessRights.AVL = SegmentDesc.System;
	AccessRights.L = SegmentDesc.LongMode;
	AccessRights.DB = SegmentDesc.DefaultBig;
	AccessRights.G = SegmentDesc.Granularity;
	AccessRights.Unusable = !AccessRights.P;

	return AccessRights.Flags;
}

_Use_decl_annotations_
ULONG32
EnforceRequiredBits (
	ULONG64 RequiredBits,
	ULONG32 BitsToSet
)
{
	UINT32 Required0Bits = (RequiredBits & 0xFFFFFFFF);
	UINT32 Allowed1Bits = ((RequiredBits >> 32) & 0xFFFFFFFF);

	if (BitsToSet & Allowed1Bits != BitsToSet)
		KdPrintError("Bits to set: %u\n Allowed bits: %u\n", BitsToSet, Allowed1Bits);

	BitsToSet |= Required0Bits;
	BitsToSet &= Allowed1Bits;

	return BitsToSet;
}

VOID
GetVmxInstructionError (
)
{
	SIZE_T Error;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &Error);

	__debugbreak();
}
