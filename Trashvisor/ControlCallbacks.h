#include "Extern.h"
#include "Shared.h"
#include "ArchUtils.h"

typedef struct _CPUID_LOGGING_INFO
{
    HANDLE FileHandle;
    HANDLE ProcessId;
    CR3 Cr3;
    PPEB pPeb;
} CPUID_LOGGING_INFO, *PCPUID_LOGGING_INFO;

extern KGUARDED_MUTEX CallbacksMutex;
extern CPUID_LOGGING_INFO CpuidLoggingInfo;

NTSTATUS
CtrlLogCpuidForProcess (
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
);