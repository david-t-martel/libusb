#pragma once
#include <libusb-1.0/libusb.h>
#include <stdbool.h>

#define FTDI_VID 0x0403
#define STM32_VID 0x0483

typedef enum
{
	DEVICE_STATE_CLOSED = 0,
	DEVICE_STATE_OPENING,
	DEVICE_STATE_OPEN,
	DEVICE_STATE_ERROR
} device_state_t;

typedef enum
{
	SERIAL_SUCCESS = 0,
	SERIAL_ERROR_NOT_FOUND = -1,
	SERIAL_ERROR_ACCESS = -2,
	SERIAL_ERROR_BUSY = -3,
	SERIAL_ERROR_TIMEOUT = -4,
	SERIAL_ERROR_INVALID_CONFIG = -5
} serial_error_t;

typedef struct
{
	uint32_t timeout_ms;
	uint32_t buffer_size;
	uint8_t endpoint_in;
	uint8_t endpoint_out;
} serial_config_t;

typedef struct
{
	uint16_t vid;
	uint16_t pid;
	char serial[256];
	char description[256];
	libusb_device_handle *handle;
	device_state_t state;
	serial_error_t last_error;
	serial_config_t config;
} usb_device_info;

// Function declarations
bool discover_devices(usb_device_info *devices, int *count);
bool open_device(usb_device_info *device);
bool send_data(usb_device_info *device, uint8_t *data, int length);
bool receive_data(usb_device_info *device, uint8_t *buffer, int length, int *received);
void close_device(usb_device_info *device);
bool set_device_config(usb_device_info *device, const serial_config_t *config);
serial_error_t get_last_error(usb_device_info *device);