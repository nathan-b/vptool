#include <iostream>
#include <list>

#include "vp_parser.h"

enum operation_type
{
	INVALID_OPERATION,
	DUMP_INDEX,
	DUMP_FILE,
	EXTRACT_FILE,
	EXTRACT_ALL,
	REPLACE_FILE,
	BUILD_PACKAGE,
};

enum option_type
{
	INVALID_OPTION,
	OUT_PATH,
	IN_PATH,
	PACKAGE_FILE,
};

struct operation
{
	operation_type type;
	std::string vp_filename;
	std::string src_filename;
	std::string dst_path;
};

operation_type string_to_operation_type(const std::string&& arg)
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

option_type read_option(const std::string& arg)
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

bool parse_cmd_line(int argc, char** argv, operation& op, std::string& vp_file)
{
	int arg_idx = 1;

	// First read the operation type
	op.type = string_to_operation_type(read_param(argc, argv, arg_idx));

	if (op.type == INVALID_OPERATION) {
		return false;
	}

	for (arg_idx = 2; arg_idx < argc; ++arg_idx) {
		// Read the parameters for the operation
		std::string param = read_param(argc, argv, arg_idx);

		if (param[0] == '-') {
			option_type opt = read_option(param);
			switch (opt) {
			case OUT_PATH:
				if (!op.dst_path.empty()) {
					std::cerr << "Multiple output paths specified";
				}
				op.dst_path = read_param(argc, argv, ++arg_idx);
				break;
			case IN_PATH:
				if (!op.src_filename.empty()) {
					std::cerr << "Multiple input files specified";
				}
				op.src_filename = read_param(argc, argv, ++arg_idx);
				break;
			case PACKAGE_FILE:
				if (!op.dst_path.empty()) {
					std::cerr << "Multiple package files specified";
				}
				op.vp_filename = read_param(argc, argv, ++arg_idx);
				break;
			case INVALID_OPTION:
				return false;
			}
		} else {
			// This is the input file
			if (!vp_file.empty()) {
				std::cerr << "Tragically, I can only process one input file at a time\n";
				return false;
			}
			vp_file = param;
		}
	}

	return true;
}

bool dump_index(const vp_index* idx)
{
	std::cout << idx->print_index_listing() << std::endl;
	return true;
}

bool dump_file(const vp_index* idx, const std::string& filename, const std::string outfilename)
{
	vp_file* f = idx->find(filename);

	if (!f) {
		std::cerr << "Could not find " << filename << " in " << idx->to_string() << std::endl;
		return false;
	}

	if (outfilename.empty()) {
		// Dump to console
		std::cout << f->get_name() << std::endl;
		std::cout << "==============================================================\n";
		std::cout << f->dump() << std::endl;
	} else {
		// Dump to file (i.e. extract)
		return f->dump(outfilename);
	}

	return true;
}

bool dump_file(const vp_index* idx, const std::string& filename)
{
	return dump_file(idx, filename, "");
}

bool extract_all(const vp_index* idx, const std::string& outpath)
{
	return false;
}

bool replace_file(vp_index* idx, const std::string& filename, const std::string& infilename)
{
	return false;
}

bool build_package(std::string& vp_filename, const std::string& src_path)
{
	return false;
}

void usage()
{
	std::cout << "Usage: vptool <operation> <vp_file> [options]\n"
	          << "  Valid operations: t / dump-index             Print the index of the package file\n"
						<< "                    d / dump-file  <-f filename>  Dump the contents of a single file in the package\n"
						<< "                    f / extract-file <-f filename> <-o output-file>  Extract the contents of a single file to disk\n"
						<< "                    x / extract-all  [-o output-path]  Extract the entire package to the output path (or current directory)\n"
						<< "                    r / replace-file <-f filename> <-i input-file>  Replace the contents of a single file\n"
						<< "                    p / build-package <-i input-path>  Build a new vp file with the contents of input-path\n";
}

int main(int argc, char** argv)
{
	operation op;
	std::string vp_file;

	if (!parse_cmd_line(argc, argv, op, vp_file)) {
		std::cerr << "Command line input error\n";
		usage();
		return -1;
	}

	if (op.type == BUILD_PACKAGE) {
		// Build package operations don't parse an index file beforehand
		if (!build_package(vp_file, op.src_filename)) {
			std::cerr << "Error building package " << vp_file << std::endl;
			return -2;
		}
		std::cout << "Success!\n";
		return 0;
	}

	// Parse the index file
	vp_index* idx = new vp_index();
	if (!idx->parse(vp_file)) {
		std::cerr << "Error parsing " << vp_file << std::endl;
		delete idx;
		return -2;
	}

	bool ret = false;
	switch (op.type) {
	case DUMP_INDEX:
		ret = dump_index(idx);
		break;
	case DUMP_FILE:
		ret = dump_file(idx, op.vp_filename);
		break;
	case EXTRACT_FILE:
		ret = dump_file(idx, op.vp_filename, op.dst_path);
		break;
	case EXTRACT_ALL:
		ret = extract_all(idx, op.dst_path);
		break;
	case REPLACE_FILE:
		ret = replace_file(idx, op.vp_filename, op.src_filename);
		break;
	default:
		return -1;
	}

	if (!ret) {
		std::cerr << "Operation did not complete successfully!\n";
	} else {
		std::cout << "Success!\n";
	}

	// Cleanup
	delete idx;

	return 0;
}
