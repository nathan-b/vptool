#include "vp_parser.h"

#include <functional>
#include <string>
#include <list>
#include <fstream>
#include <iostream>
#include <sstream>

//////////////////////////////////////////////////////////////
// Pieces of a VP file used for parsing
const uint32_t vp_sig = 0x50565056;

struct vp_header
{
	char header[4]; // Always "VPVP"
	int version;    // As of this version, still 2.
	int diroffset;  // Offset to the file index
	int direntries; // Number of entries
};

struct vp_direntry
{
	int offset;    // Offset of the file data for this entry.
	int size;      // Size of the file data for this entry
	char name[32]; // Null-terminated filename, directory name, or ".." for backdir
	int timestamp; // Time the file was last modified, in unix time.
};

/////////////////////////////////////////////////////////////
// vp_index methods

vp_index::~vp_index()
{
  if (m_root) {
    delete m_root;
  }
}

bool vp_index::parse(const std::string&& path)
{
  vp_header header;
  std::ifstream infile(path, std::ios::in | std::ios::binary);
  infile.read((char*)&header, sizeof(header));

  if (!infile) {
    std::cerr << "Error while reading file " << path << std::endl;
    return false;
  }

  if (*(uint32_t*)&header.header != vp_sig) {
    std::cerr << path << ": File signature incorrect: " << header.header << std::endl;
    return false;
  }

  m_filename = path;
  // Create the root node
  m_root = new vp_directory(".", nullptr);

  // Seek to the index
  infile.seekg(header.diroffset);

  // Now read in each entry one by one
  vp_directory* current = m_root;
  for (uint32_t i = 0; i < header.direntries; ++i) {
    vp_direntry entry;
    infile.read((char*)&entry, sizeof(entry));

    // Check if directory
    if (entry.size == 0) {
      // Am I an updir?
      if (entry.name[0] == '.' && entry.name[1] == '.' && entry.name[2] == '\0') {
        current = current->get_parent();
        if (!current) {
          std::cerr << path << ": Unexpected updir; already at top level!\n";
          delete m_root;
          m_root = nullptr;
					return false;
        }
      } else {
        // Not an updir; create a new directory node
        vp_directory* new_dir = new vp_directory(entry.name, current);
        current->add_child(new_dir);
        current = new_dir;
      }
    } else {
      // Not a directory
      vp_file* new_file = new vp_file(entry.name, entry.offset, entry.size, current);
      current->add_child(new_file);
    }
  }

  return true;
}

vp_file* vp_index::find(const std::string&& name) const
{
  if (m_root) {
    return m_root->find(name);
  }
  return nullptr;
}

std::string vp_index::print_index_listing() const
{
  int level = 0;
  std::stringstream ss;
  std::function<void(vp_node*)> print_func = [&level, &ss, &print_func](vp_node* child) {
    for (uint32_t indent = 0; indent < level; ++indent) {
      ss << "   ";
    }
    ss << child->to_string() << "\n";
    ++level;
    child->foreach_child(print_func);
    --level;
  };

  m_root->foreach_child(print_func);
  return ss.str();
}

////////////////////////////////////////////////////////////////
/// vp_directory methods

vp_directory::vp_directory(const std::string&& name, vp_directory* parent)
  : vp_node(parent),
    m_name(name)
{}

vp_directory::~vp_directory()
{
  for (auto* child : m_children) {
    delete child;
  }
}

const std::string& vp_directory::get_name() const
{
  return m_name;
}

vp_file* vp_directory::find(const std::string& name)
{
  for (auto* node : m_children) {
    vp_file* result = node->find(name);
    if (result) {
      return result;
    }
  }
  return nullptr;
}

std::string vp_directory::to_string() const
{
  return m_name + "/";
}

void vp_directory::add_child(vp_node* child)
{
  m_children.push_back(child);
}

void vp_directory::foreach_child(std::function<void(vp_node*)> f)
{
  for (auto* child : m_children) {
    f(child);
  }
}

///////////////////////////////////////////////////////////////////
/// vp_file methods

vp_file::vp_file(const std::string&& name, uint32_t offset, uint32_t size, vp_directory* parent)
  : vp_node(parent),
    m_name(name),
    m_offset(offset),
    m_size(size)
{
}

const std::string& vp_file::get_name() const
{
  return m_name;
}

vp_file* vp_file::find(const std::string& name)
{
  if (m_name == name) {
    return this;
  }
  return nullptr;
}

std::string vp_file::to_string() const
{
  return m_name;
}

void vp_file::foreach_child(std::function<void(vp_node*)> f)
{
  // vp_file has no children
}

std::string vp_file::dump() const
{
  //std::string retval(m_size);
  return "";
}

bool vp_file::dump(const std::string&& path) const
{
  return false;
}
