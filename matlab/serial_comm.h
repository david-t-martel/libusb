#pragma once
#include <libusb-1.0/libusb.h>
#include <stdbool.h>

// Correct VID/PID definitions
#define FTDI_VID 0x0403	 // FTDI
#define STM32_VID 0x0483 // STM32

typedef struct
{
	uint16_t vid;
	uint16_t pid;
	char serial[256];
	char description[256];
	libusb_device_handle *handle;
	uint32_t timeout_ms; // Added timeout configuration
} usb_device_info;

bool discover_devices(usb_device_info *devices, int *count);
bool open_device(usb_device_info *device);
bool send_data(usb_device_info *device, uint8_t *data, int length);
bool receive_data(usb_device_info *device, uint8_t *buffer, int length, int *received);
void close_device(usb_device_info *device);