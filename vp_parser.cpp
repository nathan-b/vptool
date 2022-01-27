#include "vp_parser.h"

#include <filesystem>
#include <functional>
#include <string>
#include <list>
#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>

//////////////////////////////////////////////////////////////
/// Pieces of a VP file used for parsing
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
/// vp_index methods

vp_index::~vp_index()
{
  if (m_root) {
    delete m_root;
  }
  if (m_filestream) {
    m_filestream->close();
    delete m_filestream;
  }
}

std::string vp_index::to_string() const
{
  return m_filename;
}

bool vp_index::parse(const std::string& path)
{
  vp_header header;
  m_filestream = new std::ifstream(path, std::ios::in | std::ios::binary);
  m_filestream->read((char*)&header, sizeof(header));

  if (!*m_filestream) {
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
  m_filestream->seekg(header.diroffset);

  // Now read in each entry one by one
  vp_directory* current = m_root;
  for (int i = 0; i < header.direntries; ++i) {
    vp_direntry entry;
    m_filestream->read((char*)&entry, sizeof(entry));

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
      vp_file* new_file = new vp_file(entry.name,
                                      entry.offset,
                                      entry.size,
                                      current,
                                      m_filestream);
      current->add_child(new_file);
    }
  }

  return true;
}

vp_file* vp_index::find(const std::string& name) const
{
  if (m_root) {
    return m_root->find(name);
  }
  return nullptr;
}

std::string vp_index::print_index_listing() const
{
  uint32_t level = 0;
  std::stringstream ss;

	// Build the recursive function to apply to each node
  std::function<void(vp_node*)> print_func = [&level, &ss, &print_func](vp_node* child) {
    for (uint32_t indent = 0; indent < level; ++indent) {
      ss << "   ";
    }
    ss << child->to_string() << "\n";

    ++level;
    child->foreach_child(print_func);
    --level;
  };

	// Apply to root node (which will recurse down the tree depth-wise)
	if (m_root) {
  	m_root->foreach_child(print_func);
	}
  return ss.str();
}

bool vp_index::dump(const std::string& dest_path) const
{
	if (m_root) {
		bool retval = true;
		// Now dump all the children using the new path
		m_root->foreach_child([&dest_path, &retval](const vp_node* child) {
			retval &= child->dump(dest_path);
		});

		return retval;
	}
	return false;
}

////////////////////////////////////////////////////////////////
/// vp_node methods
std::string vp_node::get_path() const
{
  std::list<std::string> elements;
  const vp_node* curr = this;
  while (curr) {
    elements.push_front(curr->get_name());
    curr = curr->get_parent();
  }

  std::stringstream path_str;
  for (auto elem : elements) {
    path_str << elem;
    if (elem != elements.back()) {
       path_str << '/';
    }
  }
  return path_str.str();
}

////////////////////////////////////////////////////////////////
/// vp_directory methods

vp_directory::vp_directory(const std::string& name, vp_directory* parent)
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

void vp_directory::foreach_child(std::function<void(const vp_node*)> f) const
{
  for (auto* child : m_children) {
    f(child);
  }
}

bool vp_directory::dump(const std::string& dest_path) const
{
	// First, create a directory for this node
	std::filesystem::path p(dest_path);
	p.append(m_name);

	std::error_code err;
	if (!std::filesystem::create_directories(p, err) && err.value() != 0) {
		std::cerr << "Failed to create directory " << p << ": " << err << std::endl;
		return false;
	}

	bool retval = true;
	// Now dump all the children using the new path
	foreach_child([&p, &retval](const vp_node* child) {
		retval &= child->dump(p.string());
	});

	return retval;
}

///////////////////////////////////////////////////////////////////
/// vp_file methods

vp_file::vp_file(const std::string& name,
                 uint32_t offset,
                 uint32_t size,
                 vp_directory* parent,
                 std::ifstream* filestream)
  : vp_node(parent),
    m_name(name),
    m_offset(offset),
    m_size(size),
    m_filestream(filestream)
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
  return;
}

void vp_file::foreach_child(std::function<void(const vp_node*)> f) const
{
  // vp_file has no children
  return;
}

std::string vp_file::dump() const
{
  m_filestream->seekg(m_offset);
  if (!*m_filestream) {
    std::cerr << "Could not seek to offset " << m_offset << std::endl;
  }

	// Read from the correct offset
  std::string retval;
  retval.resize(m_size);
  m_filestream->read(&retval[0], m_size);
  if (!*m_filestream) {
    std::cerr << "Could not read " << m_size << " bytes from file\n";
  }

  return retval;
}

bool vp_file::dump(const std::string& path) const
{
	std::filesystem::path dump_file(path);

	// If we're given a directory, just create the file in the directory
	if (std::filesystem::is_directory(dump_file)) {
		dump_file.append(m_name);
	}

  // This is not the most efficient implementation, but it should be fine
  std::string dump_buf = dump();
  if (dump_buf.empty()) {
    return false;
	}

	// Write the buffer to the file
  std::ofstream outfile(dump_file, std::ios::out | std::ios::binary);
  outfile << dump_buf;
  bool retval = (bool)outfile;
  outfile.close();

  return retval;
}
