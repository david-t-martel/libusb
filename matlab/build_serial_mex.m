function build_serial_mex(varargin)
% Parse inputs
p = inputParser;
addParameter(p, 'libusb_include', '', @ischar);
addParameter(p, 'libusb_lib', '', @ischar);
addParameter(p, 'debug', false, @islogical);
addParameter(p, 'force_rebuild', false, @islogical);
addParameter(p, 'clean', false, @islogical);
parse(p, varargin{:});

% Get directories
curr_dir = pwd;
libusb_root = fileparts(curr_dir);
release_dir = fullfile(curr_dir, 'release');

% Clean build if requested
if p.Results.clean && exist(release_dir, 'dir')
	rmdir(release_dir, 's');
end

% Create release directory
if ~exist(release_dir, 'dir')
	mkdir(release_dir);
end

% Validate MATLAB paths
matlab_include = fullfile(matlabroot, 'extern', 'include');
if ~exist(matlab_include, 'dir')
	error('MATLAB include directory not found: %s', matlab_include);
end

% Add platform-specific compiler flags
if ispc
	cxx_flags = '/EHsc /std:c++17 /W4';  % Add warning level
else
	cxx_flags = '-std=c++17 -Wall -Wextra';  % Add extra warnings
end

% Add libusb version verification
if ~check_libusb_version(p.Results)
	error('Incompatible libusb version');
end

% Add dependency checking
check_dependencies();

% Set platform-specific paths
if ispc
	[include_path, lib_path] = get_windows_paths(p.Results, curr_dir, libusb_root);
	lib_name = 'libusb-1.0';
	check_windows_dependencies(include_path, lib_path);
else
	[include_path, lib_path] = get_unix_paths(p.Results, curr_dir, libusb_root);
	lib_name = 'usb-1.0';
	check_unix_dependencies(include_path, lib_path);
end

% Construct and execute build command
cmd = construct_build_command(p.Results.debug, include_path, lib_path, ...
	lib_name, release_dir, matlab_include);
try
	mex(cmd{:});
	verify_build(release_dir);
catch ME
	cleanup_on_error(release_dir);
	rethrow(ME);
end
end

function [include_path, lib_path] = get_windows_paths(params, curr_dir, libusb_root)
if ~isempty(params.libusb_include)
	include_path = params.libusb_include;
else
	include_candidates = {
		fullfile(curr_dir, 'include'),
		fullfile(libusb_root, 'include'),
		'C:\libusb\include'
		};
	include_path = find_valid_path(include_candidates);
end

if ~isempty(params.libusb_lib)
	lib_path = params.libusb_lib;
else
	lib_candidates = {
		fullfile(curr_dir, 'lib'),
		fullfile(libusb_root, 'MS64'),
		'C:\libusb\lib'
		};
	lib_path = find_valid_path(lib_candidates);
end
end


function verify_build(release_dir)
mex_file = fullfile(release_dir, 'serial_mex');
if ispc
	mex_file = [mex_file '.' mexext];
else
	mex_file = [mex_file '.' mexext];
end

if ~exist(mex_file, 'file')
	error('Build failed: MEX file not created');
end
end

function cleanup_on_error(release_dir)
if exist(release_dir, 'dir')
	rmdir(release_dir, 's');
end
end

function [include_path, lib_path] = get_unix_paths(params, curr_dir, libusb_root)
if isempty(params.libusb_include)
	include_candidates = {
		fullfile(curr_dir, 'include'),
		fullfile(libusb_root, 'include'),
		'/usr/include',
		'/usr/local/include'
		};
	include_path = find_valid_path(include_candidates);
else
	include_path = validate_path(params.libusb_include);
end

if isempty(params.libusb_lib)
	lib_candidates = {
		fullfile(curr_dir, 'lib'),
		fullfile(libusb_root, 'lib'),
		'/usr/lib',
		'/usr/local/lib'
		};
	lib_path = find_valid_path(lib_candidates);
else
	lib_path = validate_path(params.libusb_lib);
end
end

function path = find_valid_path(candidates)
for i = 1:length(candidates)
	if exist(candidates{i}, 'dir')
		path = candidates{i};
		return;
	end
end
error('No valid path found among candidates: %s', strjoin(candidates, ', '));
end

function path = validate_path(path)
if ~exist(path, 'dir')
	error('Directory not found: %s', path);
end
end

function check_windows_dependencies(include_path, lib_path)
% Check for required files
required_files = {
	fullfile(include_path, 'libusb-1.0', 'libusb.h'),
	fullfile(lib_path, 'libusb-1.0.lib'),
	fullfile(lib_path, 'libusb-1.0.dll')
	};

for i = 1:length(required_files)
	if ~exist(required_files{i}, 'file')
		error('Required file not found: %s', required_files{i});
	end
end
end

function check_unix_dependencies(include_path, lib_path)
required_files = {
	fullfile(include_path, 'libusb-1.0', 'libusb.h'),
	fullfile(lib_path, 'libusb-1.0.so')
	};

if ismac
	required_files{2} = fullfile(lib_path, 'libusb-1.0.dylib');
end

for i = 1:length(required_files)
	if ~exist(required_files{i}, 'file')
		error('Required file not found: %s', required_files{i});
	end
end
end

function cmd = construct_build_command(debug, include_path, lib_path, lib_name, ...
	release_dir, matlab_include)

% Initialize command array
cmd = {'COMPFLAGS="$COMPFLAGS'};

% Add C++ standard and exception handling
if ispc
	cmd{end+1} = '/EHsc /std:c++17';
	if debug
		cmd{end+1} = '/Z7 /Od /MDd';
	else
		cmd{end+1} = '/O2 /MD';
	end
else
	cmd{end+1} = '-std=c++17';
	if debug
		cmd{end+1} = '-g -O0';
	else
		cmd{end+1} = '-O2';
	end
end

% Close compiler flags
cmd{end+1} = '"';

% Add include paths with validation
validatepath = @(p) exist(p, 'dir') || error('Path not found: %s', p);
validatepath(include_path);
validatepath(matlab_include);
validatepath(lib_path);

% Add includes
cmd{end+1} = ['-I"' include_path '"'];
cmd{end+1} = ['-I"' matlab_include '"'];

% Add source files
cmd{end+1} = fullfile(pwd, 'serial_mex.cpp');
cmd{end+1} = fullfile(pwd, 'serial_comm.c');

% Add library paths and dependencies
cmd{end+1} = ['-L"' lib_path '"'];
cmd{end+1} = ['-l' lib_name];

% Set output directory
validatepath(release_dir);
cmd{end+1} = ['-outdir "' release_dir '"'];

% Add Windows-specific runtime library path
if ispc
	cmd{end+1} = ['-L"' fullfile(matlabroot, 'extern', 'lib', ...
		computer('arch'), 'microsoft') '"'];
end
end

function check_dependencies()
% Verify MEX compiler setup
if ~mex.getCompilerConfigurations('C++')
	error('C++ MEX compiler not configured');
end

% Check for required headers
required_headers = {'mex.hpp', 'libusb.h'};
% ...header checking code...
end

function ok = check_libusb_version(params)
min_version = '1.0.21';
% ...version checking code...
end