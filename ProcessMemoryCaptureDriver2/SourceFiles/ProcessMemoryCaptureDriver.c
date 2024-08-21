#include "ProcessMemoryCaptureDriver.h"
#include "UsbDriver.h"
#include <wdf.h>
#include <wdfusb.h>  

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    // Initialize driver and set up function pointers
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDeviceControl;

    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\ProcessMemoryCaptureDriver");
    UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\??\\ProcessMemoryCapture");

    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &deviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateSymbolicLink(&symbolicLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        return status;
    }

    // Initialize USB device
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    deviceExtension->DeviceObject = deviceObject;

    // Initialize USB
    status = UsbDeviceInitialize(deviceObject, deviceExtension);
    if (!NT_SUCCESS(status)) {
        IoDeleteSymbolicLink(&symbolicLink);
        IoDeleteDevice(deviceObject);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS DriverDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioctlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_SUCCESS;

    switch (ioctlCode)
    {
    case IOCTL_CAPTURE_MEMORY:
        status = CaptureProcessMemory(DeviceObject, Irp);
        break;

    case IOCTL_SEND_DATA_OVER_USB:
        status = SendDataToUsbDevice(DeviceObject, Irp);
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS CaptureProcessMemory(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    PVOID inputBuffer = Irp->AssociatedIrp.SystemBuffer;

    if (inputBufferLength < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ULONG pid = *(ULONG*)inputBuffer;
    PEPROCESS process;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &process);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Example: Read memory from the process
    SIZE_T bytesRead;
    PVOID targetAddress = ...; // Set the target memory address
    PVOID buffer = ...; // Buffer to store the memory
    SIZE_T bufferSize = ...; // Size of the buffer

    __try {
        MmCopyVirtualMemory(process, targetAddress, PsGetCurrentProcess(), buffer, bufferSize, KernelMode, &bytesRead);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    ObDereferenceObject(process);
    return status;
}

void DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\??\\ProcessMemoryCapture");
    IoDeleteSymbolicLink(&symbolicLink);
    IoDeleteDevice(DriverObject->DeviceObject);
}
