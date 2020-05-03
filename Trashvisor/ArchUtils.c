#pragma once
#include "ArchUtils.h"

_Use_decl_annotations_
VOID
CaptureRequiredGuestState(
    PMISC_REGISTER_STATE pMiscRegisterState
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

    return;
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
        assert(FALSE);

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
        assert(FALSE);
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
        assert(FALSE);
    }
}

_Use_decl_annotations_
ULONGLONG
ReadDebugRegister (
    _In_ INT32 DebugRegister
)
{
    return __readdr(DebugRegister);
}

_Use_decl_annotations_
VOID
WriteDebugRegister (
    INT32 DebugRegister,
    ULONGLONG Value
)
{
	__writedr(DebugRegister, Value);
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
