// The shared library of linkage/shared_library_exceptions.pass.cpp.
#include "shared_exceptions.hpp"
#include <cerrno>
#include <new>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

LibError::LibError(const std::string& w, int c) : std::runtime_error(w), code(c) {}
LibError::~LibError() = default;

void lib_throw(int which) {
  switch (which) {
    case 0: throw 7;
    case 1: throw std::runtime_error("runtime");
    case 2: throw std::system_error(std::make_error_code(std::errc::permission_denied), "ctx");
    case 3: (void)std::vector<int>(3).at(5); break;  // [sequence.reqmts]/120: out_of_range
    case 4: throw LibError("lib", 4);
    case 5:
      try {
        throw std::logic_error("inner");
      } catch (...) {
        std::throw_with_nested(std::invalid_argument("outer"));
      }
    case 6: throw std::bad_alloc();
    case 7: throw "text";
  }
}

std::string lib_catch(void (*f)()) {
  try {
    f();
  } catch (const LibError& e) {
    return "LibError " + std::to_string(e.code);
  } catch (const std::system_error& e) {
    // The value only: the category objects belong to each copy of the library (see the test).
    return std::string("system_error ") + (e.code().value() == int(std::errc::permission_denied) ? "EACCES" : "other");
  } catch (const std::out_of_range&) {
    return "out_of_range";
  } catch (const std::exception& e) {
    return std::string("exception ") + e.what();
  } catch (int i) {
    return "int " + std::to_string(i);
  } catch (...) {
    return "other";
  }
  return "none";
}

void lib_call(void (*f)()) {
  std::string guard = "an object with a destructor on the library's frame";
  f();
}

std::exception_ptr lib_make_ptr() { return std::make_exception_ptr(std::domain_error("ptr")); }

void lib_rethrow(std::exception_ptr p) { std::rethrow_exception(p); }

int lib_uncaught() { return std::uncaught_exceptions(); }

std::string lib_format(const std::locale& loc, double v) {
  std::ostringstream os;
  os.imbue(loc);
  os << v;
  const bool has = std::has_facet<std::numpunct<char>>(loc);
  return os.str() + "|" + (has ? std::use_facet<std::numpunct<char>>(loc).decimal_point() : '?');
}

void lib_write(std::ostream& os, const char* text) { os << text << 42; }

std::error_code lib_error_code() { return std::error_code(EACCES, std::generic_category()); }

int lib_any_int(const std::any& a) {
  const int* p = std::any_cast<int>(&a);
  return p ? *p : -1;
}

std::any lib_make_any() { return std::string("from the library"); }
