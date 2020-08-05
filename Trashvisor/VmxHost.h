#pragma once
#include "VmxCore.h"
#include "ArchUtils.h"
#include "VmxUtils.h"

typedef union _VMX_EXIT_REASON
{
    struct
    {
        ULONG32 ExitReason : 16;
        ULONG32 Always0 : 1;
        ULONG32 Reserved1 : 10;
        ULONG32 Enclave : 1;
        ULONG32 PendingMTF : 1;
        ULONG32 ExitFromRoot : 1;
        ULONG32 Reserved2 : 1;
        ULONG32 VmEntryFailure : 1;
    };

    UINT32 Flags;
} VMX_EXIT_REASON;

typedef struct _VMM_EXIT_CONTEXT
{
    VMX_EXIT_REASON ExitReason;
    CR3				GuestCr3;
    UINT64			GuestRip;
    UINT32			InstructionLength;
    BOOLEAN			IncrementRip;
} VMM_EXIT_CONTEXT, *PVMM_EXIT_CONTEXT;

VOID
VmxVmExitHandlerStub (
);

VOID
VmxVmExitHandler (
    _In_ ULONG64 HostStackAddress
);

VOID
ExitHandleCpuid (
    _In_ PVMM_EXIT_CONTEXT pExitContext,
    _In_ PVMM_GUEST_CONTEXT pGPContext
);

VOID
ExitHandleRdMsr (
    _In_ PVMM_EXIT_CONTEXT pExitContext,
    _In_ PVMM_GUEST_CONTEXT pGPContext
);

VOID
ExitHandleWrMsr (
    _In_ PVMM_EXIT_CONTEXT pExitContext,
    _In_ PVMM_GUEST_CONTEXT pGPContext
);
