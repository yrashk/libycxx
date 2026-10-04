// libycxx core: error_category, error_code, error_condition and system_error ([syserr]).
//
// The value types and the category base are core: they need only <string>. Everything that
// needs a single definition or the C library is in the hosted runtime
// (src/hosted/system_error.cpp, DECISIONS §3): generic_category() and system_category() (objects
// that are constant-initialized and never destroyed, so they stay usable during static
// destruction), their messages (through the PAL's ycxx_pal_error_message), the destructors of
// error_category and system_error (their key functions, so their vtables and type_info are
// emitted there, with RTTI) and system_error's constructors. A freestanding program can name
// these types but gets a link error if it uses those parts.
//
// The hooks other headers need: <future> and <ios> specialize is_error_code_enum for future_errc
// and io_errc and declare make_error_code/make_error_condition overloads, which the converting
// constructors and assignments find by argument-dependent lookup only ([contents]/3).
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/errc.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/stdexcept.hpp>
#include <ycxx/core/type_traits.hpp>

namespace std {

class error_category;
class error_code;
class error_condition;
class system_error;

const error_category& generic_category() noexcept;
const error_category& system_category() noexcept;

template <class T>
struct is_error_code_enum : false_type {};
template <class T>
struct is_error_condition_enum : false_type {};
template <>
struct is_error_condition_enum<errc> : true_type {};
template <class T>
constexpr bool is_error_code_enum_v = is_error_code_enum<T>::value;
template <class T>
constexpr bool is_error_condition_enum_v = is_error_condition_enum<T>::value;

} // namespace std

namespace ycxx::detail::syserr_adl {
// [contents]/3: make_error_code and make_error_condition are found by argument-dependent lookup
// only. These zero-argument declarations hide every outer declaration from ordinary lookup and
// are never viable themselves.
void make_error_code() = delete;
void make_error_condition() = delete;

template <class E>
constexpr auto code_of(E e) -> decltype(make_error_code(e)) {
  return make_error_code(e);
}
template <class E>
constexpr auto condition_of(E e) -> decltype(make_error_condition(e)) {
  return make_error_condition(e);
}
} // namespace ycxx::detail::syserr_adl

namespace std {

class error_category {
public:
  constexpr error_category() noexcept {}
  virtual ~error_category(); // the key function, in the hosted runtime
  error_category(const error_category&) = delete;
  error_category& operator=(const error_category&) = delete;

  virtual const char* name() const noexcept = 0;
  virtual error_condition default_error_condition(int ev) const noexcept;
  virtual bool equivalent(int code, const error_condition& condition) const noexcept;
  virtual bool equivalent(const error_code& code, int condition) const noexcept;
  virtual string message(int ev) const = 0;

  bool operator==(const error_category& rhs) const noexcept { return this == __builtin_addressof(rhs); }
  strong_ordering operator<=>(const error_category& rhs) const noexcept {
    return compare_three_way()(this, __builtin_addressof(rhs));
  }
};

class error_code {
  int val_;
  const error_category* cat_;

public:
  error_code() noexcept : val_(0), cat_(__builtin_addressof(system_category())) {}
  error_code(int val, const error_category& cat) noexcept : val_(val), cat_(__builtin_addressof(cat)) {}
  template <class ErrorCodeEnum>
    requires is_error_code_enum_v<ErrorCodeEnum>
  error_code(ErrorCodeEnum e) noexcept {
    error_code ec = ::ycxx::detail::syserr_adl::code_of(e);
    assign(ec.value(), ec.category());
  }

  void assign(int val, const error_category& cat) noexcept {
    val_ = val;
    cat_ = __builtin_addressof(cat);
  }
  template <class ErrorCodeEnum>
    requires is_error_code_enum_v<ErrorCodeEnum>
  error_code& operator=(ErrorCodeEnum e) noexcept {
    error_code ec = ::ycxx::detail::syserr_adl::code_of(e);
    assign(ec.value(), ec.category());
    return *this;
  }
  void clear() noexcept { assign(0, system_category()); }

  int value() const noexcept { return val_; }
  const error_category& category() const noexcept { return *cat_; }
  error_condition default_error_condition() const noexcept;
  string message() const { return cat_->message(val_); }
  explicit operator bool() const noexcept { return val_ != 0; }
};

class error_condition {
  int val_;
  const error_category* cat_;

public:
  error_condition() noexcept : val_(0), cat_(__builtin_addressof(generic_category())) {}
  error_condition(int val, const error_category& cat) noexcept : val_(val), cat_(__builtin_addressof(cat)) {}
  template <class ErrorConditionEnum>
    requires is_error_condition_enum_v<ErrorConditionEnum>
  error_condition(ErrorConditionEnum e) noexcept {
    error_condition ec = ::ycxx::detail::syserr_adl::condition_of(e);
    assign(ec.value(), ec.category());
  }

