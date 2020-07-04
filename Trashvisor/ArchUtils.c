#include "ArchUtils.h"

_Use_decl_annotations_
BOOLEAN
IsVmxSupported(
)
{
	BOOLEAN bStatus = TRUE;

	int CpuIdInfo[4];
	__cpuid(CpuIdInfo, 1);

	if (!(CpuIdInfo[3] & (1 << 5)))
	{
		KdPrintError("Cpuid: Vmx not supported.\n");
		bStatus = FALSE;
		goto Exit;
	}

	IA32_FEATURE_CONTROL_REGISTER FeatureControlMsr;
	FeatureControlMsr.Flags = ReadMsr(IA32_FEATURE_CONTROL);

	if (!FeatureControlMsr.EnableVmxOutsideSmx)
	{
		KdPrintError("Vmx not enabled outside of SMX.\n");
		bStatus = FALSE;
		goto Exit;
	}

	if (!FeatureControlMsr.LockBit)
	{
		KdPrintError("Lock bit not set.\n");
		bStatus = FALSE;
		goto Exit;
	}

Exit:
	return bStatus;
}

_Use_decl_annotations_
VOID
KdPrintError (
    LPCSTR ErrorMessage,
    ...
)
{
    va_list ArgumentList;
    va_start(ArgumentList, ErrorMessage);

    vDbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, ErrorMessage, ArgumentList);

    va_end(ArgumentList);
}

_Use_decl_annotations_
ULONGLONG
ReadMsr(
	ULONG Msr	
)
{
	return __readmsr(Msr);
}

_Use_decl_annotations_
SEGMENT_DESCRIPTOR_32
GetSegmentDescriptor (
	_In_ UINT16 Selector,
	_In_ UINT64 GdtrBase
)
{
	SEGMENT_DESCRIPTOR_32 Descriptor = *(PSEGMENT_DESCRIPTOR_32)(GdtrBase + (Selector & ~0b111));
	return Descriptor;
}

_Use_decl_annotations_
SEGMENT_DESCRIPTOR_64
GetSysSegmentDescriptor (
	_In_ UINT16 Selector,
	_In_ UINT64 GdtrBase
)
{
	SEGMENT_DESCRIPTOR_64 Descriptor = *(PSEGMENT_DESCRIPTOR_64)(GdtrBase + (Selector & ~0b111));
	return Descriptor;
}

_Use_decl_annotations_
UINT32
GetSegmentBase (
	_In_ SEGMENT_DESCRIPTOR_32 Segment
)
{
	return Segment.BaseAddressHigh << 24
		| Segment.BaseAddressMiddle << 16
		| Segment.BaseAddressLow;
}

_Use_decl_annotations_
ULONG64
GetSysSegmentBase (
	_In_ SEGMENT_DESCRIPTOR_64 SysSegment
)
{
	ULONG64 BaseAddress = SysSegment.BaseAddressUpper;
	BaseAddress <<= 32;
	
	BaseAddress |= (SysSegment.BaseAddressHigh << 24);
	BaseAddress |= (SysSegment.BaseAddressMiddle << 16);
	BaseAddress |= SysSegment.BaseAddressLow;

	return BaseAddress;
}
