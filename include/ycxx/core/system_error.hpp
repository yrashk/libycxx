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
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/core/stdexcept.hpp>
#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

class error_category;
class error_code;
class error_condition;
class system_error;

const error_category& generic_category() noexcept;
const error_category& system_category() noexcept;

template <class _Tp>
struct is_error_code_enum : false_type {};
template <class _Tp>
struct is_error_condition_enum : false_type {};
template <>
struct is_error_condition_enum<errc> : true_type {};
template <class _Tp>
constexpr bool is_error_code_enum_v = is_error_code_enum<_Tp>::value;
template <class _Tp>
constexpr bool is_error_condition_enum_v = is_error_condition_enum<_Tp>::value;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__syserr_adl {
// [contents]/3: make_error_code and make_error_condition are found by argument-dependent lookup
// only. These zero-argument declarations hide every outer declaration from ordinary lookup and
// are never viable themselves.
void make_error_code() = delete;
void make_error_condition() = delete;

template <class _Ep>
constexpr auto __code_of(_Ep e) -> decltype(make_error_code(e)) {
  return make_error_code(e);
}
template <class _Ep>
constexpr auto __condition_of(_Ep e) -> decltype(make_error_condition(e)) {
  return make_error_condition(e);
}
}} // namespace __ycxx::__detail::__syserr_adl

namespace [[__gnu__::__visibility__("hidden")]] std {

class error_category {
public:
  constexpr error_category() noexcept {}
  virtual ~error_category(); // the key function, in the hosted runtime
  error_category(const error_category&) = delete;
  error_category& operator=(const error_category&) = delete;

  virtual const char* name() const noexcept = 0;
  virtual error_condition default_error_condition(int __ev) const noexcept;
  virtual bool equivalent(int code, const error_condition& __condition) const noexcept;
  virtual bool equivalent(const error_code& code, int __condition) const noexcept;
  virtual string message(int __ev) const = 0;

  bool operator==(const error_category& __rhs) const noexcept { return this == __builtin_addressof(__rhs); }
  strong_ordering operator<=>(const error_category& __rhs) const noexcept {
    return compare_three_way()(this, __builtin_addressof(__rhs));
  }
};

class error_code {
  int __val_;
  const error_category* __cat_;

public:
  error_code() noexcept : __val_(0), __cat_(__builtin_addressof(system_category())) {}
  error_code(int __val, const error_category& cat) noexcept : __val_(__val), __cat_(__builtin_addressof(cat)) {}
  template <class _ErrorCodeEnum>
    requires is_error_code_enum_v<_ErrorCodeEnum>
  error_code(_ErrorCodeEnum e) noexcept {
    error_code ec = ::__ycxx::__detail::__syserr_adl::__code_of(e);
    assign(ec.value(), ec.category());
  }

  void assign(int __val, const error_category& cat) noexcept {
    __val_ = __val;
    __cat_ = __builtin_addressof(cat);
  }
  template <class _ErrorCodeEnum>
    requires is_error_code_enum_v<_ErrorCodeEnum>
  error_code& operator=(_ErrorCodeEnum e) noexcept {
    error_code ec = ::__ycxx::__detail::__syserr_adl::__code_of(e);
    assign(ec.value(), ec.category());
    return *this;
  }
  void clear() noexcept { assign(0, system_category()); }

  int value() const noexcept { return __val_; }
  const error_category& category() const noexcept { return *__cat_; }
  error_condition default_error_condition() const noexcept;
  string message() const { return __cat_->message(__val_); }
  explicit operator bool() const noexcept { return __val_ != 0; }
};

class error_condition {
  int __val_;
  const error_category* __cat_;

public:
  error_condition() noexcept : __val_(0), __cat_(__builtin_addressof(generic_category())) {}
  error_condition(int __val, const error_category& cat) noexcept : __val_(__val), __cat_(__builtin_addressof(cat)) {}
  template <class _ErrorConditionEnum>
    requires is_error_condition_enum_v<_ErrorConditionEnum>
  error_condition(_ErrorConditionEnum e) noexcept {
    error_condition ec = ::__ycxx::__detail::__syserr_adl::__condition_of(e);
    assign(ec.value(), ec.category());
  }

