#ifndef TAG_HXX
#define TAG_HXX

#include <string>
#include <utility>
#include <odb/core.hxx>

#pragma db object
class Tag
{
public:
  Tag() = default;
  explicit Tag(std::string name) : name_(std::move(name)) {}

  unsigned long id() const { return id_; }

  const std::string& name() const { return name_; }
  void name(std::string v) { name_ = std::move(v); }

private:
  friend class odb::access;

  #pragma db id auto
  unsigned long id_{0};

  std::string name_;
};

#endif // TAG_HXX