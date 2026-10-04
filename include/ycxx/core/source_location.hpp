// libycxx core: <source_location>
#pragma once

#include <ycxx/config.hpp>

namespace std {

struct source_location {
private:
  // Name and member names are fixed by GCC and Clang: __builtin_source_location() returns a
  // `const std::source_location::__impl*` with exactly these members. (Reserved names forced
  // by the compilers' contract.)
  struct __impl {
    const char* _M_file_name;
    const char* _M_function_name;
    unsigned _M_line;
    unsigned _M_column;
  };
  const __impl* impl_ = nullptr;

public:
  static consteval source_location current(const void* p = __builtin_source_location()) noexcept {
    source_location s;
    s.impl_ = static_cast<const __impl*>(p);
    return s;
  }
  constexpr source_location() noexcept = default;

  constexpr unsigned line() const noexcept { return impl_ ? impl_->_M_line : 0u; }
  constexpr unsigned column() const noexcept { return impl_ ? impl_->_M_column : 0u; }
  constexpr const char* file_name() const noexcept { return impl_ ? impl_->_M_file_name : ""; }
  constexpr const char* function_name() const noexcept { return impl_ ? impl_->_M_function_name : ""; }
};

} // namespace std
