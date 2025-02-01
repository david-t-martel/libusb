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
	struct DeviceConfig
	{
		uint32_t timeout_ms = 1000;
		size_t buffer_size = 4096;
		bool auto_reconnect = true;
	};

	std::unique_ptr<usb_device_info> device;
	matlab::data::ArrayFactory factory;
	bool is_initialized = false;
	DeviceConfig config;

	void validateState();
	void validateInputs(const matlab::mex::ArgumentList &inputs, size_t min_args);
	void listDevices(matlab::mex::ArgumentList &outputs, matlab::mex::ArgumentList &inputs);
	void openDevice(matlab::mex::ArgumentList &outputs, matlab::mex::ArgumentList &inputs);
	void writeData(matlab::mex::ArgumentList &outputs, matlab::mex::ArgumentList &inputs);
	void readData(matlab::mex::ArgumentList &outputs, matlab::mex::ArgumentList &inputs);
	void closeDevice();
	void setConfig(matlab::mex::ArgumentList &inputs);
	void getConfig(matlab::mex::ArgumentList &outputs);

public:
	void operator()(matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs);
	~MexFunction() { closeDevice(); }
};