#include "Header.h"
#include "VmxUtils.h"

NTSTATUS DriverEntry (
    _In_ PDRIVER_OBJECT pDriverObject,
    _In_ PUNICODE_STRING pRegistryPath
)
{
    UNREFERENCED_PARAMETER(pRegistryPath);

    if (!NT_SUCCESS(GetVmxCapability()))
        goto Exit;

    PGLOBAL_VMM_CONTEXT GlobalVmmContext = NULL;

    if (!NT_SUCCESS(AllocateVmmMemory(&GlobalVmmContext)))
        goto Exit;

    assert(GlobalVmmContext != NULL);

    KeGenericCallDpc(VmxBroadcastInit, GlobalVmmContext);
Exit:
    return STATUS_UNSUCCESSFUL;
}