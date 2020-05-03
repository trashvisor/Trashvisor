#pragma once
#include <ntddk.h>
#include <intrin.h>
#include <assert.h>
#include <stdarg.h>
#include "ia32.h"

NTKERNELAPI
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
KeGenericCallDpc(
	_In_ PKDEFERRED_ROUTINE DeferredRoutine,
	_In_opt_ PVOID Context
);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
KeSignalCallDpcDone(
	_In_ PVOID SystemArgument1
);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
KeSignalCallDpcSynchronize(
	_In_ PVOID SystemArgument2
);
