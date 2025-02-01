classdef SerialDevice < handle % was serialDevice
	properties (Access = private)
		IsOpen = false
		Config = struct('timeout_ms', 1000, ...
			'buffer_size', 4096, ...
			'auto_reconnect', true)
	end

	methods
		function obj = SerialDevice()
		end

		function devices = list(obj)
			devices = serial_mex('list');
		end

		function open(obj, vid, pid, serial)
			if obj.IsOpen
				error('Device already open');
			end

			if nargin < 4
				serial_mex('open', vid, pid);
			else
				serial_mex('open', vid, pid, serial);
			end
			obj.IsOpen = true;
		end

		function write(obj, data)
			validateattributes(data, {'uint8'}, {'vector'});
			if ~obj.IsOpen
				error('Device not open');
			end
			serial_mex('write', data);
		end

		function data = read(obj, bytes)
			if ~obj.IsOpen
				error('Device not open');
			end
			try
				if nargin < 2
					bytes = obj.Config.buffer_size;
				end
				data = serial_mex('read', bytes);
			catch ME
				if strcmp(ME.identifier, 'MATLAB:error:device_disconnected')
					obj.close();
				end
				rethrow(ME);
			end
		end

		function close(obj)
			if obj.IsOpen
				serial_mex('close');
				obj.IsOpen = false;
			end
		end

		function delete(obj)
			obj.close();
		end

		function setConfig(obj, name, value)
			validateattributes(value, {'numeric', 'logical'}, {'scalar'});
			if ~isfield(obj.Config, name)
				error('Invalid configuration parameter: %s', name);
			end
			obj.Config.(name) = value;
			if obj.IsOpen
				serial_mex('setConfig', name, value);
			end
		end
	end
end
