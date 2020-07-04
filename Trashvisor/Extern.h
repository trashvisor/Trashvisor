#pragma once
#include <ntddk.h>
#include <intrin.h>
#include <stdarg.h>

#include <ia32.h>

NTSYSAPI
NTSTATUS
KeGenericCallDpc (
	_In_ PKDEFERRED_ROUTINE Routine,
	_In_ PVOID  pContext
);

NTSYSAPI
VOID
KeSignalCallDpcDone (
	_In_ PVOID SystemArgument1
);

NTSYSAPI
LOGICAL
KeSignalCallDpcSynchronize (
	_In_ PVOID SystemArgument2
);

NTSYSAPI
VOID
RtlCaptureContext(
	_Out_ PCONTEXT ContextRecord
);
