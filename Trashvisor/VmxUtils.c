#include "ArchUtils.h"
#include "VmxUtils.h"

_Use_decl_annotations_
NTSTATUS
GetVmxCapability (
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    INT32 CpuidOutput[4];
    __cpuid(CpuidOutput, 1);

    INT32 Ecx = CpuidOutput[3];

    // Bit 5 indicates VMX capabilities
    if (!(Ecx & 0x20))
    {
        KdPrintError("CPUID: Vmx not supported.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // Read IA32_FEATURE_CONTROL Msr for Vmx information
    ULONGLONG FeatureControlMSR = ReadMsr(IA32_FEATURE_CONTROL);

    if (!IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX(FeatureControlMSR))
    {
        KdPrintError("FeatureControlMsr: 0x%x\n", FeatureControlMSR);
        KdPrintError("IA32_FEATURE_CONTROL: Vmx outside of Smx bit not set.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (!IA32_FEATURE_CONTROL_LOCK_BIT(FeatureControlMSR))
    {
        KdPrintError("IA32_FEATURE_CONTROL: Lock bit not set.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

Exit:
    return Status;
}

_Use_decl_annotations_
NTSTATUS
AllocateVmmMemory (
    PGLOBAL_VMM_CONTEXT *ppGlobalVmmContext
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PHYSICAL_ADDRESS PA;
    PA.QuadPart = MAXULONG64;

    *ppGlobalVmmContext = MmAllocateContiguousMemory(
        sizeof(GLOBAL_VMM_CONTEXT),
        PA
    );

    PGLOBAL_VMM_CONTEXT pGlobalVmmContext = *ppGlobalVmmContext;

    if (pGlobalVmmContext == NULL)
    {
        KdPrintError("AllocateVmmMemory: MmAllocateContiguousMemory failed.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    pGlobalVmmContext->TotalProcessorCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    ASSERT(pGlobalVmmContext->TotalProcessorCount > 0);

    pGlobalVmmContext->SystemCr3.Flags = ReadControlRegister(3);
    
    pGlobalVmmContext->pLocalContexts = MmAllocateContiguousMemory(
        sizeof(LOCAL_VMM_CONTEXT) * pGlobalVmmContext->TotalProcessorCount,
        PA
    );

    if (pGlobalVmmContext->pLocalContexts == NULL)
    {
        KdPrintError("AllocateVmmMemory: MmAllocateContiguousMemory failed.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    IA32_VMX_BASIC_REGISTER VmxBasicRegister;
    VmxBasicRegister.Flags = ReadMsr(IA32_VMX_BASIC);

    PLOCAL_VMM_CONTEXT pCurrentLocalVmmContext = pGlobalVmmContext->pLocalContexts;

    for (UINT32 i = 0u; i < pGlobalVmmContext->TotalProcessorCount; i++)
    {
        pCurrentLocalVmmContext->pVmcs = MmAllocateContiguousMemory(
            sizeof(VMCS),
            PA
        );

        if (pCurrentLocalVmmContext->pVmcs == NULL)
        {
            KdPrintError("AllocateVmmMemory: MmAllocateContiguousMemory failed for pVmcs.\n");
            Status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }
         
        pCurrentLocalVmmContext->pVmcs->RevisionId = VmxBasicRegister.VmcsRevisionId;
        pCurrentLocalVmmContext->pVmcs->ShadowVmcsIndicator = 0;

        pCurrentLocalVmmContext->PhysVmcs = MmGetPhysicalAddress(pCurrentLocalVmmContext->pVmcs);

        pCurrentLocalVmmContext->pVmxOn = MmAllocateContiguousMemory(
            sizeof(VMXON_REGION),
            PA
        );

        if (pCurrentLocalVmmContext->pVmxOn == NULL)
        {
            KdPrintError("AllocateVmmMemory: MmAllocateContiguousMemory failed for pVmxOn region.\n");
            Status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }

        pCurrentLocalVmmContext->pVmxOn->VmcsIdentifier = VmxBasicRegister.VmcsRevisionId;

        pCurrentLocalVmmContext->PhysVmxOn = MmGetPhysicalAddress(pCurrentLocalVmmContext->pVmxOn);
        
        pCurrentLocalVmmContext->pGlobalVmmContext = pGlobalVmmContext;

        pCurrentLocalVmmContext++;
    }

Exit:
    return Status;
}

_Use_decl_annotations_
PLOCAL_VMM_CONTEXT
GetLocalVmmContext (
    PGLOBAL_VMM_CONTEXT pGlobalVmmContext,
    ULONG ProcessorIndex
)
{
    ASSERT(ProcessorIndex < pGlobalVmmContext->TotalProcessorCount);

    PLOCAL_VMM_CONTEXT pLocalContext = pGlobalVmmContext->pLocalContexts;

    for (UINT32 i = 0u; i < ProcessorIndex; i++)
        pLocalContext++;

    return pLocalContext;
}

_Use_decl_annotations_
UINT32
RetrieveAllowedControls (
    UINT64 ControlFlag,
    UINT32 ControlsToSet
)
{
    UINT32 AllowedControls = 0;
    UINT32 Allowed1Controls = (ControlFlag >> 32);
    UINT32 Allowed0Controls = (ControlFlag & 0xffffffff);

    // Confirm that none of ControlsToSet are in the allowed 1 controls
    ASSERT((ControlsToSet & Allowed1Controls) == ControlsToSet);

    AllowedControls |= Allowed0Controls;
    AllowedControls &= Allowed1Controls;

    return AllowedControls;
}

_Use_decl_annotations_
VOID
VmxVmWrite (
    SIZE_T VmxField,
    SIZE_T Value
)
{
    __vmx_vmwrite(VmxField, Value);
}

_Use_decl_annotations_
VOID
VmxVmRead (
    SIZE_T VmxField,
    PSIZE_T Ret
)
{
    return __vmx_vmread(VmxField, Ret);
}

_Use_decl_annotations_
UCHAR
VmxOn (
    PVOID pVmxOnRegion
)
{
    return __vmx_on(pVmxOnRegion);
}

_Use_decl_annotations_
UCHAR
VmxVmClear (
    PVOID pPhysVmcs
)
{
    return __vmx_vmclear(pPhysVmcs);
}

_Use_decl_annotations_
UCHAR
VmxVmPtrLd (
    PVOID pPhysVmcs
)
{
    return __vmx_vmptrld(pPhysVmcs);
}

UCHAR
VmxVmResume (
)
{
    return __vmx_vmresume();
}

UCHAR
VmxVmLaunch (
)
{
    return __vmx_vmlaunch();
}

VOID
VmxVmOff (
)
{
    __vmx_off();
}
