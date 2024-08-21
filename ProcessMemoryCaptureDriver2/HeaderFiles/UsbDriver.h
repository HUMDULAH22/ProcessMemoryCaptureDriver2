#ifndef _USB_DRIVER_H_
#define _USB_DRIVER_H_

#include <ntddk.h>
#include <wdf.h>

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    WDFUSBDEVICE UsbDevice;
    WDFUSBPIPE BulkReadPipe;
    WDFUSBPIPE BulkWritePipe;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS UsbDeviceInitialize(PDEVICE_OBJECT DeviceObject, PDEVICE_EXTENSION DeviceExtension);
NTSTATUS SendDataToUsbDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp);

#endif
