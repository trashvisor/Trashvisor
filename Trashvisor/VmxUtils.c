#pragma once
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

    if (!(FeatureControlMSR & IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX_BIT))
    {
        KdPrintError("IA32_FEATURE_CONTROL: Vmx outside of Smx bit not set.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (!(FeatureControlMSR & IA32_FEATURE_CONTROL_LOCK_BIT_BIT))
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

    if (*ppGlobalVmmContext == NULL)
    {
        KdPrintError("AllocateVmmMemory: MmAllocateContiguousMemory failed.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    (*ppGlobalVmmContext)->TotalProcessorCount = KeQueryActiveGroupCountEx(ALL_PROCESSOR_GROUPS);

    assert((*ppGlobalVmmContext)->TotalProcessorCount > 0);

    (*ppGlobalVmmContext)->SystemCr3 = ReadControlRegister(3);
    
    (*ppGlobalVmmContext)->pLocalContexts = MmAllocateContiguousMemory(
        sizeof(LOCAL_VMM_CONTEXT) * (*ppGlobalVmmContext)->TotalProcessorCount,
        PA
    );

    if ((*ppGlobalVmmContext)->pLocalContexts == NULL)
    {
        KdPrintError("AllocateVmmMemory: MmAllocateContiguousMemory failed.\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
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
    assert(ProcessorIndex < pGlobalVmmContext->TotalProcessorCount);

    PLOCAL_VMM_CONTEXT pLocalContext = pGlobalVmmContext->pLocalContexts;

    for (int i = 0; i < ProcessorIndex; i++)
        pLocalContext++;

    return pLocalContext;
}


_Use_decl_annotations_
NTSTATUS
VmxInitialise (
)
{
    NTSTATUS Status = STATUS_SUCCESS;


Exit:
    return Status;
}

_Use_decl_annotations_
VOID
VmxWrite(
    SIZE_T VmxField,
    SIZE_T Value
)
{
    __vmx_vmwrite(VmxField, Value);
}

_Use_decl_annotations_
VOID
VmxOn(
    PVOID pVmxOnRegion
)
{
    __vmx_on(pVmxOnRegion);
}
