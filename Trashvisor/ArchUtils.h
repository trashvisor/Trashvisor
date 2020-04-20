#pragma once
#include "Header.h"

ULONGLONG
ReadMsr (
    _In_ ULONG Msr
);

VOID
WriteMsr (
    _In_ ULONG Msr,
    _In_ ULONG Value
);

VOID
Cpuid (
    _In_ INT CpuidIndex,
    _Out_ PINT32 pCpuidOut
);

VOID
WriteControlRegister (
    _In_ INT ControlRegister,
    _In_ ULONGLONG Value 
);

ULONGLONG
ReadControlRegister (
    _In_ INT ControlRegister
);

VOID
KdPrintError (
    _In_z_ LPCSTR ErrorMessage,
    _In_ ...
);

VOID
KdPrintTrace (
    _In_z_ LPCSTR TraceMessage,
    _In_ ...
);
