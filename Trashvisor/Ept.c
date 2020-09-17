#include "Ept.h"
#include "VmxUtils.h"

BOOLEAN 
IsEptSupported (
)
{
    IA32_VMX_EPT_VPID_CAP_REGISTER EptVpidCapRegister;
    IA32_MTRR_DEF_TYPE_REGISTER MtrrDefTypeRegister;

    EptVpidCapRegister.Flags = ReadMsr(IA32_VMX_EPT_VPID_CAP);
    MtrrDefTypeRegister.Flags = ReadMsr(IA32_MTRR_DEF_TYPE);

    if (!EptVpidCapRegister.ExecuteOnlyPages
        || !EptVpidCapRegister.PageWalkLength4
        || !EptVpidCapRegister.MemoryTypeWriteBack)
    {
        KdPrintError("VmxEpt capabilities not supported.\n");
        return FALSE;
    }
    
    return TRUE;
}

_Use_decl_annotations_
VOID
SetupContextEptInformation (
)
{
    IA32_MTRR_CAPABILITIES_REGISTER MtrrCapRegister;
    IA32_MTRR_PHYSBASE_REGISTER MtrrPhysBase;
    IA32_MTRR_PHYSMASK_REGISTER MtrrPhysMask;

    MtrrCapRegister.Flags = ReadMsr(IA32_MTRR_CAPABILITIES);

    for (int i = 0; i < MtrrCapRegister.VariableRangeCount; i++)
    {
        MtrrPhysBase.Flags = ReadMsr(IA32_MTRR_PHYSBASE0 + i * 2);
        MtrrPhysMask.Flags = ReadMsr(IA32_MTRR_PHYSMASK0 + i * 2);

        if (!MtrrPhysMask.Valid)
            continue;

        UINT64 Mask = MtrrPhysMask.PageFrameNumber;

        UINT64 BeginAddress = MtrrPhysBase.PageFrameNumber << 12;
        UINT64 EndAddress = BeginAddress + (Mask << 12) - 1;
        
        pGlobalVmmContext->EptMtrrDescriptors[i].BeginAddress = BeginAddress;
        pGlobalVmmContext->EptMtrrDescriptors[i].EndAddress = EndAddress;
        pGlobalVmmContext->EptMtrrDescriptors[i].MemoryType = MtrrPhysBase.Type;
    }
}

PEPT_REGION
GenerateEptTables (
)
{
    PHYSICAL_ADDRESS PA;
    PA.QuadPart = MAXULONG64;

    //__debugbreak();

    PEPT_REGION pEptRegion = MmAllocateContiguousMemory(sizeof(EPT_REGION), PA);

    if (pEptRegion == NULL)
    {
        KdPrintError("GenerateEptTables: MmAllocateContiguousMemory failed.\n");
        goto Exit;
    }

    RtlZeroMemory(
        pEptRegion,
        sizeof(EPT_REGION)
    );

    InitializeListHead(&pEptRegion->SplitList);
    InitializeListHead(&pEptRegion->HookList);

    pEptRegion->Pml4[0].Flags = 0;
    pEptRegion->Pml4[0].PageFrameNumber =
        MmGetPhysicalAddress(&pEptRegion->Pdpt[0]).QuadPart / PAGE_SIZE;
    pEptRegion->Pml4[0].ReadAccess = 1;
    pEptRegion->Pml4[0].WriteAccess = 1;
    pEptRegion->Pml4[0].ExecuteAccess = 1;

    for (int i = 0; i < PDPT_ENTRY_COUNT; i++)
    {
        pEptRegion->Pdpt[i].Flags = 0;
        pEptRegion->Pdpt[i].PageFrameNumber =
            MmGetPhysicalAddress(&pEptRegion->Pdt[i][0]).QuadPart / PAGE_SIZE;
        pEptRegion->Pdpt[i].ReadAccess = 1;
        pEptRegion->Pdpt[i].WriteAccess = 1;
        pEptRegion->Pdpt[i].ExecuteAccess = 1;
    }

    IA32_MTRR_CAPABILITIES_REGISTER MtrrCapRegister;

    MtrrCapRegister.Flags = ReadMsr(IA32_MTRR_CAPABILITIES);

    for (int i = 0; i < PDPT_ENTRY_COUNT; i++)
    {
        for (int j = 0; j < PDT_ENTRY_COUNT; j++)
        {
            PEPDE pEpde = &pEptRegion->Pdt[i][j];
            pEpde->Flags = 0;
            pEpde->PageFrameNumber = i * PDT_ENTRY_COUNT + j;
            pEpde->ReadAccess = 1;
            pEpde->WriteAccess = 1;
            pEpde->ExecuteAccess = 1;
            pEpde->LargePage = 1;

            if (i == 0 && j == 0)
            {
                pEpde->MemoryType = MEMORY_TYPE_UNCACHEABLE;
                continue;
            }

            pEpde->MemoryType = MEMORY_TYPE_WRITE_BACK;

            for (int k = 0; k < MtrrCapRegister.VariableRangeCount; k++)
            {
                EPT_MTRR_DESCRIPTOR EptMtrrDescriptor = pGlobalVmmContext->EptMtrrDescriptors[k];

                if (pEpde->PageFrameNumber >= EptMtrrDescriptor.BeginAddress
                    && pEpde->PageFrameNumber + LARGE_PAGE_SIZE - 1 <= EptMtrrDescriptor.EndAddress)
                {
                    pEpde->MemoryType = EptMtrrDescriptor.MemoryType;
                    break;
                }
            }
        }
    }

Exit:
    return pEptRegion;
}

