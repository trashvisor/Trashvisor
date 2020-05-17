#include "Header.h"
#include "VmxUtils.h"

NTSTATUS DriverEntry (
    _In_ PDRIVER_OBJECT pDriverObject,
    _In_ PUNICODE_STRING pRegistryPath
)
{
    UNREFERENCED_PARAMETER(pRegistryPath);

    NTSTATUS Status = STATUS_SUCCESS;

    if (!NT_SUCCESS(GetVmxCapability()))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    PGLOBAL_VMM_CONTEXT GlobalVmmContext = NULL;

    if (!NT_SUCCESS(AllocateVmmMemory(&GlobalVmmContext)))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    ASSERT(GlobalVmmContext != NULL);

    KeGenericCallDpc(VmxBroadcastInit, GlobalVmmContext);

Exit:
    return Status;
}