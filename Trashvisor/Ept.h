#pragma once
#include "ArchUtils.h"

#define PML4_ENTRY_COUNT (1 << 9)
#define PDPT_ENTRY_COUNT (1 << 9)
#define PDT_ENTRY_COUNT  (1 << 9)
#define PT_ENTRY_SPLIT_COUNT (1 << 9)
#define PT_ENTRY_COUNT   (1 << 12)

#define LARGE_PAGE_SIZE 0x200000

typedef struct _GLOBAL_VMM_CONTEXT  *PGLOBAL_VMM_CONTEXT;
typedef struct _LOCAL_VMM_CONTEXT   *PLOCAL_VMM_CONTEXT;

typedef EPDE_2MB *PEPDE;
typedef EPDE *PEPDE_SPLIT;
typedef EPTE *PEPTE;

extern PGLOBAL_VMM_CONTEXT pGlobalVmmContext;

#define GET_PML4_INDEX(Address)  (Address >> 39) & 0x1ff
#define GET_PDPT_INDEX(Address)  (Address >> 30) & 0x1ff
#define GET_PDT_INDEX(Address)   (Address >> 21) & 0x1ff
#define GET_PT_INDEX(Address)    (Address >> 12) & 0x1ff
#define GET_PTE_INDEX(Address)   (Address)       & 0x1ff

typedef struct _EPT_REGION
{
    DECLSPEC_ALIGN(PAGE_SIZE) EPT_PML4 Pml4[PML4_ENTRY_COUNT];
    DECLSPEC_ALIGN(PAGE_SIZE) EPDPTE   Pdpt[PDPT_ENTRY_COUNT];
    DECLSPEC_ALIGN(PAGE_SIZE) EPDE_2MB Pdt[PDPT_ENTRY_COUNT][PDT_ENTRY_COUNT];

    LIST_ENTRY SplitList;
    LIST_ENTRY HookList;
} EPT_REGION, *PEPT_REGION; 

typedef struct _EPT_SPLIT_LARGE_PAGE
{
    DECLSPEC_ALIGN(PAGE_SIZE) EPTE Pte[PT_ENTRY_SPLIT_COUNT];
    EPDE_2MB Epde;
    LIST_ENTRY SplitListEntry;
} EPT_SPLIT_LARGE_PAGE, *PEPT_SPLIT_LARGE_PAGE;

typedef struct _EPT_HOOKED_ENTRY
{
    DECLSPEC_ALIGN(PAGE_SIZE) CHAR FakePage[PAGE_SIZE];
    LIST_ENTRY EptHookedListEntry;
    UINT64 PhysBaseAddress;

    PEPTE pPte;
    EPTE  OriginalEntry;
    EPTE  ExecuteOnlyEntry;
    EPTE  NoExecuteEntry;
} EPT_HOOKED_ENTRY, *PEPT_HOOKED_ENTRY;

typedef struct _EPT_MTRR_DESCRIPTOR
{
    UINT64 BeginAddress;
    UINT64 EndAddress;
    UINT8  MemoryType;
} EPT_MTRR_DESCRIPTOR, *PEPT_MTRR_DESCRIPTOR;

typedef struct _EPT_INVEPT_DESCRIPTOR
{
    UINT64 EptPointer;
    UINT64 Reserved;
} EPT_INVEPT_DESCRIPTOR, *PEPT_INVEPT_DESCRIPTOR;

BOOLEAN
IsEptSupported (
);

VOID
SetupContextEptInformation (
);

PEPT_REGION
GenerateEptTables (
);

BOOLEAN
SetupEptp (
);

BOOLEAN
EptCreateHook (
    _In_ UINT64 VirtualAddress,
    _In_ PCHAR FakePage
);

VOID
_invept (
    UINT64 InvEptType,
    PEPT_INVEPT_DESCRIPTOR pInvEptDescriptor
);
