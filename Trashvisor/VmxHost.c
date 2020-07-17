#include "VmxHost.h"
#include "ControlCallbacks.h"

_Use_decl_annotations_
VOID
VmxVmExitHandler (
	ULONG64 HostStackAddress
)
{
	PVMM_REGISTER_CONTEXT pGPContext = (PVMM_REGISTER_CONTEXT)HostStackAddress;
	__vmx_vmread(VMCS_GUEST_RSP, &pGPContext->Rsp);

	VMX_EXIT_REASON ExitReason;
	__vmx_vmread(VMCS_EXIT_REASON, &ExitReason.Flags);

	switch (ExitReason.ExitReason)
	{
	case VMX_EXIT_REASON_EXECUTE_CPUID:
	{

		CR3 GuestCr3;
		__vmx_vmread(VMCS_GUEST_CR3, &GuestCr3.Flags);

		ULONG64 Rip;
		__vmx_vmread(VMCS_GUEST_RIP, &Rip);

		if (GuestCr3.Flags == CpuidLoggingInfo.Cr3.Flags)
		{
			KdPrintError(
				"Cpuid called for process with RAX: 0x%llx\n",
				pGPContext->Rax
			);

			pGPContext->Rax = 0;
			pGPContext->Rbx = 0; 
			pGPContext->Rcx = 0; 
			pGPContext->Rdx = 0; 
		}
		else
		{
            int Cpuid[4];
            __cpuid(Cpuid, pGPContext->Rax);
            pGPContext->Rax = Cpuid[0];
            pGPContext->Rbx = Cpuid[1];
            pGPContext->Rcx = Cpuid[2];
            pGPContext->Rdx = Cpuid[3];
		}

		goto IncrementRip;
	}
	case VMX_EXIT_REASON_EXECUTE_RDMSR:
	{
		//GdbBreakPoint();
		ULONG64 Value = __readmsr(pGPContext->Rcx);
		pGPContext->Rax = (Value) & 0xFFFFFFFF;
		pGPContext->Rdx = (Value >> 32) & 0xFFFFFFFF;

		goto IncrementRip;
	}
	case VMX_EXIT_REASON_EXECUTE_WRMSR:
	{
		ULONG64 Value = pGPContext->Rdx;
		Value <<= 32;
		Value |= pGPContext->Rax;
		__writemsr(pGPContext->Rcx, Value);

		goto IncrementRip;
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
		break;

	}

IncrementRip:
	{
		ULONG64 GuestRip;
		__vmx_vmread(VMCS_GUEST_RIP, &GuestRip);

		ULONG32 Length;
		__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &Length);
		GuestRip += Length;

		__vmx_vmwrite(VMCS_GUEST_RIP, GuestRip);
		return;
	}
}
