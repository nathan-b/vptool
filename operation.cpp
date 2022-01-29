#include "operation.h"

#include <string>
#include <iostream>

static operation_type string_to_operation_type(const std::string&& arg)
{
	// Possible values:
	//  t dump-index   > DUMP_INDEX
	//  d dump-file    > DUMP_FILE
	//  f extract-file > EXTRACT_FILE
	//  x extract-all  > EXTRACT_ALL
	//  r replace-file > REPLACE_FILE
	//  c p build-package > BUILD_PACKAGE

	// First check for short argument
	if (arg.length() == 1) {
		switch (arg[0]) {
		case 't':
			return DUMP_INDEX;
		case 'd':
			return DUMP_FILE;
		case 'f':
			return EXTRACT_FILE;
		case 'x':
			return EXTRACT_ALL;
		case 'r':
			return REPLACE_FILE;
		case 'c': // Be kind to people who forget this isn't tar
		case 'p':
			return BUILD_PACKAGE;
		default:
			return INVALID_OPERATION;
		}
	}

	// dump_file or dump-file? We accept both!
	// This implementation also accepts garbage at the end of the string, which
	// is just a side effect
	if (arg.substr(0, 4) == "dump") {
		if (arg.substr(5, 4) == "file") {
			return DUMP_FILE;
		} else if (arg.substr(5, 5) == "index") {
			return DUMP_INDEX;
		}
		return INVALID_OPERATION;
	} else if (arg.substr(0, 6) == "extract") {
		if (arg.substr(8, 4) == "file") {
			return EXTRACT_FILE;
		} else if (arg.substr(8, 3) == "all") {
			return EXTRACT_ALL;
		}
		return INVALID_OPERATION;
	} else if (arg.substr(0, 6) == "replace" && arg.substr(7, 4) == "file") {
		return REPLACE_FILE;
	} else if (arg.substr(0, 5) == "build" && arg.substr(6, 7) == "package") {
		return BUILD_PACKAGE;
	}
	return INVALID_OPERATION;
}

static option_type read_option(const std::string& arg)
{
	//  -o  --output-path  > OUT_PATH
	//  -i  --input-file   > IN_FILE
	//  -f  --package-file > PACKAGE_FILE

	if (arg.length() == 2) {
		switch (arg[1]) {
		case 'o':
			return OUT_PATH;
		case 'i':
			return IN_PATH;
		case 'f':
			return PACKAGE_FILE;
		default:
			return INVALID_OPTION;
		}
	}

	if (arg.substr(2, 6) == "output" && arg.substr(9, 4) == "path") {
		return OUT_PATH;
	} else if (arg.substr(2, 5) == "input" && arg.substr(8, 4) == "file") {
		return IN_PATH;
	} else if (arg.substr(2, 7) == "package" && arg.substr(10, 4) == "file") {
		return PACKAGE_FILE;
	}
	return INVALID_OPTION;
}

static inline std::string read_param(int argc, char** argv, int idx)
{
	if (idx >= argc) {
		return "";
	}
	return argv[idx];
}

bool operation::parse(int argc, char** argv)
{
	int arg_idx = 1;

	// First read the operation type
	m_type = string_to_operation_type(read_param(argc, argv, arg_idx));

	if (m_type == INVALID_OPERATION) {
		return false;
	}

	for (arg_idx = 2; arg_idx < argc; ++arg_idx) {
		// Read the parameters for the operation
		std::string param = read_param(argc, argv, arg_idx);

		if (param[0] == '-') {
			option_type opt = read_option(param);
			switch (opt) {
			case OUT_PATH:
				if (!m_dst_path.empty()) {
					std::cerr << "Warning: Multiple output paths specified\n";
				}
				m_dst_path = read_param(argc, argv, ++arg_idx);
				break;
			case IN_PATH:
				if (!m_src_filename.empty()) {
					std::cerr << "Warning: Multiple input files specified\n";
				}
				m_src_filename = read_param(argc, argv, ++arg_idx);
				break;
			case PACKAGE_FILE:
				if (!m_dst_path.empty()) {
					std::cerr << "Warning: Multiple package files specified\n";
				}
				m_vp_filename = read_param(argc, argv, ++arg_idx);
				break;
			case INVALID_OPTION:
				return false;
			}
		} else {
			// This is the input file
			if (!m_package_filename.empty()) {
				std::cerr << "Tragically, I can only process one input file at a time\n";
				return false;
			}
			m_package_filename = param;
		}
	}

	return true;
}
