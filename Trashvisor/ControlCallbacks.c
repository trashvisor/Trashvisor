#include "ControlCallbacks.h"

#define USER_DIRECTORY_TABLE_BASE_OFFSET 0x280
#define PPEB_OFFSET 0x3f8

KGUARDED_MUTEX CallbacksMutex = { 0 };
CPUID_LOGGING_INFO CpuidLoggingInfo = { 0 };
WCHAR ProcessName[MAX_PATH_LENGTH];

VOID
CpuidLoggingProcessCallback (
    PEPROCESS Eproc,
    HANDLE ProcessId,
    PPS_CREATE_NOTIFY_INFO pCreateInfo
)
{
    if (pCreateInfo == NULL)
        return;

    PCHAR pKproc = (PCHAR)Eproc;
    PCHAR pEproc = (PCHAR)Eproc;

    PWCHAR Found = wcsstr(
        pCreateInfo->CommandLine->Buffer,
        ProcessName
    );

    if (Found != NULL)
    {
        // Get UserDirectoryTableBase, introduced post-meltdown
        // DirectoryTableBase is the Kernel Cr3
        CR3 CR3ToTrack;
        CR3ToTrack.Flags = *(PULONG64)(pKproc + USER_DIRECTORY_TABLE_BASE_OFFSET);

        CpuidLoggingInfo.ProcessId = ProcessId;
        CpuidLoggingInfo.Cr3 = CR3ToTrack;
        CpuidLoggingInfo.pPeb = *(PUINT64)(pEproc + PPEB_OFFSET);

        KdPrintError("TRACKING CR3: 0x%llx\n", CR3ToTrack.Flags);
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
    
    if (wcslen(pIoctlInfo->FilePath) > MAX_PATH_LENGTH ||
        sizeof(pIoctlInfo->FilePath) != sizeof(ProcessName))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    
    KdPrintError(
        "Received: Process name => %S\nFile path => %S\n",
        pIoctlInfo->ProcessName,
        pIoctlInfo->FilePath
    );

    memcpy(ProcessName, pIoctlInfo->ProcessName, sizeof(ProcessName));

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

    CpuidLoggingInfo.FileHandle = FileHandle;

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