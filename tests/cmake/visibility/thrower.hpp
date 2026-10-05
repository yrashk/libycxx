// The interface of thrower.cpp, a shared library built with libycxx.
#pragma once

struct base {
  virtual ~base() = default;
  int b = 1;
};
struct derived : base {
  int d = 2;
};

extern "C" void throw_runtime_error();
extern "C" void throw_int();
extern "C" void throw_derived();
extern "C" void throw_derived_pointer();
extern "C" int library_uncaught_exceptions();
