// libycxx core: <typeinfo>. Layout per the Itanium C++ ABI: a vtable pointer and the mangled
// name. type_info objects are emitted by the compiler; their vtables come from the ABI runtime.
#pragma once

#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/exception_base.hpp>

namespace ycxx::detail {
// A type_info's name pointer as the compiler stored it, made readable: Clang's Apple arm64 C++ ABI
// sets bit 63 for a type_info that may exist in several linked images (cfg::rtti_non_unique_bit;
// such type_infos must compare by name, which libycxx does for every name not marked '*').
inline const char* rtti_name(const char* stored) noexcept {
  if constexpr (cfg::rtti_non_unique_bit)
    return reinterpret_cast<const char*>(reinterpret_cast<__UINTPTR_TYPE__>(stored) &
                                         ~(static_cast<__UINTPTR_TYPE__>(1) << 63));
  return stored;
}
} // namespace ycxx::detail

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
      const char* a = stored_name();
      const char* b = rhs.stored_name();
      return this == &rhs || (a[0] != '*' && b[0] != '*' && __builtin_strcmp(a, b) == 0);
    }
  }
  bool before(const type_info& rhs) const noexcept {
    const char* a = raw_name();
    const char* b = rhs.raw_name();
    if (stored_name()[0] == '*' || rhs.stored_name()[0] == '*')
      return a < b;
    return __builtin_strcmp(a, b) < 0;
  }
  size_t hash_code() const noexcept {
    // FNV-1a over the name, consistent with operator==.
    size_t h = static_cast<size_t>(14695981039346656037ULL);
    for (const char* p = raw_name(); *p; ++p)
      h = (h ^ static_cast<unsigned char>(*p)) * static_cast<size_t>(1099511628211ULL);
    return stored_name()[0] == '*' ? reinterpret_cast<size_t>(this) : h;
  }
  const char* name() const noexcept { return raw_name(); }

  type_info(const type_info&) = delete;
  type_info& operator=(const type_info&) = delete;

protected:
  const char* name_;
  explicit type_info(const char* n) noexcept : name_(n) {}

private:
  const char* stored_name() const noexcept { return ycxx::detail::rtti_name(name_); }
  const char* raw_name() const noexcept {
    const char* n = stored_name();
    return n[0] == '*' ? n + 1 : n;
  }
};

class bad_cast : public exception {
public:
  constexpr bad_cast() noexcept {}
  constexpr bad_cast(const bad_cast&) noexcept = default;
  constexpr bad_cast& operator=(const bad_cast&) noexcept = default;
#if !YCXX_EXCEPTION_DTOR_OUT_OF_LINE
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
#if !YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~bad_typeid() override {}
#else
  ~bad_typeid() override; // see the header comment of exception_base.hpp
#endif
  constexpr const char* what() const noexcept override { return "std::bad_typeid"; }
};

} // namespace std

namespace ycxx::detail {

// &typeid(T), or nullptr without RTTI. typeid cannot even be parsed under -fno-rtti (not in a
// discarded branch, not in an uninstantiated template), so this is the one place that spells
// it; users gate on cfg::rtti in-language (DECISIONS §1 rule 4).
#if YCXX_HAS_RTTI
template <class T>
inline constexpr const std::type_info* type_id = &typeid(T);
#else
template <class T>
inline constexpr const std::type_info* type_id = nullptr;
#endif

} // namespace ycxx::detail