  void assign(int __val, const error_category& cat) noexcept {
    __val_ = __val;
    __cat_ = __builtin_addressof(cat);
  }
  template <class _ErrorConditionEnum>
    requires is_error_condition_enum_v<_ErrorConditionEnum>
  error_condition& operator=(_ErrorConditionEnum e) noexcept {
    error_condition ec = ::__ycxx::__detail::__syserr_adl::__condition_of(e);
    assign(ec.value(), ec.category());
    return *this;
  }
  void clear() noexcept { assign(0, generic_category()); }

  int value() const noexcept { return __val_; }
  const error_category& category() const noexcept { return *__cat_; }
  string message() const { return __cat_->message(__val_); }
  explicit operator bool() const noexcept { return __val_ != 0; }
};

inline error_code make_error_code(errc e) noexcept { return error_code(static_cast<int>(e), generic_category()); }
inline error_condition make_error_condition(errc e) noexcept {
  return error_condition(static_cast<int>(e), generic_category());
}

inline error_condition error_code::default_error_condition() const noexcept { return __cat_->default_error_condition(__val_); }

// [syserr.code.nonmembers]: written against the declaration of basic_ostream; usable once
// <ostream> is included.
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const error_code& ec) {
  return __os << ec.category().name() << ':' << ec.value();
}

// [syserr.compare]
inline bool operator==(const error_code& __lhs, const error_code& __rhs) noexcept {
  return __lhs.category() == __rhs.category() && __lhs.value() == __rhs.value();
}
inline bool operator==(const error_code& __lhs, const error_condition& __rhs) noexcept {
  return __lhs.category().equivalent(__lhs.value(), __rhs) || __rhs.category().equivalent(__lhs, __rhs.value());
}
inline bool operator==(const error_condition& __lhs, const error_condition& __rhs) noexcept {
  return __lhs.category() == __rhs.category() && __lhs.value() == __rhs.value();
}
inline strong_ordering operator<=>(const error_code& __lhs, const error_code& __rhs) noexcept {
  if (auto c = __lhs.category() <=> __rhs.category(); c != 0)
    return c;
  return __lhs.value() <=> __rhs.value();
}
inline strong_ordering operator<=>(const error_condition& __lhs, const error_condition& __rhs) noexcept {
  if (auto c = __lhs.category() <=> __rhs.category(); c != 0)
    return c;
  return __lhs.value() <=> __rhs.value();
}

// [syserr.errcat.virtuals]: the defaults of the virtual members.
inline error_condition error_category::default_error_condition(int __ev) const noexcept { return error_condition(__ev, *this); }
inline bool error_category::equivalent(int code, const error_condition& __condition) const noexcept {
  return default_error_condition(code) == __condition;
}
inline bool error_category::equivalent(const error_code& code, int __condition) const noexcept {
  return *this == code.category() && code.value() == __condition;
}

// [syserr.hash]: the value and the category's identity.
template <>
struct hash<error_code> {
  [[nodiscard]] size_t operator()(const error_code& ec) const noexcept {
    return static_cast<size_t>(::__ycxx::__detail::__mum(
        static_cast<uint64_t>(static_cast<unsigned>(ec.value())) ^ ::__ycxx::__detail::__hash_k1,
        reinterpret_cast<uintptr_t>(__builtin_addressof(ec.category())) ^ ::__ycxx::__detail::__hash_k2));
  }
};
template <>
struct hash<error_condition> {
  [[nodiscard]] size_t operator()(const error_condition& ec) const noexcept {
    return static_cast<size_t>(::__ycxx::__detail::__mum(
        static_cast<uint64_t>(static_cast<unsigned>(ec.value())) ^ ::__ycxx::__detail::__hash_k1,
        reinterpret_cast<uintptr_t>(__builtin_addressof(ec.category())) ^ ::__ycxx::__detail::__hash_k2));
  }
};

// [syserr.syserr]. what() is what_arg + ": " + code().message(), or the message alone when
// what_arg is empty or absent (composed by the constructors, in the hosted runtime).
class system_error : public runtime_error {
  error_code __code_;

public:
  system_error(error_code ec, const string& __what_arg);
  system_error(error_code ec, const char* __what_arg);
  system_error(error_code ec);
  system_error(int __ev, const error_category& __ecat, const string& __what_arg);
  system_error(int __ev, const error_category& __ecat, const char* __what_arg);
  system_error(int __ev, const error_category& __ecat);
  system_error(const system_error&) noexcept = default;
  system_error& operator=(const system_error&) noexcept = default;
  ~system_error() override; // the key function, in the hosted runtime

  const error_code& code() const noexcept { return __code_; }
  const char* what() const noexcept override { return runtime_error::what(); }
};

} // namespace std
