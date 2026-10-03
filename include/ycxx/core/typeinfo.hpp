// libycxx core: <typeinfo>. Layout per the Itanium C++ ABI: a vtable pointer and the mangled
// name. type_info objects are emitted by the compiler; their vtables come from the ABI runtime.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>

namespace std {

class type_info {
public:
  virtual ~type_info();

  constexpr bool operator==(const type_info& rhs) const noexcept {
    if consteval {
      return this == &rhs;
    } else {
      // Itanium ABI: a name starting with '*' is unique to its object (compare addresses);
      // otherwise names are compared as strings, since the same type may have several
      // type_info objects across shared objects.
      return this == &rhs || (name_[0] != '*' && rhs.name_[0] != '*' && __builtin_strcmp(name_, rhs.name_) == 0);
    }
  }
  bool before(const type_info& rhs) const noexcept {
    const char* a = raw_name();
    const char* b = rhs.raw_name();
    if (name_[0] == '*' || rhs.name_[0] == '*')
      return a < b;
    return __builtin_strcmp(a, b) < 0;
  }
  size_t hash_code() const noexcept {
    // FNV-1a over the name, consistent with operator==.
    size_t h = static_cast<size_t>(14695981039346656037ULL);
    for (const char* p = raw_name(); *p; ++p)
      h = (h ^ static_cast<unsigned char>(*p)) * static_cast<size_t>(1099511628211ULL);
    return name_[0] == '*' ? reinterpret_cast<size_t>(this) : h;
  }
  const char* name() const noexcept { return raw_name(); }

  type_info(const type_info&) = delete;
  type_info& operator=(const type_info&) = delete;

protected:
  const char* name_;
  explicit type_info(const char* n) noexcept : name_(n) {}

private:
  const char* raw_name() const noexcept { return name_[0] == '*' ? name_ + 1 : name_; }
};

class bad_cast : public exception {
public:
  bad_cast() noexcept {}
  bad_cast(const bad_cast&) noexcept = default;
  bad_cast& operator=(const bad_cast&) noexcept = default;
  ~bad_cast() noexcept override;
  const char* what() const noexcept override;
};

class bad_typeid : public exception {
public:
  bad_typeid() noexcept {}
  bad_typeid(const bad_typeid&) noexcept = default;
  bad_typeid& operator=(const bad_typeid&) noexcept = default;
  ~bad_typeid() noexcept override;
  const char* what() const noexcept override;
};

} // namespace std
