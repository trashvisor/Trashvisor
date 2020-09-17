#include "VmxHost.h"
#include "ControlCallbacks.h"

VOID
VmxSetupHostContext (
    _In_ PVMM_EXIT_CONTEXT pExitContext,
    _In_ PVMM_GUEST_CONTEXT pGPContext
)
{
    __vmx_vmread(VMCS_GUEST_RSP, &pGPContext->Rsp);
    __vmx_vmread(VMCS_GUEST_CR3, &pExitContext->GuestCr3.Flags);
    __vmx_vmread(VMCS_GUEST_RIP, &pExitContext->GuestRip);
    __vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &pExitContext->GuestPhysicalAddress);
    __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &pExitContext->InstructionLength);
    __vmx_vmread(VMCS_EXIT_REASON, &pExitContext->ExitReason.Flags);

    pExitContext->IncrementRip = FALSE;
}

VOID
VmxHandleExitReason(
    _In_ PVMM_EXIT_CONTEXT pExitContext,
    _In_ PVMM_GUEST_CONTEXT pGPContext
)
{
    switch (pExitContext->ExitReason.ExitReason)
    {
    case VMX_EXIT_REASON_EXECUTE_CPUID:
    {
        pExitContext->IncrementRip = TRUE;
        ExitHandleCpuid(pExitContext, pGPContext);
        break;
    }
    case VMX_EXIT_REASON_EXECUTE_RDMSR:
    {
        pExitContext->IncrementRip = TRUE;
        ExitHandleRdMsr(pExitContext, pGPContext);
        break;
    }
    case VMX_EXIT_REASON_EXECUTE_WRMSR:
    {
        pExitContext->IncrementRip = TRUE;
        ExitHandleWrMsr(pExitContext, pGPContext);
        break;
    }
    case VMX_EXIT_REASON_EXECUTE_INVEPT:
    {
        pExitContext->IncrementRip = TRUE;
        ExitHandleInvept(pExitContext, pGPContext);
        break;
    }
    case VMX_EXIT_REASON_EPT_VIOLATION:
    {
        //__debugbreak();
        ExitHandleEptViolation(pExitContext, pGPContext);
        //__debugbreak();
        break;
    }
    case VMX_EXIT_REASON_EXECUTE_VMCLEAR:
        // Implement shutting down later.
        // Remember... the fucking vmcs is cleared... theres no vmresuming...
    case VMX_EXIT_REASON_EXECUTE_VMXOFF:
    case VMX_EXIT_REASON_EXECUTE_XSETBV:
    case VMX_EXIT_REASON_EXECUTE_GETSEC:
    case VMX_EXIT_REASON_EXECUTE_INVD:
    default:
        __debugbreak();
    }
}

VOID
HostIncrementRip(
    PVMM_EXIT_CONTEXT pExitContext
)
{
    if (pExitContext->IncrementRip == FALSE)
        return;

    UINT64 NewRip = pExitContext->GuestRip + pExitContext->InstructionLength;

    __vmx_vmwrite(VMCS_GUEST_RIP, NewRip);
}

_Use_decl_annotations_
VOID
VmxVmExitHandler (
    ULONG64 HostStackAddress
)
{
    VMM_EXIT_CONTEXT ExitContext = { 0 };

    PVMM_GUEST_CONTEXT pGPContext = (PVMM_GUEST_CONTEXT)HostStackAddress;

    VmxSetupHostContext(&ExitContext, pGPContext);

    VmxHandleExitReason(&ExitContext, pGPContext);

    HostIncrementRip(&ExitContext);
}
