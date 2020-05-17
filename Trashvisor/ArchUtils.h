#pragma once
#include "Header.h"

typedef SEGMENT_DESCRIPTOR_REGISTER_64* PSEGMENT_DESCRIPTOR_REGISTER_64;

typedef struct _REGISTER_CONTEXT
{
    UINT64 P1Home;
    UINT64 P2Home;
    UINT64 P3Home;
    UINT64 P4Home;
    UINT64 P5Home;
    UINT64 P6Home;

    UINT32 ContextFlags;
    UINT32 MxCsr;

    SEGMENT_SELECTOR SegCs;
    SEGMENT_SELECTOR SegDs;
    SEGMENT_SELECTOR SegEs;
    SEGMENT_SELECTOR SegFs;
    SEGMENT_SELECTOR SegGs;
    SEGMENT_SELECTOR SegSs;
    UINT32 EFlags;

    UINT64 Dr0;
    UINT64 Dr1;
    UINT64 Dr2;
    UINT64 Dr3;
    UINT64 Dr6;
    UINT64 Dr7;

    UINT64 Rax;
    UINT64 Rcx;
    UINT64 Rdx;
    UINT64 Rbx;
    UINT64 Rsp;
    UINT64 Rbp;
    UINT64 Rsi;
    UINT64 Rdi;
    UINT64 R8;
    UINT64 R9;
    UINT64 R10;
    UINT64 R11;
    UINT64 R12;
    UINT64 R13;
    UINT64 R14;
    UINT64 R15;
    UINT64 Rip;

    XSAVE_FORMAT FltSave;

    M128A Header[2];
    M128A Legacy[8];

    M128A Xmm0;
    M128A Xmm1;
    M128A Xmm2;
    M128A Xmm3;
    M128A Xmm4;
    M128A Xmm5;
    M128A Xmm6;
    M128A Xmm7;
    M128A Xmm8;
    M128A Xmm9;
    M128A Xmm10;
    M128A Xmm11;
    M128A Xmm12;
    M128A Xmm13;
    M128A Xmm14;
    M128A Xmm15;

    M128A VectorRegister[26];
    UINT64 VectorControl;
    
    UINT64 DebugControl;
    UINT64 LastBranchToRip;
    UINT64 LastBranchFromRip;
    UINT64 LastExceptionToRip;
    UINT64 LastExceptionFromRip;
} REGISTER_CONTEXT, *PREGISTER_CONTEXT;

typedef struct _MISC_REGISTER_CONTEXT 
{
    CR0 Cr0;
    CR3 Cr3;
    CR4 Cr4;

    DR7 Dr7;

    SEGMENT_DESCRIPTOR_REGISTER_64 Gdtr;
    SEGMENT_DESCRIPTOR_REGISTER_64 Idtr;

    SEGMENT_SELECTOR Tr;
    SEGMENT_SELECTOR Ldtr;

    IA32_DEBUGCTL_REGISTER DebugCtlMsr;
    IA32_SYSENTER_CS_REGISTER SysEnterCsMsr;
    ULONG_PTR SysEnterEspMsr;
    ULONG_PTR SysEnterEipMsr;
    IA32_PERF_GLOBAL_CTRL_REGISTER PerfGlobalCtrlMsr;
    IA32_PAT_REGISTER PatMsr;
    IA32_EFER_REGISTER EferMsr;
    IA32_BNDCFGS_REGISTER BndCfgsMsr;
    IA32_RTIT_CTL_REGISTER RtitCtlMSr;
    // Looks like ia32.h doesn't define
    // IA32_S_CET
    // IA32_INTERRUPT_SSP_TABLE_ADDR
    // Doesn't matter. Processor doesn't support em
} MISC_REGISTER_CONTEXT, *PMISC_REGISTER_CONTEXT;

typedef union _VMCS_GUEST_ACCESS_RIGHTS
{
    struct
    {
        UINT64 SegmentType : 4;
        UINT64 DescriptorType : 1;
        UINT64 DPL : 2;
        UINT64 Present : 1;
        UINT64 Reserved : 4;
        UINT64 AVL : 1;
        UINT64 Long : 1;
        UINT64 DefaultOpSize : 1;
        UINT64 Granularity : 1;
        UINT64 SegmentUnusable : 1;
        UINT64 Reserved2 : 15;
    };

    UINT32 Flags;
} VMCS_GUEST_ACCESS_RIGHTS, *PVMCS_GUEST_ACCESS_RIGHTS;

VOID
CaptureMiscRegisterContext (
    _Out_ PMISC_REGISTER_CONTEXT pMiscRegisterContext 
);

VOID
CaptureRegisterContext (
    _Out_ PREGISTER_CONTEXT pRegisterContext
);

SEGMENT_DESCRIPTOR_64
GetDescriptorFromSelector (
    _In_ SEGMENT_SELECTOR Selector,
    _In_ SEGMENT_DESCRIPTOR_REGISTER_64 Gdtr
);

UINT32
GetBaseFromSegDescriptor (
    _In_ SEGMENT_DESCRIPTOR_64 Descriptor
);

UINT32
GetLimitFromSegDescriptor (
    _In_ SEGMENT_DESCRIPTOR_64 Descriptor
);

VMCS_GUEST_ACCESS_RIGHTS
GetAccessRightsFromSegDescriptor (
    _In_ SEGMENT_DESCRIPTOR_64 Descriptor
);

ULONGLONG
ReadMsr (
    _In_ ULONG Msr
);

VOID
WriteMsr (
    _In_ ULONG Msr,
    _In_ ULONG Value
);

VOID
Cpuid (
    _In_ INT32 CpuidIndex,
    _Out_ PINT32 pCpuidOut
);

ULONGLONG
ReadControlRegister (
    _In_ INT32 ControlRegister
);

VOID
WriteControlRegister (
    _In_ INT32 ControlRegister,
    _In_ ULONGLONG Value 
);

ULONGLONG
ReadDebugRegister (
    _In_ UINT32 DebugRegister
);

VOID
WriteDebugRegister (
    _In_ UINT32 DebugRegister,
    _In_ ULONGLONG Value
);

VOID
KdPrintError (
    _In_z_ LPCSTR ErrorMessage,
    _In_ ...
);

VOID
KdPrintTrace (
    _In_z_ LPCSTR TraceMessage,
    _In_ ...
);

EXTERN_C
_str(
    _In_ PVOID pTr
);

EXTERN_C
_sldt(
    _In_ PVOID pLdt
);
