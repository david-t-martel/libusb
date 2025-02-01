% Add release directory to path
addpath(fullfile(pwd, 'release'));

% Build with local paths first, then custom paths if specified
try
	build_serial_mex('debug', true);
catch
	% If local build fails, try with custom paths
	build_serial_mex('libusb_include', 'C:\custom\include', ...
		'libusb_lib', 'C:\custom\lib', ...
		'debug', true);
end

% Create device object
dev = SerialDevice();

try
	% List available devices
	devices = dev.list()

	% Open first STM32 device found
	dev.open(hex2dec('0483'), hex2dec('5740'))

	% Send data
	dev.write(uint8([1 2 3 4]))

	% Read response
	response = dev.read(64)

catch ME
	% Error handling
	fprintf('Error: %s\n', ME.message);
	dev.close();
	rethrow(ME);
end

% Clean up
dev.close();
rmpath(fullfile(pwd, 'release'));