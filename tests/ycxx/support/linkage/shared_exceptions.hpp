// Interface between linkage/shared_library_exceptions.pass.cpp and the shared library built from
// shared_exceptions_lib.cpp.
#pragma once
#include <exception>
#include <stdexcept>
#include <string>

// A user exception type whose key function (and so its vtable and type_info) is defined in the
// shared library only.
struct __attribute__((visibility("default"))) LibError : std::runtime_error {
  int code;
  LibError(const std::string& w, int c);
  ~LibError() override;
};

// Throws an exception chosen by which (see the cases in the test).
__attribute__((visibility("default"))) void lib_throw(int which);
// Calls f and returns a description of what escaped it ("none" if nothing did).
__attribute__((visibility("default"))) std::string lib_catch(void (*f)());
// Calls f inside the library; whatever f throws passes through the library's frames.
__attribute__((visibility("default"))) void lib_call(void (*f)());
// An exception_ptr made inside the library.
__attribute__((visibility("default"))) std::exception_ptr lib_make_ptr();
// Rethrows p inside the library.
__attribute__((visibility("default"))) void lib_rethrow(std::exception_ptr p);
// std::uncaught_exceptions() as the library sees it.
__attribute__((visibility("default"))) int lib_uncaught();
