// libycxx core: the <stdexcept> classes ([std.exceptions]), constexpr (P3068, P3378).
//
// Each class stores its message in one pointer, `text_`, to an immutable NTBS (shared_message).
// Copying never throws ([exception]/2), so the storage depends on when the object lives:
//   - At run time the text sits in a reference-counted heap block owned by the hosted runtime
//     (src/hosted/stdexcept.cpp: message_create/_retain/_release); a copy adds a reference.
//   - During constant evaluation the text is a plain new[]-allocated array, and a copy
//     duplicates it (an allocation that fails there is not a constant expression, so noexcept
//     holds). Such an allocation cannot outlive the evaluation, so the two forms never meet.
// This header needs only <exception>'s base class, so ycxx/core/error.hpp can include it and
// throw these classes during constant evaluation. The constructors taking `const string&` are
// only declared here; they are defined, inline and constexpr, after basic_string
// (ycxx/core/basic_string.hpp), which every caller has included to have a string at all.
//
// Everything is inline and constexpr, so these classes have no key function; their vtables and
// type_info are emitted where needed, as for the classes in exception_base.hpp. In hosted builds
// without RTTI (YCXX_EXCEPTION_DTOR_OUT_OF_LINE) the destructors are declared out of line instead
// and defined in the hosted runtime, built with RTTI, which throws these classes itself
// (DECISIONS §4); there they are not constexpr-destructible.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>

namespace [[gnu::visibility("hidden")]] std {
template <class CharT>
struct char_traits;
template <class T>
class allocator;
template <class CharT, class Traits, class Alloc>
class basic_string;
using string = basic_string<char, char_traits<char>, allocator<char>>;
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// The run-time representation, defined in the hosted runtime. message_create returns the text
// of a new block holding a copy of [s, s + n) and a terminating null, with one reference.
const char* message_create(const char* s, std::size_t n);
void message_retain(const char* text) noexcept;
void message_release(const char* text) noexcept;

// Shared, immutable message storage for the <stdexcept> classes.
class shared_message {
  const char* text_;

  // Constant evaluation only: a copy of at most n characters of s, up to its first null (all
  // that what() can show), plus a null.
  static constexpr const char* clone(const char* s, std::size_t n) {
    std::size_t len = 0;
    while (len != n && s[len] != '\0')
      ++len;
    char* p = new char[len + 1];
    for (std::size_t i = 0; i != len; ++i)
      p[i] = s[i];
    p[len] = '\0';
    return p;
  }

public:
  constexpr explicit shared_message(const char* s) : shared_message(s, __builtin_strlen(s)) {}
  constexpr shared_message(const char* s, std::size_t n) : text_(nullptr) {
    if consteval {
      text_ = clone(s, n);
    } else {
      text_ = ::ycxx::detail::message_create(s, n);
    }
  }
  constexpr shared_message(const shared_message& o) noexcept : text_(o.text_) {
    if consteval {
      text_ = clone(o.text_, static_cast<std::size_t>(-1));
    } else {
      ::ycxx::detail::message_retain(text_);
    }
  }
  constexpr shared_message& operator=(const shared_message& o) noexcept {
    if (text_ != o.text_) {
      shared_message tmp(o);
      const char* t = tmp.text_;
      tmp.text_ = text_;
      text_ = t;
    }
    return *this;
  }
  constexpr ~shared_message() {
    if consteval {
      delete[] text_;
    } else {
      ::ycxx::detail::message_release(text_);
    }
  }
  constexpr const char* c_str() const noexcept { return text_; }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

class logic_error : public exception {
  ycxx::detail::shared_message msg_;

public:
  constexpr explicit logic_error(const string& what_arg); // defined in basic_string.hpp
  constexpr explicit logic_error(const char* what_arg) : msg_(what_arg) {}
  constexpr logic_error(const logic_error&) noexcept = default;
  constexpr logic_error& operator=(const logic_error&) noexcept = default;
#if !YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~logic_error() override {}
#else
  ~logic_error() override; // see the header comment
#endif
  constexpr const char* what() const noexcept override { return msg_.c_str(); }
};

class runtime_error : public exception {
  ycxx::detail::shared_message msg_;

public:
  constexpr explicit runtime_error(const string& what_arg); // defined in basic_string.hpp
  constexpr explicit runtime_error(const char* what_arg) : msg_(what_arg) {}
  constexpr runtime_error(const runtime_error&) noexcept = default;
  constexpr runtime_error& operator=(const runtime_error&) noexcept = default;
#if !YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~runtime_error() override {}
#else
  ~runtime_error() override; // see the header comment
#endif
  constexpr const char* what() const noexcept override { return msg_.c_str(); }
};

// The derived classes declare a destructor only where it must be out of line; otherwise the
// implicit one is constexpr. Their `const string&` constructors are defined in basic_string.hpp.
class domain_error : public logic_error {
public:
  constexpr explicit domain_error(const string& what_arg);
  constexpr explicit domain_error(const char* what_arg) : logic_error(what_arg) {}
#if YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  domain_error(const domain_error&) noexcept = default;
  domain_error& operator=(const domain_error&) noexcept = default;
  ~domain_error() override;
#endif
};
class invalid_argument : public logic_error {
public:
  constexpr explicit invalid_argument(const string& what_arg);
  constexpr explicit invalid_argument(const char* what_arg) : logic_error(what_arg) {}
#if YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  invalid_argument(const invalid_argument&) noexcept = default;
  invalid_argument& operator=(const invalid_argument&) noexcept = default;
  ~invalid_argument() override;
#endif
};
class length_error : public logic_error {
public:
  constexpr explicit length_error(const string& what_arg);
  constexpr explicit length_error(const char* what_arg) : logic_error(what_arg) {}
#if YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  length_error(const length_error&) noexcept = default;
  length_error& operator=(const length_error&) noexcept = default;
  ~length_error() override;
#endif
};
class out_of_range : public logic_error {
public:
  constexpr explicit out_of_range(const string& what_arg);
  constexpr explicit out_of_range(const char* what_arg) : logic_error(what_arg) {}
#if YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  out_of_range(const out_of_range&) noexcept = default;
  out_of_range& operator=(const out_of_range&) noexcept = default;
  ~out_of_range() override;
#endif
};
class range_error : public runtime_error {
public:
  constexpr explicit range_error(const string& what_arg);
  constexpr explicit range_error(const char* what_arg) : runtime_error(what_arg) {}
#if YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  range_error(const range_error&) noexcept = default;
  range_error& operator=(const range_error&) noexcept = default;
  ~range_error() override;
#endif
};
class overflow_error : public runtime_error {
public:
  constexpr explicit overflow_error(const string& what_arg);
  constexpr explicit overflow_error(const char* what_arg) : runtime_error(what_arg) {}
#if YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  overflow_error(const overflow_error&) noexcept = default;
  overflow_error& operator=(const overflow_error&) noexcept = default;
  ~overflow_error() override;
#endif
};
class underflow_error : public runtime_error {
public:
  constexpr explicit underflow_error(const string& what_arg);
  constexpr explicit underflow_error(const char* what_arg) : runtime_error(what_arg) {}
#if YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  underflow_error(const underflow_error&) noexcept = default;
  underflow_error& operator=(const underflow_error&) noexcept = default;
  ~underflow_error() override;
#endif
};

} // namespace std
