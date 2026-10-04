// libycxx core: the std::exception class and the language-support exception types that core
// headers name (bad_alloc, bad_array_new_length).
//
// All members are inline and constexpr (P3068, constexpr exceptions), so these classes have no
// key function: their vtables and type_info objects are emitted as COMDAT in the translation
// units that need them. The toolchain's Itanium ABI runtime (libsupc++), which throws some of
// them itself (operator new), also defines them. The linker keeps one definition of each symbol,
// and both agree on layout and behaviour (Itanium ABI, same what() strings).
#pragma once

#include <ycxx/config.hpp>

namespace std {

class exception {
public:
  constexpr exception() noexcept {}
  constexpr exception(const exception&) noexcept = default;
  constexpr exception& operator=(const exception&) noexcept = default;
  constexpr virtual ~exception() {}
  constexpr virtual const char* what() const noexcept { return "std::exception"; }
};

class bad_alloc : public exception {
public:
  constexpr bad_alloc() noexcept {}
  constexpr bad_alloc(const bad_alloc&) noexcept = default;
  constexpr bad_alloc& operator=(const bad_alloc&) noexcept = default;
  constexpr ~bad_alloc() override {}
  constexpr const char* what() const noexcept override { return "std::bad_alloc"; }
};

class bad_array_new_length : public bad_alloc {
public:
  constexpr bad_array_new_length() noexcept {}
  constexpr ~bad_array_new_length() override {}
  constexpr const char* what() const noexcept override { return "std::bad_array_new_length"; }
};

} // namespace std
