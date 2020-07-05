#include "Extern.h"
#include "Shared.h"
#include "ArchUtils.h"

extern ULONG64 LogCpuidProcessCr3;
extern KGUARDED_MUTEX CallbacksMutex;

NTSTATUS
CtrlLogCpuidForProcess (
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
);