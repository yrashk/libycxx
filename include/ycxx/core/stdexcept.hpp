// libycxx core: <stdexcept> class declarations.
//
// Each class stores its message in one pointer to an immutable, reference-counted heap block,
// so copying never throws ([exception]/2). Member definitions live in the hosted runtime
// (src/hosted/stdexcept.cpp). The constructors taking `const string&` are defined inline by
// <string> -- any TU that can call them has a std::string and so has included <string>.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>

namespace std {

template <class CharT>
struct char_traits;
template <class T>
class allocator;
template <class CharT, class Traits, class Alloc>
class basic_string;
using string = basic_string<char, char_traits<char>, allocator<char>>;

} // namespace std

namespace ycxx::detail {
// Shared, immutable message storage for the <stdexcept> classes.
class shared_message {
  const char* text_;

public:
  explicit shared_message(const char* s);
  shared_message(const char* s, std::size_t n);
  shared_message(const shared_message& o) noexcept;
  shared_message& operator=(const shared_message& o) noexcept;
  ~shared_message();
  const char* c_str() const noexcept { return text_; }
};
} // namespace ycxx::detail

namespace std {

class logic_error : public exception {
  ycxx::detail::shared_message msg_;

public:
  explicit logic_error(const string& what_arg);
  explicit logic_error(const char* what_arg);
  logic_error(const logic_error&) noexcept = default;
  logic_error& operator=(const logic_error&) noexcept = default;
  ~logic_error() noexcept override;
  const char* what() const noexcept override;
};

class runtime_error : public exception {
  ycxx::detail::shared_message msg_;

public:
  explicit runtime_error(const string& what_arg);
  explicit runtime_error(const char* what_arg);
  runtime_error(const runtime_error&) noexcept = default;
  runtime_error& operator=(const runtime_error&) noexcept = default;
  ~runtime_error() noexcept override;
  const char* what() const noexcept override;
};

class domain_error : public logic_error {
public:
  explicit domain_error(const string& what_arg);
  explicit domain_error(const char* what_arg) : logic_error(what_arg) {}
  ~domain_error() noexcept override;
};
class invalid_argument : public logic_error {
public:
  explicit invalid_argument(const string& what_arg);
  explicit invalid_argument(const char* what_arg) : logic_error(what_arg) {}
  ~invalid_argument() noexcept override;
};
class length_error : public logic_error {
public:
  explicit length_error(const string& what_arg);
  explicit length_error(const char* what_arg) : logic_error(what_arg) {}
  ~length_error() noexcept override;
};
class out_of_range : public logic_error {
public:
  explicit out_of_range(const string& what_arg);
  explicit out_of_range(const char* what_arg) : logic_error(what_arg) {}
  ~out_of_range() noexcept override;
};
class range_error : public runtime_error {
public:
  explicit range_error(const string& what_arg);
  explicit range_error(const char* what_arg) : runtime_error(what_arg) {}
  ~range_error() noexcept override;
};
class overflow_error : public runtime_error {
public:
  explicit overflow_error(const string& what_arg);
  explicit overflow_error(const char* what_arg) : runtime_error(what_arg) {}
  ~overflow_error() noexcept override;
};
class underflow_error : public runtime_error {
public:
  explicit underflow_error(const string& what_arg);
  explicit underflow_error(const char* what_arg) : runtime_error(what_arg) {}
  ~underflow_error() noexcept override;
};

} // namespace std
