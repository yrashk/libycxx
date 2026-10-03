// libycxx: <exception> declarations. terminate/uncaught_exceptions come from the ABI runtime.
#pragma once

#include <ycxx/core/exception_base.hpp>

namespace std {

class bad_exception : public exception {
public:
  bad_exception() noexcept {}
  bad_exception(const bad_exception&) noexcept = default;
  bad_exception& operator=(const bad_exception&) noexcept = default;
  ~bad_exception() noexcept override;
  const char* what() const noexcept override;
};

using terminate_handler = void (*)();
terminate_handler get_terminate() noexcept;
terminate_handler set_terminate(terminate_handler f) noexcept;
[[noreturn]] void terminate() noexcept;
int uncaught_exceptions() noexcept;

} // namespace std
