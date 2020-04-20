#include "Header.h"

NTSTATUS DriverEntry (
    _In_ PDRIVER_OBJECT pDriverObject,
    _In_ PUNICODE_STRING pRegistryPath
)
{
    if (!NT_SUCCESS(VmxInitialise()))
        goto Exit;

Exit:
    return STATUS_UNSUCCESSFUL;
}