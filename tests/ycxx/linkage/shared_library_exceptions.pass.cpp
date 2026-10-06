// Exceptions crossing the boundary between a program and a shared library that both use the
// library (the shared library is linked through the same wrapper as the program, so with a
// static libycxx it carries its own copy of the library: what hidden symbol visibility has to
// keep working). The standard has no shared libraries; the requirements checked are those of a
// single program, which such a split must not break:
//   [except.handle]/3: a handler of type cv T or const T& matches an exception object of type E
//     if T and E are the same type or T is an unambiguous public base class of E;
//   [except.throw]/2, [except.handle]/7: the exception propagates through the library's frames,
//     destroying their automatic objects;
//   [except.uncaught]/1, [uncaught.exceptions]/1: uncaught_exceptions() counts the exceptions
//     thrown and not yet caught, in the whole thread;
//   [propagation]/7-10: make_exception_ptr, current_exception and rethrow_exception refer to
//     the same exception object wherever they are called;
//   [except.nested]: rethrow_if_nested rethrows the nested exception;
//   [syserr.syserr.members]: system_error::code() as constructed (compared with errc through
//     the program's own generic_category(): the category objects are a property of each copy of
//     the library, so only the error value and the condition are checked).
// FLAGS: -fPIC
// SHARED: ../support/linkage/shared_exceptions_lib.cpp
// REQUIRES: exceptions
#include <any>
#include <exception>
#include <locale>
#include <new>
#include <stdexcept>
#include <sstream>
#include <string>
#include <system_error>
#include "check.hpp"
#include "../support/linkage/shared_exceptions.hpp"

static std::string catch_here(int which) {
  try {
    lib_throw(which);
  } catch (const LibError& e) {
    return "LibError " + std::to_string(e.code) + " " + e.what();
  } catch (const std::system_error& e) {
    return std::string("system_error ") + (e.code().value() == int(std::errc::permission_denied) ? "EACCES" : "other");
  } catch (const std::out_of_range&) {
    return "out_of_range";
  } catch (const std::invalid_argument& e) {
    try {
      std::rethrow_if_nested(e);
    } catch (const std::logic_error& inner) {
      return std::string("nested ") + e.what() + " " + inner.what();
    }
    return "not nested";
  } catch (const std::bad_alloc&) {
    return "bad_alloc";
  } catch (const std::exception& e) {
    return std::string("exception ") + e.what();
  } catch (int i) {
    return "int " + std::to_string(i);
  } catch (const char* s) {
    return std::string("text ") + s;
  }
  return "none";
}

static int observed_main = -1, observed_lib = -1;
struct Observer {
  ~Observer() {
    observed_main = std::uncaught_exceptions();
    observed_lib = lib_uncaught();
  }
};

int main() {
  // Thrown in the library, caught in the program.
  CHECK(catch_here(0) == "int 7");
  CHECK(catch_here(1) == "exception runtime");
  CHECK(catch_here(2) == "system_error EACCES");
  CHECK(catch_here(3) == "out_of_range");
  CHECK(catch_here(4) == "LibError 4 lib");
  CHECK(catch_here(5) == "nested outer inner");
  CHECK(catch_here(6) == "bad_alloc");
  CHECK(catch_here(7) == "text text");
  try {
    lib_throw(1);
  } catch (const std::exception&) {
    std::exception_ptr p = std::current_exception();
    CHECK(p != nullptr);
    try {
      std::rethrow_exception(p);
    } catch (const std::runtime_error& e) {
      CHECK(std::string(e.what()) == "runtime");
    }
  }

  // Thrown in the program, caught in the library.
  CHECK(lib_catch([] { throw 9; }) == "int 9");
  CHECK(lib_catch([] { throw std::runtime_error("prog"); }) == "exception prog");
  CHECK(lib_catch([] { throw LibError("from program", 11); }) == "LibError 11");
  CHECK(lib_catch([] { throw std::system_error(std::make_error_code(std::errc::permission_denied)); }) ==
        "system_error EACCES");
  CHECK(lib_catch([] { (void)std::string("abc").at(9); }) == "out_of_range");
  CHECK(lib_catch([] {}) == "none");

  // Thrown in the program, through the library's frames, caught in the program; the copy that
  // threw counts the exception as uncaught while it propagates. (Each copy of the library counts
  // only its own exceptions: STATUS, known limitations, hidden visibility.)
  try {
    lib_call([] {
      Observer o;
      throw std::overflow_error("through");
    });
    CHECK(false);
  } catch (const std::overflow_error& e) {
    CHECK(std::string(e.what()) == "through");
    CHECK(std::uncaught_exceptions() == 0);
    CHECK(lib_uncaught() == 0);
  }
  CHECK(observed_main == 1);
  observed_main = observed_lib = -1;
  try {
    Observer o;
    lib_throw(0);
  } catch (int) {
    CHECK(lib_uncaught() == 0 && std::uncaught_exceptions() == 0);
  }
  CHECK(observed_lib == 1);

  // exception_ptr in both directions.
  std::exception_ptr p = lib_make_ptr();
  CHECK(p != nullptr);
  for (int i = 0; i < 2; ++i) {
    try {
      std::rethrow_exception(p);
    } catch (const std::domain_error& e) {
      CHECK(std::string(e.what()) == "ptr");
    }
  }
  try {
    lib_rethrow(std::make_exception_ptr(std::length_error("back")));
  } catch (const std::logic_error& e) {
    CHECK(std::string(e.what()) == "back");
  }
  try {
    lib_rethrow(p);
  } catch (const std::domain_error& e) {
    CHECK(std::string(e.what()) == "ptr");
  }
  p = nullptr;

  // Library objects crossing the boundary.
  //   [locale.facet]/6, [locale.global.templates]: the facets of a locale made in the program
  //   are found by use_facet in the library (one id per facet interface in the program);
  //   [ostream]: the library writes to the program's stream; [syserr.errcat.objects]/1:
  //   "All calls to this function shall return references to the same object", so an
  //   error_code made in the library compares equal to the program's errc condition;
  //   [any.nonmembers]/5: any_cast of the type the any holds succeeds on either side.
  struct Comma : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
  };
  std::locale comma(std::locale::classic(), new Comma);
  CHECK(lib_format(comma, 2.5) == "2,5|,");
  CHECK(lib_format(std::locale::classic(), 2.5) == "2.5|.");
  std::ostringstream os;
  lib_write(os, "written ");
  CHECK(os.str() == "written 42");
  std::error_code ec = lib_error_code();
  // The library's generic_category() is its own copy's object (STATUS, known limitations): the
  // value and the category's name are what cross.
  CHECK(std::string(ec.category().name()) == std::generic_category().name());
  CHECK(ec.value() == int(std::errc::permission_denied));
  CHECK(lib_any_int(std::any(17)) == 17);
  CHECK(lib_any_int(std::any(17L)) == -1);
  std::any a = lib_make_any();
  CHECK(a.type() == typeid(std::string));
  CHECK(std::any_cast<std::string>(&a) != nullptr && *std::any_cast<std::string>(&a) == "from the library");
  return 0;
}
