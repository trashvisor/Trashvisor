#include "VmxHost.h"
#include "ControlCallbacks.h"

_Use_decl_annotations_
VOID
HandleCpuid(
    PVMM_EXIT_CONTEXT pExitContext,
    PGUEST_REGISTER_CONTEXT pGPContext
)
{
    INT32 Cpuid[4];
    CR4 Cr4;

    if (pCpuidLoggingInfo == NULL)
        return;

    if (pExitContext->GuestCr3.Flags == pCpuidLoggingInfo->UserCr3.Flags)
    {
        // Adjust CR3 to the guest's CR3.
        // Required for usermode memory inspection

        __writecr3(pCpuidLoggingInfo->KernelCr3.Flags);

        PCHAR pPeb = (PCHAR)pCpuidLoggingInfo->pPeb;

        // Disable SMAP
        // Required to iterate the usermode PEB
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
                DataLine.CpuidRax = pGPContext->Rax;
                DataLine.CpuidModuleRva = pExitContext->GuestRip - BeginAddress;

                RtlCopyMemory(
                    DataLine.ModuleName,
                    pCurrentModule->BaseDllName.Buffer,
                    pCurrentModule->BaseDllName.Length
                );

                KeAcquireGuardedMutex(&CallbacksMutex);

                if (pCpuidLoggingInfo->CurrentDataLine < pCpuidLoggingInfo->DataLineLimit)
                    ;//pCpuidLoggingInfo->LoggingData[pCpuidLoggingInfo->CurrentDataLine++] = DataLine;
                else
                    KdPrintError("Readjust IOCTL data limit.\n");

                KeReleaseGuardedMutex(&CallbacksMutex);

                break;
            }

            CurrLink = CurrLink->Flink;

        } while (CurrLink->Flink != FirstLink);

        // Reset CR4 to its original value
        Cr4.SmapEnable = 1;
        __writecr4(Cr4.Flags);

        // Reset CR3 to its original value
        __writecr3(pExitContext->GuestCr3.Flags);
    }
//CpuidHandler:
/*
__cpuid(Cpuid, pGPContext->Rax);

pGPContext->Rax = Cpuid[0];
pGPContext->Rbx = Cpuid[1];
pGPContext->Rcx = Cpuid[2];
pGPContext->Rdx = Cpuid[3];
*/
}

_Use_decl_annotations_
VOID
HandleRdMsr(
    PVMM_EXIT_CONTEXT pExitContext,
    PGUEST_REGISTER_CONTEXT pGPContext
)
{
    ULONG64 Value = __readmsr(pGPContext->Rcx);
    pGPContext->Rax = (Value) & 0xFFFFFFFF;
    pGPContext->Rdx = (Value >> 32) & 0xFFFFFFFF;
}

_Use_decl_annotations_
VOID
HandleWrMsr(
    PVMM_EXIT_CONTEXT pExitContext,
    PGUEST_REGISTER_CONTEXT pGPContext
)
{
    ULONG64 Value = pGPContext->Rdx;
    Value <<= 32;
    Value |= pGPContext->Rax;
    __writemsr(pGPContext->Rcx, Value);
}
