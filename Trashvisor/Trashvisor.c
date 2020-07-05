#include "ArchUtils.h"
#include "VmxCore.h"
#include "ControlCallbacks.h"

_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH DriverDeviceCreate;
_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH DriverDeviceClose;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH DriverDeviceCtrl;

DRIVER_UNLOAD DriverUnload;

PGLOBAL_VMM_CONTEXT pGlobalVmmContext;

NTSTATUS
DriverEntry (
	_In_ PDRIVER_OBJECT pDriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS Status = STATUS_SUCCESS;

	UNICODE_STRING NtDeviceName;
	UNICODE_STRING DosDeviceLinkName;

	KeInitializeGuardedMutex(&CallbacksMutex);

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DriverDeviceCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDeviceClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceCtrl;
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

	if (pGlobalVmmContext->ActivatedProcessorCount != 
		pGlobalVmmContext->LogicalProcessorCount)
	{
		Status = STATUS_UNSUCCESSFUL;
		goto Exit;
	}

	KdPrintError("Successfully hypervised!\n");

	RtlInitUnicodeString(&NtDeviceName, NT_DEVICE_NAME);

	PDEVICE_OBJECT pTvDevice;

	Status = IoCreateDevice(
		pDriverObject,
		0,
		&NtDeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&pTvDevice
	);

	if (!NT_SUCCESS(Status))
	{
		KdPrintError("Failed to create device. Error code: %d\n",
			Status
		);

		goto Exit;
	}

	RtlInitUnicodeString(&DosDeviceLinkName, DOS_DEVICE_NAME);

	Status = IoCreateSymbolicLink(&DosDeviceLinkName, &NtDeviceName);

	if (!NT_SUCCESS(Status))
	{
		KdPrintError("Failed to create symbolic link. Error code: %d.\n",
			Status
		);
		
		goto Exit;
	}

Exit:
	return Status;
}

NTSTATUS
DriverDeviceCreate (
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);
	
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS
DriverDeviceClose (
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS
DriverDeviceCtrl (
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
)
{
	PIO_STACK_LOCATION pIrpStack;
	ULONG Ioctl;
	NTSTATUS Status = STATUS_SUCCESS;

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	Ioctl = pIrpStack->Parameters.DeviceIoControl.IoControlCode;

	__debugbreak();

	switch (Ioctl)
	{
	case IOCTL_LOG_CPUID_PROCESS:
		Status = CtrlLogCpuidForProcess(pDeviceObject, pIrp);
		break;
	default:
		KdPrintError("DriverDeviceCtrl: Default case.\n");
		Status = STATUS_UNSUCCESSFUL;
		break;
	}

	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return Status;
}

VOID 
DriverUnload (
	_In_ PDRIVER_OBJECT pDriverObject
)
{
	KeGenericCallDpc(
		VmxBroadcastTeardown,
		pGlobalVmmContext
	);
}