  void assign(int val, const error_category& cat) noexcept {
    val_ = val;
    cat_ = __builtin_addressof(cat);
  }
  template <class ErrorConditionEnum>
    requires is_error_condition_enum_v<ErrorConditionEnum>
  error_condition& operator=(ErrorConditionEnum e) noexcept {
    error_condition ec = ::ycxx::detail::syserr_adl::condition_of(e);
    assign(ec.value(), ec.category());
    return *this;
  }
  void clear() noexcept { assign(0, generic_category()); }

  int value() const noexcept { return val_; }
  const error_category& category() const noexcept { return *cat_; }
  string message() const { return cat_->message(val_); }
  explicit operator bool() const noexcept { return val_ != 0; }
};

inline error_code make_error_code(errc e) noexcept { return error_code(static_cast<int>(e), generic_category()); }
inline error_condition make_error_condition(errc e) noexcept {
  return error_condition(static_cast<int>(e), generic_category());
}

inline error_condition error_code::default_error_condition() const noexcept { return cat_->default_error_condition(val_); }

// [syserr.compare]
inline bool operator==(const error_code& lhs, const error_code& rhs) noexcept {
  return lhs.category() == rhs.category() && lhs.value() == rhs.value();
}
inline bool operator==(const error_code& lhs, const error_condition& rhs) noexcept {
  return lhs.category().equivalent(lhs.value(), rhs) || rhs.category().equivalent(lhs, rhs.value());
}
inline bool operator==(const error_condition& lhs, const error_condition& rhs) noexcept {
  return lhs.category() == rhs.category() && lhs.value() == rhs.value();
}
inline strong_ordering operator<=>(const error_code& lhs, const error_code& rhs) noexcept {
  if (auto c = lhs.category() <=> rhs.category(); c != 0)
    return c;
  return lhs.value() <=> rhs.value();
}
inline strong_ordering operator<=>(const error_condition& lhs, const error_condition& rhs) noexcept {
  if (auto c = lhs.category() <=> rhs.category(); c != 0)
    return c;
  return lhs.value() <=> rhs.value();
}

// [syserr.errcat.virtuals]: the defaults of the virtual members.
inline error_condition error_category::default_error_condition(int ev) const noexcept { return error_condition(ev, *this); }
inline bool error_category::equivalent(int code, const error_condition& condition) const noexcept {
  return default_error_condition(code) == condition;
}
inline bool error_category::equivalent(const error_code& code, int condition) const noexcept {
  return *this == code.category() && code.value() == condition;
}

// [syserr.hash]: the value and the category's identity.
template <>
struct hash<error_code> {
  [[nodiscard]] size_t operator()(const error_code& ec) const noexcept {
    return static_cast<size_t>(::ycxx::detail::mum(
        static_cast<uint64_t>(static_cast<unsigned>(ec.value())) ^ ::ycxx::detail::hash_k1,
        reinterpret_cast<uintptr_t>(__builtin_addressof(ec.category())) ^ ::ycxx::detail::hash_k2));
  }
};
template <>
struct hash<error_condition> {
  [[nodiscard]] size_t operator()(const error_condition& ec) const noexcept {
    return static_cast<size_t>(::ycxx::detail::mum(
        static_cast<uint64_t>(static_cast<unsigned>(ec.value())) ^ ::ycxx::detail::hash_k1,
        reinterpret_cast<uintptr_t>(__builtin_addressof(ec.category())) ^ ::ycxx::detail::hash_k2));
  }
};

// [syserr.syserr]. what() is what_arg + ": " + code().message(), or the message alone when
// what_arg is empty or absent (composed by the constructors, in the hosted runtime).
class system_error : public runtime_error {
  error_code code_;

public:
  system_error(error_code ec, const string& what_arg);
  system_error(error_code ec, const char* what_arg);
  system_error(error_code ec);
  system_error(int ev, const error_category& ecat, const string& what_arg);
  system_error(int ev, const error_category& ecat, const char* what_arg);
  system_error(int ev, const error_category& ecat);
  system_error(const system_error&) noexcept = default;
  system_error& operator=(const system_error&) noexcept = default;
  ~system_error() override; // the key function, in the hosted runtime

  const error_code& code() const noexcept { return code_; }
  const char* what() const noexcept override { return runtime_error::what(); }
};

} // namespace std
