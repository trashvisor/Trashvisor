#include "ArchUtils.h"
#include "VmxUtils.h"
#include "VmxCore.h"

UNICODE_STRING DeviceName;
UNICODE_STRING SymbolicLinkName;

DRIVER_UNLOAD DriverUnload;

PGLOBAL_VMM_CONTEXT pGlobalVmmContext;

NTSTATUS
DriverEntry (
	PDRIVER_OBJECT pDriverObject,
	PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS Status = STATUS_SUCCESS;

	pDriverObject->DriverUnload = DriverUnload;

	if (!IsVmxSupported())
	{
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}
	
	pGlobalVmmContext = AllocateGlobalVmmContext();

	if (pGlobalVmmContext == NULL)
	{
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	KeGenericCallDpc(
		VmxBroadcastInit,
		pGlobalVmmContext
	);

	/*
	if (pGlobalVmmContext->ActivatedProcessorCount != 
		pGlobalVmmContext->LogicalProcessorCount)
	{
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}
	*/
	//KdPrintError("Successfully hypervised!\n");
/*
	RtlInitUnicodeString(&DeviceName, L"\\Device\\HVD");

	PDEVICE_OBJECT pHvdDevice;

	Status = IoCreateDevice(
		pDriverObject,
		0,
		&DeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&pHvdDevice
	);

	if (!NT_SUCCESS(Status))
	{
		KdPrintError("Failed to create device. Error code: %d\n",
			Status
		);

		goto Exit;
	}

	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\HVDDevice");

	Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

	if (!NT_SUCCESS(Status))
	{
		KdPrintError("Failed to create symbolic link. Error code: %d.\n",
			Status
		);
		
		goto Exit;
	}*/

Exit:
	return Status;
}

VOID 
DriverUnload (
	PDRIVER_OBJECT pDriverObject
)
{
	KeGenericCallDpc(
		VmxBroadcastTeardown,
		pGlobalVmmContext
	);
}
