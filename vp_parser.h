#pragma once
#include <string>
#include <list>
#include <cstdint>
#include <functional>
#include <iostream>
#include <fstream>

class vp_file;
class vp_directory;

/**
 * Abstract base class for a single direntry in the VP file.
 */
class vp_node
{
public:
  vp_node(vp_directory* parent): m_parent(parent) {}
  virtual ~vp_node() {}

  /// Get a user-friendly printable name for the node
  virtual const std::string& get_name() const = 0;

  /// Get the path for the node
  /// The path is given using standard UNIX path represenation
  virtual std::string get_path() const;

  /// Find a file with the given name
  virtual vp_file* find(const std::string& name) = 0;

  /// Get a user-friendly string representation of the node
  /// (might not equal the value returned by get_name())
  virtual std::string to_string() const = 0;

  /// Return the enclosing directory, or nullptr if it's the root node
  virtual vp_directory* get_parent() const { return m_parent; }

  /// Call the given callback on every child node
  virtual void foreach_child(std::function<void(vp_node*)> f) = 0;

protected:
  vp_directory* m_parent = nullptr;
};

/**
 * Represents a directory direntry in the VP file.
 */
class vp_directory : public vp_node
{
public:
  vp_directory(const std::string& name, vp_directory* parent);
  ~vp_directory();

  virtual const std::string& get_name() const override;
  virtual vp_file* find(const std::string& name) override;
  virtual std::string to_string() const override;
  virtual void foreach_child(std::function<void(vp_node*)> f);

  void add_child(vp_node* child);

private:
  std::string m_name;
  std::list<vp_node*> m_children;
};

/**
 * Represents a file direntry in the VP file.
 */
class vp_file : public vp_node
{
public:
  vp_file(const std::string& name,
          uint32_t offset,
          uint32_t size,
          vp_directory* parent,
          std::ifstream* filestream);
  virtual const std::string& get_name() const;
  virtual vp_file* find(const std::string& name);
  virtual std::string to_string() const;
  virtual void foreach_child(std::function<void(vp_node*)> f);

  /// Returns a string with the text contents of the file
  std::string dump() const;

  /// Writes the text contents of the file to the given path
  bool dump(const std::string& path) const;

private:
  std::string m_name;
  uint32_t m_offset;
  uint32_t m_size;
  std::ifstream* m_filestream;
};

/**
 * Represents an entire Volition Package.
 */
class vp_index
{
public:
  ~vp_index();

  // Given the path to a .vp file, populate this vp_index with its contents
  bool parse(const std::string& path);

  // Find a file with the given name. Only files, not directories.
  vp_file* find(const std::string& name) const;

  // Human-friendly name for printing
  std::string to_string() const;

  // Prints the directory index for the package
  std::string print_index_listing() const;
private:
  std::string m_filename;
  vp_directory* m_root = nullptr;
  std::ifstream* m_filestream = nullptr;
};