BOOLEAN
SetupEptp (
)
{
    //__debugbreak();
    BOOLEAN Status = TRUE;

    EPT_POINTER EptPointer;
    EptPointer.Flags = 0;

    PEPT_REGION pEptRegion = GenerateEptTables();

    if (pEptRegion == NULL)
    {
        KdPrintError("SetupEptp failed.\n");
        Status = FALSE;
        goto Exit;
    }

    EptPointer.PageFrameNumber = 
        MmGetPhysicalAddress(&pEptRegion->Pml4[0]).QuadPart / PAGE_SIZE;
    EptPointer.PageWalkLength = 3;
    EptPointer.MemoryType = MEMORY_TYPE_WRITE_BACK;

    pGlobalVmmContext->EptPointer.Flags = EptPointer.Flags;
    pGlobalVmmContext->pEptRegion = pEptRegion;
    /*
    pLocalVmmContext->EptPointer.Flags = EptPointer.Flags;
    pLocalVmmContext->pEptRegion = pEptRegion;
    */

Exit:
    return Status;
}

_Use_decl_annotations_
BOOLEAN
Split2MBPage (
    UINT64 VirtualAddress
)
{
    UINT32 PdptIndex = GET_PDPT_INDEX(VirtualAddress);
    UINT32 PdtIndex = GET_PDT_INDEX(VirtualAddress);
    BOOLEAN Status = TRUE;

    PEPT_REGION pEptRegion = pGlobalVmmContext->pEptRegion;

    PEPDE pEpde = &pEptRegion->Pdt[PdptIndex][PdtIndex];

    if (!pEpde->LargePage)
    {
        //__debugbreak();
        KdPrintError("Already split page.\n");
        goto Exit;
    }

    PHYSICAL_ADDRESS PhysAddress;
    PhysAddress.QuadPart = MAXULONG64;

    PEPT_SPLIT_LARGE_PAGE pSplitPage = 
        MmAllocateContiguousMemory(sizeof(EPT_SPLIT_LARGE_PAGE), PhysAddress);

    if (pSplitPage == NULL)
    {
        KdPrintError("Split2MBPage: Could not allocate memory.\n");
        Status = FALSE;
        goto Exit;
    }

    RtlZeroMemory(
        pSplitPage,
        sizeof(EPT_SPLIT_LARGE_PAGE)
    );

    pSplitPage->Epde = *pEpde;

    pEpde->LargePage = 0;

    UINT64 FrameOffset = pEpde->PageFrameNumber * (LARGE_PAGE_SIZE / PAGE_SIZE);

    for (int i = 0; i < PT_ENTRY_SPLIT_COUNT; i++)
    {
        pSplitPage->Pte[i].ReadAccess = 1;
        pSplitPage->Pte[i].WriteAccess = 1;
        pSplitPage->Pte[i].ExecuteAccess = 1;
        pSplitPage->Pte[i].PageFrameNumber = i + FrameOffset;
    }

    EPDE NewEpde;
    NewEpde.Flags = 0;

    NewEpde.ReadAccess = 1;
    NewEpde.WriteAccess = 1;
    NewEpde.ExecuteAccess = 1;
    NewEpde.PageFrameNumber = 
        MmGetPhysicalAddress(&pSplitPage->Pte[0]).QuadPart / PAGE_SIZE;

    RtlCopyMemory(
        pEpde,
        &NewEpde,
        sizeof(NewEpde)
    );

    InsertHeadList(
        &pGlobalVmmContext->pEptRegion->SplitList, 
        &pSplitPage->SplitListEntry
    );

Exit:
    return Status;
}

