#pragma once
#include "ArchUtils.h"

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
    INT CpuidIndex,
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
    INT ControlRegister,
    ULONGLONG Value
)
{
    switch (ControlRegister)
    {
    case 0:
        __writecr0(Value);
    case 2:
        __writecr2(Value);
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
    _In_ INT ControlRegister
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
