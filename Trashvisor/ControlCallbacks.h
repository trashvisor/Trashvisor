#include "Extern.h"
#include "Shared.h"
#include "ArchUtils.h"

extern ULONG64 LogCpuidProcessCr3;
extern KGUARDED_MUTEX CallbacksMutex;
extern INFO_IOCTL_CPUID_PROCESS InfoIoctlCpuidProcess;

NTSTATUS
CtrlLogCpuidForProcess (
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
);