_Use_decl_annotations_
BOOLEAN
EptCreateHook (
    UINT64 VirtualAddress,
    PCHAR FakePage
)
{
    BOOLEAN Status = TRUE;

    //Page in virtual address
    __writecr4(__readcr4() & ~CR4_SMAP_ENABLE_FLAG);
    volatile INT32 PageReader;
    PageReader = *(PINT32)VirtualAddress;
    __writecr4(__readcr4() | CR4_SMAP_ENABLE_FLAG);

    UINT64 PhysicalAddress = MmGetPhysicalAddress((PVOID)VirtualAddress).QuadPart;

    KdPrintError("Installing hook for physical address => 0x%llx\n", PhysicalAddress);

    //__debugbreak();
    UINT32 PdptIndex = GET_PDPT_INDEX(PhysicalAddress);
    UINT32 PdtIndex = GET_PDT_INDEX(PhysicalAddress);
    UINT32 PtIndex = 0;

    EPT_INVEPT_DESCRIPTOR InvEptDescriptor;
    InvEptDescriptor.Reserved = 0;

    PEPT_HOOKED_ENTRY pEptHookedEntry = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(EPT_HOOKED_ENTRY),
        'ETPE'
    );

    if (pEptHookedEntry == NULL)
    {
        KdPrintError("EptCreateHook: ExAllocatePoolWithTag failed to allocate memory.\n");
        Status = FALSE;
        goto Exit;
    }

    PEPDE pEpde = &pGlobalVmmContext->pEptRegion->Pdt[PdptIndex][PdtIndex];
    PEPDE_SPLIT pEpdte = (PEPDE_SPLIT)pEpde;

    if (pEpde->LargePage)
    {
        if (!Split2MBPage(PhysicalAddress))
        {
            Status = FALSE;
            goto Exit;
        }
    }

    PtIndex = GET_PT_INDEX(PhysicalAddress);

    PHYSICAL_ADDRESS PhysAddr;
    PhysAddr.QuadPart = pEpdte->PageFrameNumber * PAGE_SIZE;

    PEPTE pPageTable = MmGetVirtualForPhysical(PhysAddr);
    PEPTE pOriginalEntry = &pPageTable[PtIndex];

    pEptHookedEntry->PhysBaseAddress = PhysicalAddress;

    pEptHookedEntry->pPte = pOriginalEntry;
    pEptHookedEntry->OriginalEntry = *pOriginalEntry;

    pEptHookedEntry->ExecuteOnlyEntry.Flags = pOriginalEntry->Flags;
    pEptHookedEntry->ExecuteOnlyEntry.ExecuteAccess = 1;
    pEptHookedEntry->ExecuteOnlyEntry.ReadAccess = 0;
    pEptHookedEntry->ExecuteOnlyEntry.WriteAccess = 0;

    pEptHookedEntry->NoExecuteEntry.Flags = pOriginalEntry->Flags;
    pEptHookedEntry->NoExecuteEntry.ExecuteAccess = 0;
    pEptHookedEntry->NoExecuteEntry.ReadAccess = 1;
    pEptHookedEntry->NoExecuteEntry.WriteAccess = 1;

    pEptHookedEntry->ExecuteOnlyEntry.PageFrameNumber = 
        MmGetPhysicalAddress(pEptHookedEntry->FakePage).QuadPart / PAGE_SIZE;
    
    RtlCopyMemory(
        pEptHookedEntry->FakePage,
        FakePage,
        PAGE_SIZE
    );

    InsertHeadList(
        &pGlobalVmmContext->pEptRegion->HookList,
        &pEptHookedEntry->EptHookedListEntry
    );

    *pOriginalEntry = pEptHookedEntry->NoExecuteEntry;
    InvEptDescriptor.EptPointer = pGlobalVmmContext->EptPointer.Flags;

    //_invept(InveptAllContext, &InvEptDescriptor);
    
Exit:
    return Status;
}
