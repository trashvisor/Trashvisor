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
		if (pCpuidLoggingInfo == NULL)
			goto IncrementRip;

		CR3 GuestCr3;
		__vmx_vmread(VMCS_GUEST_CR3, &GuestCr3.Flags);

		ULONG64 Rip;
		__vmx_vmread(VMCS_GUEST_RIP, &Rip);

		if (GuestCr3.Flags == pCpuidLoggingInfo->UserCr3.Flags)
		{
			//__debugbreak();

			__writecr3(pCpuidLoggingInfo->KernelCr3.Flags);
	
			KdPrintError(
				"Cpuid called for process with RAX: 0x%llx\n",
				pGPContext->Rax
			);
			
			PCHAR pPeb = (PCHAR)pCpuidLoggingInfo->pPeb;

			// Disable SMAP
			CR4 Cr4;
			Cr4.Flags = __readcr4();
			Cr4.SmapEnable = 0;
			__writecr4(Cr4.Flags);

			PPEB_LDR_DATA pLdrData = *(PUINT64)(pPeb + 0x18);

			PLIST_ENTRY FirstLink = &(pLdrData->InMemOrderModuleList);
			PLIST_ENTRY CurrLink = FirstLink;

			CPUID_DATA_LINE DataLine;

			do 
			{
				__debugbreak();
                PLDR_MODULE pCurrentModule = CONTAINING_RECORD(CurrLink->Flink, LDR_MODULE, InMemOrderModuleList.Flink);

				PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(pCurrentModule->BaseAddress);

				UINT32 Size = NtHeader->OptionalHeader.SizeOfImage;

				UINT_PTR BeginAddress = pCurrentModule->BaseAddress;
				UINT_PTR EndAddress = BeginAddress + Size;

				if (Rip >= BeginAddress && Rip <= EndAddress)
				{
					DataLine.CpuidRax = pGPContext->Rax;
					DataLine.CpuidModuleRva = Rip - BeginAddress;

					RtlCopyMemory(
						DataLine.ModuleName,
						pCurrentModule->BaseDllName.Buffer,
						pCurrentModule->BaseDllName.Length
					);
					
					KdPrintError("Found cpuid call with RVA: 0x%x in: %wZ\n",
						Rip - BeginAddress,
						pCurrentModule->BaseDllName);

					pCpuidLoggingInfo->LoggingData[pCpuidLoggingInfo->CurrentDataLine++] = DataLine;

					break;
				}

				CurrLink = CurrLink->Flink;

			} while (CurrLink->Flink != FirstLink);

            Cr4.SmapEnable = 1;
            __writecr4(Cr4.Flags);
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
