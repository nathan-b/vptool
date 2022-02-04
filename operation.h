#pragma once

#include <string>

enum operation_type {
	INVALID_OPERATION,
	DUMP_INDEX,
	DUMP_FILE,
	EXTRACT_FILE,
	EXTRACT_ALL,
	REPLACE_FILE,
	BUILD_PACKAGE,
};

enum option_type {
	INVALID_OPTION,
	OUT_PATH,
	IN_PATH,
	PACKAGE_FILE,
};

class operation {
public:
	bool parse(int argc, char** argv);

	operation_type get_type() const { return m_type; }
	const std::string& get_internal_filename() const { return m_vp_filename; }
	const std::string& get_src_filename() const { return m_src_filename; }
	const std::string& get_dest_path() const { return m_dst_path; }
	const std::string& get_package_filename() const { return m_package_filename; }

private:
	operation_type m_type;
	std::string m_vp_filename;
	std::string m_src_filename;
	std::string m_dst_path;
	std::string m_package_filename;
};
