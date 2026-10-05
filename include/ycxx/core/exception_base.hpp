// libycxx core: the std::exception class and the language-support exception types that core
// headers name (bad_alloc, bad_array_new_length).
//
// Constexpr exceptions (P3068) need every member inline and constexpr, so these classes have
// no key function: their vtables and type_info objects are emitted (COMDAT) by each translation
// unit that needs them. libycxx's ABI runtime (src/abi), which throws some of them itself
// (dynamic_cast, typeid, array new), emits the same symbols; the linker keeps one, and all
// copies agree (Itanium layout, same what() strings).
//
// Exception: a translation unit built with -fno-rtti would emit vtables with an empty RTTI slot,
// and if one of those won the link, dynamic_cast/typeid in RTTI code would crash. So in hosted
// builds without RTTI (YCXX_EXCEPTION_DTOR_OUT_OF_LINE), the classes the runtime throws (exception,
// bad_alloc, bad_array_new_length, bad_exception, bad_cast, bad_typeid) declare their destructor
// out of line. It is then the key function, and only the runtime (src/abi/exception_classes.cpp,
// built with RTTI) emits the vtable. The cost: no constexpr destruction of these classes in
// -fno-rtti code. Other classes (bad_optional_access, ...) have no such fallback, so a
// program that mixes RTTI and -fno-rtti translation units is unsupported for them (DECISIONS §4).
#pragma once

#include <ycxx/config.hpp>

namespace [[gnu::visibility("hidden")]] std {

class exception {
public:
  constexpr exception() noexcept {}
  constexpr exception(const exception&) noexcept = default;
  constexpr exception& operator=(const exception&) noexcept = default;
#if !YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr virtual ~exception() {}
#else
  virtual ~exception(); // see the header comment
#endif
  constexpr virtual const char* what() const noexcept { return "std::exception"; }
};

class bad_alloc : public exception {
public:
  constexpr bad_alloc() noexcept {}
  constexpr bad_alloc(const bad_alloc&) noexcept = default;
  constexpr bad_alloc& operator=(const bad_alloc&) noexcept = default;
#if !YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~bad_alloc() override {}
#else
  ~bad_alloc() override; // see the header comment of exception_base.hpp
#endif
  constexpr const char* what() const noexcept override { return "std::bad_alloc"; }
};

class bad_array_new_length : public bad_alloc {
public:
  constexpr bad_array_new_length() noexcept {}
  constexpr bad_array_new_length(const bad_array_new_length&) noexcept = default;
  constexpr bad_array_new_length& operator=(const bad_array_new_length&) noexcept = default;
#if !YCXX_EXCEPTION_DTOR_OUT_OF_LINE
  constexpr ~bad_array_new_length() override {}
#else
  ~bad_array_new_length() override; // see the header comment of exception_base.hpp
#endif
  constexpr const char* what() const noexcept override { return "std::bad_array_new_length"; }
};

} // namespace std
