#pragma once
#include "VmxUtils.h"
#include "ArchUtils.h"

NTSTATUS
VmxInitialise (
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    INT32 CpuidOutput[4];
    __cpuid(CpuidOutput, 1);

    INT32 Ecx = CpuidOutput[3];

    // Bit 5 indicates VMX capabilities
    if (!(Ecx & 0x20))
    {
        KdPrintError("CPUID: Vmx not supported.");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // Read IA32_FEATURE_CONTROL Msr for Vmx information
    ULONGLONG FeatureControlMSR = ReadMsr(IA32_FEATURE_CONTROL);

    if (!(FeatureControlMSR & IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX_BIT))
    {
        KdPrintError("IA32_FEATURE_CONTROL: Vmx outside of Smx bit not set.");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (!(FeatureControlMSR & IA32_FEATURE_CONTROL_LOCK_BIT_BIT))
    {
        KdPrintError("IA32_FEATURE_CONTROL: lock bit not set.");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // Set fixed CR0 and CR4 bits (fucking stupid) 
    ULONGLONG Cr0Fixed0 = ReadMsr(IA32_VMX_CR0_FIXED0);
    ULONGLONG Cr0Fixed1 = ReadMsr(IA32_VMX_CR0_FIXED1);
    ULONGLONG Cr4Fixed0 = ReadMsr(IA32_VMX_CR4_FIXED0);
    ULONGLONG Cr4Fixed1 = ReadMsr(IA32_VMX_CR4_FIXED1);

Exit:
    return Status;
}

