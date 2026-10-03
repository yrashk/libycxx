// libycxx core: the std::exception class and the language-support exception types that core
// headers name (bad_alloc, bad_array_new_length).
//
// These are declarations only. Their key functions, vtables and type_info objects come from
// the toolchain's Itanium C++ ABI runtime (libsupc++), so the layouts and virtual-function
// order follow the Itanium ABI. Merely declaring them needs no runtime; using them does.
#pragma once

#include <ycxx/config.hpp>

namespace std {

class exception {
public:
  exception() noexcept {}
  exception(const exception&) noexcept = default;
  exception& operator=(const exception&) noexcept = default;
  virtual ~exception() noexcept;
  virtual const char* what() const noexcept;
};

class bad_alloc : public exception {
public:
  bad_alloc() noexcept {}
  bad_alloc(const bad_alloc&) noexcept = default;
  bad_alloc& operator=(const bad_alloc&) noexcept = default;
  ~bad_alloc() noexcept override;
  const char* what() const noexcept override;
};

class bad_array_new_length : public bad_alloc {
public:
  bad_array_new_length() noexcept {}
  ~bad_array_new_length() noexcept override;
  const char* what() const noexcept override;
};

} // namespace std
