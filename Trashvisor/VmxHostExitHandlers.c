#include "VmxHost.h"
#include "ControlCallbacks.h"

_Use_decl_annotations_
VOID
ExitHandleCpuid (
    PVMM_EXIT_CONTEXT pExitContext,
    PVMM_GUEST_CONTEXT pGPContext
)
{
    pExitContext->IncrementRip = TRUE;

    UINT64 StoredRax = pGPContext->Rax;

    int Cpuid[4];

    __cpuid(Cpuid, pGPContext->Rax);

    pGPContext->Rax = Cpuid[0];
    pGPContext->Rbx = Cpuid[1];
    pGPContext->Rcx = Cpuid[2];
    pGPContext->Rdx = Cpuid[3];

    if (pCpuidLoggingInfo == NULL)
        return;

    if (pExitContext->GuestCr3.Flags == pCpuidLoggingInfo->UserCr3.Flags)
    {
        __writecr3(pCpuidLoggingInfo->KernelCr3.Flags);

        KdPrintError(
            "Cpuid called for process with RAX: 0x%llx\n",
            StoredRax
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
            PLDR_MODULE pCurrentModule = CONTAINING_RECORD(CurrLink->Flink, LDR_MODULE, InMemOrderModuleList.Flink);

            PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(pCurrentModule->BaseAddress);

            UINT32 Size = NtHeader->OptionalHeader.SizeOfImage;

            UINT_PTR BeginAddress = pCurrentModule->BaseAddress;
            UINT_PTR EndAddress = BeginAddress + Size;

            if (pExitContext->GuestRip >= BeginAddress && pExitContext->GuestRip <= EndAddress)
            {
                DataLine.CpuidRax = StoredRax;
                DataLine.CpuidModuleRva = pExitContext->GuestRip - BeginAddress;

                RtlCopyMemory(
                    DataLine.ModuleName,
                    pCurrentModule->BaseDllName.Buffer,
                    pCurrentModule->BaseDllName.Length
                );

                KdPrintError("Found cpuid call with RVA: 0x%x in: %wZ\n",
                    pExitContext->GuestRip - BeginAddress,
                    pCurrentModule->BaseDllName);

                if (pCpuidLoggingInfo->CurrentDataLine < pCpuidLoggingInfo->MaxLogCount)
                    pCpuidLoggingInfo->LoggingData[pCpuidLoggingInfo->CurrentDataLine++] = DataLine;
                else
                    KdPrintError("Adjust the max logging count!.\n");

                break;
            }

            CurrLink = CurrLink->Flink;

        } while (CurrLink->Flink != FirstLink);

        Cr4.SmapEnable = 1;
        __writecr4(Cr4.Flags);

        // Could rewrite cr3 but no point
    }
}

VOID
ExitHandleRdMsr (
    _In_ PVMM_EXIT_CONTEXT pExitContext,
    _In_ PVMM_GUEST_CONTEXT pGPContext
)
{
    ULONG64 Value = __readmsr(pGPContext->Rcx);
    pGPContext->Rax = (Value) & 0xFFFFFFFF;
    pGPContext->Rdx = (Value >> 32) & 0xFFFFFFFF;
}

VOID
ExitHandleWrMsr (
    _In_ PVMM_EXIT_CONTEXT pExitContext,
    _In_ PVMM_GUEST_CONTEXT pGPContext
)
{
    ULONG64 Value = pGPContext->Rdx;
    Value <<= 32;
    Value |= pGPContext->Rax;
    __writemsr(pGPContext->Rcx, Value);
}
