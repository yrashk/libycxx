// libycxx core: <typeinfo>. Layout per the Itanium C++ ABI: a vtable pointer and the mangled
// name. type_info objects are emitted by the compiler; their vtables come from the ABI runtime.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// A type_info's name pointer as the compiler stored it, made readable: Clang's Apple arm64 C++ ABI
// sets bit 63 for a type_info that may exist in several linked images (cfg::rtti_non_unique_bit;
// such type_infos must compare by name, which libycxx does for every name not marked '*').
inline const char* __rtti_name(const char* __stored) noexcept {
  if constexpr (__cfg::__rtti_non_unique_bit)
    return reinterpret_cast<const char*>(reinterpret_cast<__UINTPTR_TYPE__>(__stored) &
                                         ~(static_cast<__UINTPTR_TYPE__>(1) << 63));
  return __stored;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

class type_info {
public:
  virtual ~type_info();

  constexpr bool operator==(const type_info& __rhs) const noexcept {
    if consteval {
      return this == &__rhs;
    } else {
      // Itanium ABI: a name starting with '*' is unique to its object (compare addresses);
      // otherwise names are compared as strings, since the same type may have several
      // type_info objects across shared objects.
      const char* a = __stored_name();
      const char* b = __rhs.__stored_name();
      return this == &__rhs || (a[0] != '*' && b[0] != '*' && __builtin_strcmp(a, b) == 0);
    }
  }
  bool before(const type_info& __rhs) const noexcept {
    const char* a = __raw_name();
    const char* b = __rhs.__raw_name();
    if (__stored_name()[0] == '*' || __rhs.__stored_name()[0] == '*')
      return a < b;
    return __builtin_strcmp(a, b) < 0;
  }
  size_t hash_code() const noexcept {
    // FNV-1a over the name, consistent with operator==.
    size_t h = static_cast<size_t>(14695981039346656037ULL);
    for (const char* p = __raw_name(); *p; ++p)
      h = (h ^ static_cast<unsigned char>(*p)) * static_cast<size_t>(1099511628211ULL);
    return __stored_name()[0] == '*' ? reinterpret_cast<size_t>(this) : h;
  }
  const char* name() const noexcept { return __raw_name(); }

  type_info(const type_info&) = delete;
  type_info& operator=(const type_info&) = delete;

protected:
  const char* __name_;
  explicit type_info(const char* n) noexcept : __name_(n) {}

private:
  const char* __stored_name() const noexcept { return __ycxx::__detail::__rtti_name(__name_); }
  const char* __raw_name() const noexcept {
    const char* n = __stored_name();
    return n[0] == '*' ? n + 1 : n;
  }
};

class bad_cast : public exception {
public:
  constexpr bad_cast() noexcept {}
  constexpr bad_cast(const bad_cast&) noexcept = default;
  constexpr bad_cast& operator=(const bad_cast&) noexcept = default;
#if !_YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~bad_cast() override {}
#else
  ~bad_cast() override; // see the header comment of exception_base.hpp
#endif
  constexpr const char* what() const noexcept override { return "std::bad_cast"; }
};

class bad_typeid : public exception {
public:
  constexpr bad_typeid() noexcept {}
  constexpr bad_typeid(const bad_typeid&) noexcept = default;
  constexpr bad_typeid& operator=(const bad_typeid&) noexcept = default;
#if !_YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~bad_typeid() override {}
#else
  ~bad_typeid() override; // see the header comment of exception_base.hpp
#endif
  constexpr const char* what() const noexcept override { return "std::bad_typeid"; }
};

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// &typeid(T), or nullptr without RTTI. typeid cannot even be parsed under -fno-rtti (not in a
// discarded branch, not in an uninstantiated template), so this is the one place that spells
// it; users gate on cfg::rtti in-language (DECISIONS §1 rule 4).
#if _YCXX_HAS_RTTI
template <class _Tp>
inline constexpr const std::type_info* __type_id = &typeid(_Tp);
#else
template <class _Tp>
inline constexpr const std::type_info* __type_id = nullptr;
#endif

}} // namespace __ycxx::__detail
