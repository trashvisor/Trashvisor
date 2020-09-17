#pragma once
#include "Ept.h"

typedef VMXON* PVMXON;
typedef struct _LOCAL_VMM_CONTEXT* PLOCAL_VMM_CONTEXT;

typedef struct _MISC_CONTEXT
{
    CR0 Cr0;
    CR3 Cr3;
    CR4 Cr4;
    DR7 Dr7;

    SEGMENT_DESCRIPTOR_REGISTER_64 Gdtr;
    SEGMENT_DESCRIPTOR_REGISTER_64 Idtr;

    SEGMENT_SELECTOR Tr;
    SEGMENT_SELECTOR Ldtr;
} MISC_CONTEXT, *PMISC_CONTEXT;

typedef struct _GLOBAL_VMM_CONTEXT
{
    PLOCAL_VMM_CONTEXT* ppLocalVmmContexts;
    ULONG LogicalProcessorCount;
    ULONG ActivatedProcessorCount;
    CR3 SystemCr3;
    PEPT_REGION pEptRegion;
    EPT_POINTER EptPointer;
    EPT_MTRR_DESCRIPTOR EptMtrrDescriptors[8];
} GLOBAL_VMM_CONTEXT, *PGLOBAL_VMM_CONTEXT;

typedef struct _VMM_GUEST_CONTEXT
{
    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;

    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;

    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
} VMM_GUEST_CONTEXT, *PVMM_GUEST_CONTEXT;

typedef struct _LOCAL_VMM_CONTEXT
{
    BOOLEAN VmxActivated;

    CONTEXT RegisterContext;
    MISC_CONTEXT MiscRegisterContext;

    //PEPT_REGION pEptRegion;
    //EPT_POINTER EptPointer;

    PVMXON pVmxOn;
    PVMCS pVmcs;
    PVOID PhysVmxOn;
    PVOID PhysVmcs;

    DECLSPEC_ALIGN(PAGE_SIZE) CHAR MsrBitmap[4096];
    PVOID PhysMsrBitmap;

    PGLOBAL_VMM_CONTEXT pGlobalVmmContext;

    DECLSPEC_ALIGN(PAGE_SIZE) CHAR HostStack[0x1000 * 0xA];
    UINT_PTR StackTop;
} LOCAL_VMM_CONTEXT, *PLOCAL_VMM_CONTEXT;

typedef union _VMCS_ACCESS_RIGHTS
{
    struct
    {
        UINT32 SegmentType : 4;
        UINT32 S : 1;
        UINT32 DPL : 2;
        UINT32 P : 1;
        UINT32 Reserved1 : 4;
        UINT32 AVL : 1;
        UINT32 L : 1;
        UINT32 DB : 1;
        UINT32 G : 1;
        UINT32 Unusable : 1;
    };

    struct
    {
        UINT16 FlagsLowerWord;
        UINT16 FlagsHigherWord;
    };

    UINT32 Flags;
} VMCS_ACCESS_RIGHTS, *PVMCS_ACCESS_RIGHTS;

_Must_inspect_result_
PGLOBAL_VMM_CONTEXT
AllocateGlobalVmmContext (
);

_Must_inspect_result_
PLOCAL_VMM_CONTEXT
AllocateLocalVmmContext (
    _In_ PGLOBAL_VMM_CONTEXT pGlobalVmmContext
);

_Must_inspect_result_
PVMCS
AllocateVmcs (
);

_Must_inspect_result_
PVMXON
AllocateVmxOn (
);

_Must_inspect_result_
PLOCAL_VMM_CONTEXT
RetrieveLocalContext (
    _In_ ULONG ProcessorNumber,
    _In_ PGLOBAL_VMM_CONTEXT pGlobalVmmContext
);

VOID
CaptureState (
    _In_ PLOCAL_VMM_CONTEXT pLocalVmmContext
);

VOID
CaptureMiscContext (
    _In_ PMISC_CONTEXT pMiscContext
);

BOOLEAN
EnableVmx (
    _In_ PLOCAL_VMM_CONTEXT pLocalVmmContext
);

VOID
GetSegmentSetup (
    _Out_ PSEGMENT_SETUP pSegmentSetup,
    _In_ UINT16 Selector,
    _In_ UINT64 GdtrBase
);

VOID
GetSysSegmentSetup (
    _Out_ PSEGMENT_SETUP pSegmentSetup,
    _In_ UINT16 Selector,
    _In_ UINT64 GdtrBase
);

UINT32
GetAccessRights (
    _In_ SEGMENT_DESCRIPTOR_32 SegmentDesc
);

UINT32
GetSysAccessRights(
    _In_ SEGMENT_DESCRIPTOR_64 SegmentDesc
);

ULONG32
EnforceRequiredBits(
    _In_ ULONG64 RequiredBits,
    _In_ ULONG32 BitsToSet
);
