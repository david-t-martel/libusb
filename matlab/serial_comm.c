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
				devices[found].state = DEVICE_STATE_CLOSED;
				devices[found].last_error = SERIAL_SUCCESS;
				found++;
			}
		}
	}

	*count = found;
	libusb_free_device_list(list, 1);
	libusb_exit(ctx);
	return found > 0;
}

void setConfig(matlab::mex::ArgumentList &inputs)
{
	validateState();
	validateInputs(inputs, 2);

	std::string param_name = inputs[0][0];
	double value = inputs[1][0];

	serial_config_t new_config = device->config;

	if (param_name == "timeout_ms")
	{
		new_config.timeout_ms = static_cast<uint32_t>(value);
	}
	else if (param_name == "buffer_size")
	{
		new_config.buffer_size = static_cast<uint32_t>(value);
	}
	else if (param_name == "endpoint_in")
	{
		new_config.endpoint_in = static_cast<uint8_t>(value);
	}
	else if (param_name == "endpoint_out")
	{
		new_config.endpoint_out = static_cast<uint8_t>(value);
	}
	else
	{
		throw std::invalid_argument("Invalid configuration parameter");
	}

	if (!set_device_config(device.get(), &new_config))
	{
		throw std::runtime_error("Failed to set device configuration");
	}
}

bool open_device(usb_device_info *device)
{
	device->state = DEVICE_STATE_OPENING;
	libusb_context *ctx = NULL;
	libusb_init(&ctx);

	libusb_device **list;
	ssize_t cnt = libusb_get_device_list(ctx, &list);
	bool success = false;

	for (ssize_t i = 0; i < cnt; i++)
	{
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(list[i], &desc);

		if (desc.idVendor == device->vid && desc.idProduct == device->pid)
		{
			if (libusb_open(list[i], &device->handle) == 0)
			{
				libusb_claim_interface(device->handle, 0);
				device->state = DEVICE_STATE_OPEN;
				success = true;
				break;
			}
		}
	}

	libusb_free_device_list(list, 1);

	if (!success)
	{
		device->state = DEVICE_STATE_ERROR;
		device->last_error = SERIAL_ERROR_NOT_FOUND;
	}

	return success;
}

bool send_data(usb_device_info *device, uint8_t *data, int length)
{
	if (device->state != DEVICE_STATE_OPEN)
	{
		device->last_error = SERIAL_ERROR_NOT_FOUND;
		return false;
	}

	int transferred;
	int result = libusb_bulk_transfer(
		device->handle,
		device->config.endpoint_out,
		data,
		length,
		&transferred,
		device->config.timeout_ms);
	if (result != 0 || transferred != length)
	{
		device->last_error = SERIAL_ERROR_TIMEOUT;
		return false;
	}

	return true;
}

bool receive_data(usb_device_info *device, uint8_t *buffer, int length, int *received)
{
	if (device->state != DEVICE_STATE_OPEN)
	{
		device->last_error = SERIAL_ERROR_NOT_FOUND;
		return false;
	}

	int result = libusb_bulk_transfer(
		device->handle,
		device->config.endpoint_in,
		buffer,
		length,
		received,
		device->config.timeout_ms);
	if (result != 0)
	{
		device->last_error = SERIAL_ERROR_TIMEOUT;
		return false;
	}

	return true;
}

void close_device(usb_device_info *device)
{
	if (device->handle)
	{
		libusb_release_interface(device->handle, 0);
		libusb_close(device->handle);
		device->handle = NULL;
	}
	device->state = DEVICE_STATE_CLOSED;
}