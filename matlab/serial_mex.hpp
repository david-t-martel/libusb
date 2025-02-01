#pragma once
#include "mex.hpp"
#include "mexAdapter.hpp"
#include "serial_comm.h"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

class MexFunction : public matlab::mex::Function
{
private:
	std::unique_ptr<device_context_t, void (*)(device_context_t *)> device;
	matlab::data::ArrayFactory factory;

	// Simplified configuration handling
	void updateConfig(const matlab::data::StructArray &config)
	{
		device_config_t new_config = {};
		// Parse MATLAB struct directly into device config
		if (device)
		{
			device_configure(device.get(), &new_config);
		}
	}

	// Simplified device management
	void openDevice(matlab::mex::ArgumentList &outputs,
					matlab::mex::ArgumentList &inputs)
	{
		if (!device)
		{
			device.reset(device_create());
		}
		// ...rest of open logic...
	}

	bool is_initialized = false;
	DeviceConfig config;

	void validateState();
	void validateInputs(const matlab::mex::ArgumentList &inputs, size_t min_args);
	void listDevices(matlab::mex::ArgumentList &outputs, matlab::mex::ArgumentList &inputs);
	void writeData(matlab::mex::ArgumentList &outputs, matlab::mex::ArgumentList &inputs);
	void readData(matlab::mex::ArgumentList &outputs, matlab::mex::ArgumentList &inputs);
	void closeDevice();
	void setConfig(matlab::mex::ArgumentList &inputs);
	void getConfig(matlab::mex::ArgumentList &outputs);

	// Error handling utilities
	void handleDeviceError(const std::exception &err)
	{
		closeDevice();
		throwMatlabError("MATLAB:error:device_error", err.what());
	}

	void throwMatlabError(const std::string &id, const std::string &msg)
	{
		matlab::data::ArrayFactory factory;
		throwException(factory.createScalar(id),
					   factory.createScalar(msg));
	}

	// Buffer management
	void flushBuffer()
	{
		if (device && device->handle)
		{
			uint8_t dummy[64];
			int received;
			while (receive_data(device.get(), dummy, sizeof(dummy), &received))
			{
				if (received == 0)
					break;
			}
		}
	}

	size_t getAvailableBuffer()
	{
		return config.buffer_size - circular_buffer.size();
	}

	// Add error handling helper
	void checkDeviceError()
	{
		if (device)
		{
			char error[256];
			if (device_get_error(device.get(), error, sizeof(error)))
			{
				throwMatlabError("MATLAB:error:device", error);
			}
		}
	}

	// Add configuration parser
	device_config_t parseConfig(const matlab::data::StructArray &config)
	{
		device_config_t result = {};
		try
		{
			if (config.hasField("timeout_ms"))
			{
				result.usb_config.timeout_ms =
					static_cast<uint32_t>(config[0]["timeout_ms"][0]);
			}
			if (config.hasField("auto_reconnect"))
			{
				result.flags |= config[0]["auto_reconnect"][0] ? DEVICE_FLAG_AUTO_RECONNECT : 0;
			}
		}
		catch (const matlab::data::TypeMismatchException &)
		{
			throwMatlabError("MATLAB:error:config", "Invalid configuration type");
		}
		return result;
	}

public:
	void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs);
	~MexFunction() { closeDevice(); }
};