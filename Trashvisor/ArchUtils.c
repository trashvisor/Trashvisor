#include "ArchUtils.h"
#include "VmxUtils.h"

_Use_decl_annotations_
VOID
CaptureRequiredGuestState (
    PMISC_REGISTER_CONTEXT pMiscRegisterState
)
{
    pMiscRegisterState->Cr0.Flags = ReadControlRegister(0);
    pMiscRegisterState->Cr3.Flags = ReadControlRegister(3);
    pMiscRegisterState->Cr4.Flags = ReadControlRegister(4);

    pMiscRegisterState->Dr7.Flags = ReadDebugRegister(7);

    _sgdt(&pMiscRegisterState->Gdtr.Limit);
    __sidt(&pMiscRegisterState->Idtr.Limit);

    _str(&pMiscRegisterState->Tr);
    _sldt(&pMiscRegisterState->Ldtr);

    pMiscRegisterState->DebugCtlMsr.Flags = ReadMsr(IA32_DEBUGCTL);
    pMiscRegisterState->PatMsr.Flags = ReadMsr(IA32_PAT);
    pMiscRegisterState->BndCfgsMsr.Flags = ReadMsr(IA32_BNDCFGS);
    pMiscRegisterState->EferMsr.Flags = ReadMsr(IA32_EFER);
    pMiscRegisterState->SysEnterCsMsr.Flags = ReadMsr(IA32_SYSENTER_CS);
    pMiscRegisterState->SysEnterEipMsr = ReadMsr(IA32_SYSENTER_EIP);
    pMiscRegisterState->SysEnterEspMsr = ReadMsr(IA32_SYSENTER_ESP);
    pMiscRegisterState->PerfGlobalCtrlMsr.Flags = ReadMsr(IA32_PERF_GLOBAL_CTRL);

    return;
}

_Use_decl_annotations_
SEGMENT_DESCRIPTOR_64 
GetDescriptorFromSelector (
    SEGMENT_SELECTOR Selector,
    SEGMENT_DESCRIPTOR_REGISTER_64 Gdtr
)
{
    SEGMENT_DESCRIPTOR_64 Desc = *(PSEGMENT_DESCRIPTOR_64)(Gdtr.BaseAddress + (Selector.Index << 3));
    return Desc;
}

_Use_decl_annotations_
UINT32
GetBaseFromSegDescriptor (
    SEGMENT_DESCRIPTOR_64 Descriptor
)
{
    return (Descriptor.BaseAddressHigh << 24 
        | Descriptor.BaseAddressMiddle << 16 
        | Descriptor.BaseAddressLow);
}

_Use_decl_annotations_
UINT32
GetLimitFromSegDescriptor (
    SEGMENT_DESCRIPTOR_64 Descriptor
)
{
    return (Descriptor.SegmentLimitHigh << 16 | Descriptor.SegmentLimitLow);
}

_Use_decl_annotations_
VMCS_GUEST_ACCESS_RIGHTS
GetAccessRightsFromSegDescriptor (
    SEGMENT_DESCRIPTOR_64 Descriptor
)
{
    VMCS_GUEST_ACCESS_RIGHTS AccessRights;
    
    AccessRights.SegmentType = Descriptor.Type;
    AccessRights.DescriptorType = Descriptor.DescriptorType;
    AccessRights.DPL = Descriptor.DescriptorPrivilegeLevel;
    AccessRights.Present = Descriptor.Present;
    AccessRights.AVL = Descriptor.System;
    AccessRights.Long = Descriptor.LongMode;
    AccessRights.DefaultOpSize = Descriptor.DefaultBig;
    AccessRights.Granularity = Descriptor.Granularity;
    AccessRights.SegmentUnusable = 0;
   
    return AccessRights;
}

_Use_decl_annotations_
ULONGLONG
ReadMsr (
    ULONG Msr
)
{
    return __readmsr(Msr);
}

_Use_decl_annotations_
VOID
WriteMsr (
    ULONG Msr,
    ULONG Value
)
{
    __writemsr(Value, Msr);
}

_Use_decl_annotations_
VOID
Cpuid (
    INT32 CpuidIndex,
    PINT32 pCpuidOut
)
{
    if (pCpuidOut == NULL)
        ASSERT(FALSE);

    __cpuid(pCpuidOut, CpuidIndex);
}

_Use_decl_annotations_
VOID
WriteControlRegister (
    INT32 ControlRegister,
    ULONGLONG Value
)
{
    switch (ControlRegister)
    {
    case 0:
        __writecr0(Value);
    case 3:
        __writecr3(Value);
    case 4:
        __writecr4(Value);
    case 8:
        __writecr8(Value);
    default:
        ASSERT(FALSE);
    }
}

_Use_decl_annotations_
ULONGLONG
ReadControlRegister (
    INT32 ControlRegister
)
{
    switch (ControlRegister)
    {
    case 0:
        return __readcr0();
    case 2:
        return __readcr2();
    case 3:
        return __readcr3();
    case 4:
        return __readcr4();
    default:
        ASSERT(FALSE);
    }
}

_Use_decl_annotations_
ULONGLONG
ReadDebugRegister (
    CONST UINT32 DebugRegister
)
{
    switch (DebugRegister)
    {
    case 0:
        return __readdr(0);
    case 1:
        return __readdr(1);
    case 2:
        return __readdr(2);
    case 3:
        return __readdr(3);
    case 6:
        return __readdr(6);
    case 7:
        return __readdr(7);
    default:
        ASSERT(FALSE);
    }
}

_Use_decl_annotations_
VOID
WriteDebugRegister (
    UINT32 DebugRegister,
    ULONGLONG Value
)
{
    switch (DebugRegister)
    {
    case 0:
        __writedr(0, Value);
    case 1:
        __writedr(1, Value);
    case 2:
        __writedr(2, Value);
    case 3:
        __writedr(3, Value);
    case 6:
        __writedr(6, Value);
    case 7:
        __writedr(7, Value);
    default:
        ASSERT(FALSE);
    }
}

_Use_decl_annotations_
VOID
KdPrintError (
    LPCSTR ErrorMessage,
    ...
)
{
    va_list ArgumentList;
    va_start(ArgumentList, ErrorMessage);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, ErrorMessage, ArgumentList);

    va_end(ArgumentList);
}

_Use_decl_annotations_
VOID
KdPrintTrace (
    _In_z_ LPCSTR TraceMessage,
    ...
)
{
    va_list ArgumentList;
    va_start(ArgumentList, TraceMessage);

    DbgPrint(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, TraceMessage, ArgumentList);

    va_end(ArgumentList);
}
