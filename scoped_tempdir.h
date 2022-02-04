#pragma once

#include <filesystem>
#include <string>

class scoped_tempdir
{
public:
  scoped_tempdir(std::string prefix = "");
  ~scoped_tempdir();

  std::filesystem::path get_path() const;
  operator std::filesystem::path() const;
  operator std::string() const;
  operator bool() const;
  std::filesystem::path operator /(const std::filesystem::path& rhs) const;

private:
  std::filesystem::path m_dir;
  bool m_valid;
};
