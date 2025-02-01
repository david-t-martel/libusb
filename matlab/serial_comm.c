#include "serial_comm.h"
#include <stdio.h>

bool discover_devices(usb_device_info *devices, int *count)
{
	libusb_device **list;
	libusb_context *ctx = NULL;
	int found = 0;

	libusb_init(&ctx);
	ssize_t cnt = libusb_get_device_list(ctx, &list);

	for (ssize_t i = 0; i < cnt && found < *count; i++)
	{
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(list[i], &desc);

		if (desc.idVendor == FTDI_VID || desc.idVendor == STM32_VID)
		{
			devices[found].vid = desc.idVendor;
			devices[found].pid = desc.idProduct;

			libusb_device_handle *handle;
			if (libusb_open(list[i], &handle) == 0)
			{
				libusb_get_string_descriptor_ascii(handle,
												   desc.iSerialNumber,
												   (unsigned char *)devices[found].serial,
												   sizeof(devices[found].serial));
				libusb_get_string_descriptor_ascii(handle,
												   desc.iProduct,
												   (unsigned char *)devices[found].description,
												   sizeof(devices[found].description));
				libusb_close(handle);
				found++;
			}
		}
	}

	*count = found;
	libusb_free_device_list(list, 1);
	libusb_exit(ctx);
	return found > 0;
}

bool open_device(usb_device_info *device)
{
	libusb_context *ctx = NULL;
	libusb_init(&ctx);

	device->handle = NULL;
	libusb_device **list;
	ssize_t cnt = libusb_get_device_list(ctx, &list);

	for (ssize_t i = 0; i < cnt; i++)
	{
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(list[i], &desc);

		if (desc.idVendor == device->vid && desc.idProduct == device->pid)
		{
			libusb_open(list[i], &device->handle);
			if (device->handle)
			{
				libusb_claim_interface(device->handle, 0);
				break;
			}
		}
	}

	libusb_free_device_list(list, 1);
	return device->handle != NULL;
}

bool send_data(usb_device_info *device, uint8_t *data, int length)
{
	int transferred;
	int result = libusb_bulk_transfer(
		device->handle,
		0x01, // OUT endpoint
		data,
		length,
		&transferred,
		1000 // 1 second timeout
	);
	return (result == 0 && transferred == length);
}

bool receive_data(usb_device_info *device, uint8_t *buffer, int length, int *received)
{
	int result = libusb_bulk_transfer(
		device->handle,
		0x81, // IN endpoint
		buffer,
		length,
		received,
		1000 // 1 second timeout
	);
	return result == 0;
}

void close_device(usb_device_info *device)
{
	if (device->handle)
	{
		libusb_release_interface(device->handle, 0);
		libusb_close(device->handle);
		device->handle = NULL;
	}
}