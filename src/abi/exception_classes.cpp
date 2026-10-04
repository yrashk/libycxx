// libycxx ABI runtime: key functions for the exception classes that -fno-rtti code declares with
// an out-of-line destructor (exception_base.hpp, DECISIONS §4). This file is built with RTTI and
// with YCXX_EXCEPTION_KEY_FUNCTIONS (see CMakeLists.txt), which makes the headers declare those
// destructors out of line here too; so this translation unit emits their vtables, with type_info,
// for the whole program.
#include <exception>
#include <new>
#include <typeinfo>

namespace std {

exception::~exception() {}
bad_alloc::~bad_alloc() {}
bad_array_new_length::~bad_array_new_length() {}
bad_exception::~bad_exception() {}
bad_cast::~bad_cast() {}
bad_typeid::~bad_typeid() {}

} // namespace std
