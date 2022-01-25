#pragma once
#include <string>
#include <list>
#include <cstdint>
#include <functional>

class vp_file;
class vp_directory;

class vp_node
{
public:
  vp_node(vp_directory* parent): m_parent(parent) {}
  virtual ~vp_node() {}

  virtual const std::string& get_name() const = 0;
  virtual vp_file* find(const std::string& name) = 0;
  virtual std::string to_string() const = 0;
  virtual vp_directory* get_parent() const { return m_parent; }
  virtual void foreach_child(std::function<void(vp_node*)> f) = 0;

protected:
  vp_directory* m_parent = nullptr;
};

class vp_directory : public vp_node
{
public:
  vp_directory(const std::string&& name, vp_directory* parent);
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

class vp_file : public vp_node
{
public:
  vp_file(const std::string&& name, uint32_t offset, uint32_t size, vp_directory* parent);
  virtual const std::string& get_name() const;
  virtual vp_file* find(const std::string& name);
  virtual std::string to_string() const;
  virtual void foreach_child(std::function<void(vp_node*)> f);

  std::string dump() const;
  bool dump(const std::string&& path) const;

private:
  std::string m_name;
  uint32_t m_offset;
  uint32_t m_size;

  friend class vp_index;
};

class vp_index
{
public:
  ~vp_index();

  bool parse(const std::string&& path);
  vp_file* find(const std::string&& name) const;
  std::string print_index_listing() const;
private:
  std::string m_filename;
  vp_directory* m_root;
};
