#include "ControlCallbacks.h"

UINT64 LogCpuidProcessCr3 = 0;
INFO_IOCTL_CPUID_PROCESS InfoIoctlCpuidProcess;
KGUARDED_MUTEX CallbacksMutex;

VOID
CpuidLoggingProcessCallback (
    PEPROCESS pEproc,
    HANDLE ProcessId,
    PPS_CREATE_NOTIFY_INFO pCreateInfo
)
{
    if (pCreateInfo == NULL)
        return;

    PCHAR pKproc = (PCHAR)pEproc;

    /*
    KdPrintError(
        "Creating process: %wZ\n",
        pCreateInfo->CommandLine
    );
    */

    PWCHAR Found = wcsstr(
        pCreateInfo->CommandLine->Buffer,
        InfoIoctlCpuidProcess.ProcessName
    );

    if (Found != NULL)
    {
        //__debugbreak();
        // Assume that only one process will trigger this 
        LogCpuidProcessCr3 = *(PULONG64)(pKproc + 0x28) & ~0xfffULL;

        KdPrintError("TRACKING CR3: 0x%llx\n", LogCpuidProcessCr3);
    }
}

NTSTATUS
CtrlLogCpuidForProcess (
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    PINFO_IOCTL_CPUID_PROCESS pIoctlInfo = 
        (PINFO_IOCTL_CPUID_PROCESS)pIrp->AssociatedIrp.SystemBuffer;

    UINT32 InputLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

    // Verify length
    if (sizeof(INFO_IOCTL_CPUID_PROCESS) > InputLength)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    // Verify parameters
    if (wcslen(pIoctlInfo->ProcessName) > MAX_PATH_LENGTH)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    
    if (wcslen(pIoctlInfo->FilePath) > MAX_PATH_LENGTH)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    
    KdPrintError(
        "Received: Process name => %S\nFile path => %S\n",
        pIoctlInfo->ProcessName,
        pIoctlInfo->FilePath
    );

    // Next stage of validation/creation
    // Attempt to create a file with the name
    HANDLE FileHandle;

    OBJECT_ATTRIBUTES Oa;
    UNICODE_STRING FilePath;

    RtlInitUnicodeString(&FilePath, pIoctlInfo->FilePath);

    InitializeObjectAttributes(
        &Oa,
        &FilePath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    IO_STATUS_BLOCK IoStatusBlock;

    Status = ZwCreateFile(
        &FileHandle,
        FILE_WRITE_DATA | FILE_READ_ACCESS,
        &Oa,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT,
        NULL,
        0
    );

    if (!NT_SUCCESS(Status))
    {
        KdPrintError(
            "CtrlLogCpuidProcess: ZwCreateFile failed with: 0x%x\n",
            Status
        );

        goto Exit;
    }

    KeAcquireGuardedMutex(&CallbacksMutex);

    RtlCopyMemory(
        &InfoIoctlCpuidProcess,
        pIoctlInfo,
        sizeof(INFO_IOCTL_CPUID_PROCESS)
    );


    KeReleaseGuardedMutex(&CallbacksMutex);

    Status = PsSetCreateProcessNotifyRoutineEx2(
        PsCreateProcessNotifySubsystems,
        CpuidLoggingProcessCallback,
        FALSE
    );

    if (!NT_SUCCESS(Status))
    {
        __debugbreak();
        KdPrintError(
            "CtrlLogCpuidProcess: PsSetCreateProcessNotifyRoutineEx2 failed with: 0x%x\n",
            Status
        );
    }

Exit:
    return Status;
}