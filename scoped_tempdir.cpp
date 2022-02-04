#include "scoped_tempdir.h"

#include <cstdint>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <system_error>

scoped_tempdir::scoped_tempdir(std::string prefix)
{
	const uint32_t max_attempts = 1024;
	std::random_device rd;
	std::default_random_engine reng(rd());
	std::uniform_int_distribution rand(100000, 999999);
	m_valid = false;

	uint32_t attempts = 0;
	while (attempts++ < max_attempts) {
		std::stringstream ss;
		ss << prefix << rand(reng);
		std::filesystem::path path = std::filesystem::temp_directory_path() / ss.str();

		if (std::filesystem::create_directory(path)) {
			// We got our directory
			m_dir = path;
			m_valid = true;
			return;
		}
	}
}

scoped_tempdir::~scoped_tempdir()
{
	std::error_code ec;
	std::filesystem::remove_all(m_dir, ec);
}

std::filesystem::path scoped_tempdir::get_path() const
{
	return m_dir;
}

scoped_tempdir::operator std::filesystem::path() const
{
	return m_dir;
}

scoped_tempdir::operator std::string() const
{
	return m_dir;
}

scoped_tempdir::operator bool() const
{
	return m_valid;
}

std::filesystem::path scoped_tempdir::operator/(const std::filesystem::path& rhs) const
{
	return m_dir / rhs;
}
