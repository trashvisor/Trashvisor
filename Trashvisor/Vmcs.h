#pragma once
#include "Extern.h"
#include "ArchUtils.h"
#include "VmxHost.h"

VOID
SetupVmcs (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
	_In_ ULONG_PTR GuestRip,
	_In_ ULONG_PTR GuestRsp
);

VOID
SetupVmcsGuestState (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
	_In_ ULONG_PTR GuestRip,
	_In_ ULONG_PTR GuestRsp
);

VOID
SetupVmcsHostState (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext
);

VOID
SetupVmcsExecutionCtls (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
	_In_ BOOLEAN UseTrue
);

VOID
SetupVmcsVmExitCtls (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
	_In_ BOOLEAN UseTrue
);

VOID
SetupVmcsVmEntryCtls (
	_In_ PLOCAL_VMM_CONTEXT pLocalVmmContext,
	_In_ BOOLEAN UseTrue
);
