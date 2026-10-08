// libycxx core: the <stdexcept> classes ([std.exceptions]), constexpr (P3068, P3378).
//
// Each class stores its message in one pointer, `__text_`, to an immutable NTBS (shared_message).
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
// without RTTI (_YCXX_EXCEPTION_DTOR_OUT_OF_LINE) the destructors are declared out of line instead
// and defined in the hosted runtime, built with RTTI, which throws these classes itself
// (DECISIONS §4); there they are not constexpr-destructible.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _CharT>
struct char_traits;
template <class _Tp>
class allocator;
template <class _CharT, class _Traits, class _Alloc>
class basic_string;
using string = basic_string<char, char_traits<char>, allocator<char>>;
}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// The run-time representation, defined in the hosted runtime. message_create returns the text
// of a new block holding a copy of [s, s + n) and a terminating null, with one reference.
const char* __message_create(const char* s, std::size_t n);
void __message_retain(const char* __text) noexcept;
void __message_release(const char* __text) noexcept;

// Shared, immutable message storage for the <stdexcept> classes.
class __shared_message {
  const char* __text_;

  // Constant evaluation only: a copy of at most n characters of s, up to its first null (all
  // that what() can show), plus a null.
  static constexpr const char* __clone(const char* s, std::size_t n) {
    std::size_t __len = 0;
    while (__len != n && s[__len] != '\0')
      ++__len;
    char* p = new char[__len + 1];
    for (std::size_t i = 0; i != __len; ++i)
      p[i] = s[i];
    p[__len] = '\0';
    return p;
  }

public:
  constexpr explicit __shared_message(const char* s) : __shared_message(s, __builtin_strlen(s)) {}
  constexpr __shared_message(const char* s, std::size_t n) : __text_(nullptr) {
    if consteval {
      __text_ = __clone(s, n);
    } else {
      __text_ = ::__ycxx::__detail::__message_create(s, n);
    }
  }
  constexpr __shared_message(const __shared_message& __o) noexcept : __text_(__o.__text_) {
    if consteval {
      __text_ = __clone(__o.__text_, static_cast<std::size_t>(-1));
    } else {
      ::__ycxx::__detail::__message_retain(__text_);
    }
  }
  constexpr __shared_message& operator=(const __shared_message& __o) noexcept {
    if (__text_ != __o.__text_) {
      __shared_message __tmp(__o);
      const char* t = __tmp.__text_;
      __tmp.__text_ = __text_;
      __text_ = t;
    }
    return *this;
  }
  constexpr ~__shared_message() {
    if consteval {
      delete[] __text_;
    } else {
      ::__ycxx::__detail::__message_release(__text_);
    }
  }
  constexpr const char* c_str() const noexcept { return __text_; }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class logic_error : public exception {
  __ycxx::__detail::__shared_message __msg_;

public:
  constexpr explicit logic_error(const string& __what_arg); // defined in basic_string.hpp
  constexpr explicit logic_error(const char* __what_arg) : __msg_(__what_arg) {}
  constexpr logic_error(const logic_error&) noexcept = default;
  constexpr logic_error& operator=(const logic_error&) noexcept = default;
#if !_YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~logic_error() override {}
#else
  ~logic_error() override; // see the header comment
#endif
  constexpr const char* what() const noexcept override { return __msg_.c_str(); }
};

class runtime_error : public exception {
  __ycxx::__detail::__shared_message __msg_;

public:
  constexpr explicit runtime_error(const string& __what_arg); // defined in basic_string.hpp
  constexpr explicit runtime_error(const char* __what_arg) : __msg_(__what_arg) {}
  constexpr runtime_error(const runtime_error&) noexcept = default;
  constexpr runtime_error& operator=(const runtime_error&) noexcept = default;
#if !_YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~runtime_error() override {}
#else
  ~runtime_error() override; // see the header comment
#endif
  constexpr const char* what() const noexcept override { return __msg_.c_str(); }
};

// The derived classes declare a destructor only where it must be out of line; otherwise the
// implicit one is constexpr. Their `const string&` constructors are defined in basic_string.hpp.
class domain_error : public logic_error {
public:
  constexpr explicit domain_error(const string& __what_arg);
  constexpr explicit domain_error(const char* __what_arg) : logic_error(__what_arg) {}
#if _YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  domain_error(const domain_error&) noexcept = default;
  domain_error& operator=(const domain_error&) noexcept = default;
  ~domain_error() override;
#endif
};
class invalid_argument : public logic_error {
public:
  constexpr explicit invalid_argument(const string& __what_arg);
  constexpr explicit invalid_argument(const char* __what_arg) : logic_error(__what_arg) {}
#if _YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  invalid_argument(const invalid_argument&) noexcept = default;
  invalid_argument& operator=(const invalid_argument&) noexcept = default;
  ~invalid_argument() override;
#endif
};
class length_error : public logic_error {
public:
  constexpr explicit length_error(const string& __what_arg);
  constexpr explicit length_error(const char* __what_arg) : logic_error(__what_arg) {}
#if _YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  length_error(const length_error&) noexcept = default;
  length_error& operator=(const length_error&) noexcept = default;
  ~length_error() override;
#endif
};
class out_of_range : public logic_error {
public:
  constexpr explicit out_of_range(const string& __what_arg);
  constexpr explicit out_of_range(const char* __what_arg) : logic_error(__what_arg) {}
#if _YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  out_of_range(const out_of_range&) noexcept = default;
  out_of_range& operator=(const out_of_range&) noexcept = default;
  ~out_of_range() override;
#endif
};
class range_error : public runtime_error {
public:
  constexpr explicit range_error(const string& __what_arg);
  constexpr explicit range_error(const char* __what_arg) : runtime_error(__what_arg) {}
#if _YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  range_error(const range_error&) noexcept = default;
  range_error& operator=(const range_error&) noexcept = default;
  ~range_error() override;
#endif
};
class overflow_error : public runtime_error {
public:
  constexpr explicit overflow_error(const string& __what_arg);
  constexpr explicit overflow_error(const char* __what_arg) : runtime_error(__what_arg) {}
#if _YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  overflow_error(const overflow_error&) noexcept = default;
  overflow_error& operator=(const overflow_error&) noexcept = default;
  ~overflow_error() override;
#endif
};
class underflow_error : public runtime_error {
public:
  constexpr explicit underflow_error(const string& __what_arg);
  constexpr explicit underflow_error(const char* __what_arg) : runtime_error(__what_arg) {}
#if _YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  underflow_error(const underflow_error&) noexcept = default;
  underflow_error& operator=(const underflow_error&) noexcept = default;
  ~underflow_error() override;
#endif
};

}} // namespace std
