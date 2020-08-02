#include "Extern.h"
#include "Shared.h"
#include "ArchUtils.h"

#define MAX_MODULE_LENGTH 30

typedef struct _CPUID_DATA_LINE
{
    UINT32 CpuidRax;
    UINT64 CpuidModuleRva;
    WCHAR ModuleName[MAX_MODULE_LENGTH];
} CPUID_DATA_LINE, *PCPUID_DATA_LINE;

typedef struct _CPUID_LOGGING_INFO
{
    HANDLE FileHandle;
    HANDLE ProcessId;
    CR3 UserCr3;
    CR3 KernelCr3;
    PPEB pPeb;
    UINT32 CurrentDataLine;
    CPUID_DATA_LINE LoggingData[0];
} CPUID_LOGGING_INFO, *PCPUID_LOGGING_INFO;

extern KGUARDED_MUTEX CallbacksMutex;
extern PCPUID_LOGGING_INFO pCpuidLoggingInfo;

NTSTATUS
CtrlLogCpuidForProcess (
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
);