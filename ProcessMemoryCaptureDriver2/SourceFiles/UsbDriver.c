#include "UsbDriver.h"
#include <wdf.h>
#include <wdfusb.h> 

NTSTATUS UsbDeviceInitialize(PDEVICE_OBJECT DeviceObject, PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS status;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFUSBPIPE pipe;
    WDF_USB_DEVICE_INFORMATION deviceInfo;
    WDFUSBINTERFACE usbInterface;

    WDF_USB_DEVICE_INFORMATION_INIT(&deviceInfo);

    // Create the USB device object
    status = WdfUsbTargetDeviceCreateWithParameters(
        DeviceObject,
        NULL,
        WDF_NO_OBJECT_ATTRIBUTES,
        &DeviceExtension->UsbDevice
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Select the USB configuration
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
    status = WdfUsbTargetDeviceSelectConfig(
        DeviceExtension->UsbDevice,
        WDF_NO_OBJECT_ATTRIBUTES,
        &configParams
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Get the default USB interface
    usbInterface = configParams.Types.SingleInterface.ConfiguredUsbInterface;

    // Find the bulk IN and OUT pipes
    for (UCHAR i = 0; i < WdfUsbInterfaceGetNumConfiguredPipes(usbInterface); i++) {
        pipe = WdfUsbInterfaceGetConfiguredPipe(usbInterface, i, NULL);
        WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

        if (WdfUsbTargetPipeIsInEndpoint(pipe)) {
            DeviceExtension->BulkReadPipe = pipe;
        }
        else if (WdfUsbTargetPipeIsOutEndpoint(pipe)) {
            DeviceExtension->BulkWritePipe = pipe;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS SendDataToUsbDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PVOID dataBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG dataSize = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS status;

    WDF_MEMORY_DESCRIPTOR memoryDescriptor;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, dataBuffer, dataSize);

    // Send data to the USB device using a bulk transfer
    status = WdfUsbTargetPipeWriteSynchronously(
        deviceExtension->BulkWritePipe,
        WDF_NO_HANDLE,
        NULL,
        &memoryDescriptor,
        NULL
    );

    return status;
}
