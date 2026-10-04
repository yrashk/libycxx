// Built with -fno-rtti. The draft has no mode without run-time type information; this checks
// that the library facilities whose specification does not involve typeid or dynamic_cast keep
// their specified behavior in such a program (throwing and catching, which the language
// implements without user-visible RTTI, included):
//   [any.nonmembers]/6-10: any_cast (value and pointer forms; a wrong type gives nullptr or
//     bad_any_cast)
//   [func.wrap.func.targ]/3: target<T>() points at the target if it has type T, else nullptr
//   [util.smartptr.getdeleter]/1: get_deleter<D>(p): the deleter if p owns one of type D
//   [util.smartptr.shared.cast]/2-4: static_pointer_cast; [util.smartptr.enab]
//   [sequence.reqmts]/121: at() throws out_of_range ([std.exceptions]: derived from logic_error)
//   [variant.get]/8: bad_variant_access; [optional.observe]/18: bad_optional_access
//   [propagation]/7-12: current_exception in a handler, rethrow_exception, make_exception_ptr;
//     /14 exception_ptr_cast
//   [except.nested]/5-9: throw_with_nested's exception is derived from nested_exception and the
//     argument; rethrow_nested rethrows the captured exception
//   [locale.global.templates]/1-4: use_facet / has_facet ([locale.facet]/6: identified by id)
//   [syserr.syserr.members]: system_error::code()
//   [format.functions], [re.alg.search]
#include <any>
#include <exception>
#include <format>
#include <functional>
#include <locale>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <variant>
#include <vector>
#include "check.hpp"

// FLAGS: -fno-rtti

struct Fn {
  int k;
  int operator()(int x) const { return x * k; }
};
struct Deleter {
  int* n;
  void operator()(int* p) const {
    ++*n;
    delete p;
  }
};
struct Base {
  virtual ~Base() = default;
  int b = 1;
};
struct Derived : Base {
  int d = 2;
};
struct Self : std::enable_shared_from_this<Self> {};
struct MyError : std::runtime_error {
  int code;
  MyError(int c) : std::runtime_error("mine"), code(c) {}
};

int main() {
  {
    std::any a = 5;
    CHECK(std::any_cast<int>(a) == 5);
    CHECK(std::any_cast<long>(&a) == nullptr);
    CHECK(std::any_cast<int>(&a) != nullptr);
    a = std::string(64, 'q');
    CHECK(std::any_cast<std::string&>(a).size() == 64);
    CHECK(std::any_cast<int>(&a) == nullptr);
    bool caught = false;
    try {
      (void)std::any_cast<int>(a);
    } catch (const std::bad_any_cast& e) {
      caught = true;
      const std::bad_cast& bc = e;  // [any.bad.any.cast]: derived from bad_cast
      (void)bc.what();
    }
    CHECK(caught);
  }
  {
    std::function<int(int)> f = Fn{3};
    CHECK(f.target<Fn>() != nullptr && f.target<Fn>()->k == 3);
    CHECK(f.target<int (*)(int)>() == nullptr);
    CHECK(f(2) == 6);
    std::function<int(int)> g;
    CHECK(g.target<Fn>() == nullptr);
    bool caught = false;
    try {
      g(1);
    } catch (const std::bad_function_call&) {
      caught = true;
    }
    CHECK(caught);
  }
  {
    int n = 0;
    {
      std::shared_ptr<int> p(new int(1), Deleter{&n});
      CHECK(std::get_deleter<Deleter>(p) != nullptr && std::get_deleter<Deleter>(p)->n == &n);
      CHECK(std::get_deleter<std::default_delete<int>>(p) == nullptr);
      auto q = std::make_shared<int>(3);
      CHECK(std::get_deleter<Deleter>(q) == nullptr);
    }
    CHECK(n == 1);
    std::shared_ptr<Base> b = std::make_shared<Derived>();
    auto d = std::static_pointer_cast<Derived>(b);
    CHECK(d->d == 2 && d.use_count() == 2);
    auto s = std::make_shared<Self>();
    CHECK(s->shared_from_this() == s);
  }
  {
    std::vector<int> v;
    int which = 0;
    try {
      (void)v.at(0);
    } catch (const std::logic_error& e) {
      which = 1;
      try {
        throw;
      } catch (const std::out_of_range&) {
        which = 2;
      }
    }
    CHECK(which == 2);
    try {
      std::variant<int, long> var = 1;
      (void)std::get<long>(var);
    } catch (const std::bad_variant_access&) {
      which = 3;
    }
    CHECK(which == 3);
    try {
      (void)std::optional<int>().value();
    } catch (const std::exception&) {
      which = 4;
    }
    CHECK(which == 4);
  }
  {
    std::exception_ptr p;
    try {
      throw MyError(7);
    } catch (...) {
      p = std::current_exception();
    }
    CHECK(p != nullptr);
    int code = 0;
    try {
      std::rethrow_exception(p);
    } catch (const std::runtime_error& e) {
      code = static_cast<const MyError&>(e).code;
    }
    CHECK(code == 7);
    auto c = std::exception_ptr_cast<MyError>(p);
    CHECK(c && (*c).code == 7);
    auto r = std::exception_ptr_cast<std::runtime_error>(p);  // a base class: matches as a handler would
    CHECK(r);
    CHECK(!std::exception_ptr_cast<std::logic_error>(p));
    auto q = std::make_exception_ptr(41);
    int got = 0;
    try {
      std::rethrow_exception(q);
    } catch (int i) {
      got = i;
    }
    CHECK(got == 41);
  }
  {
    int inner = 0;
    try {
      try {
        throw 5;
      } catch (...) {
        std::throw_with_nested(MyError(1));
      }
    } catch (const std::nested_exception& ne) {
      CHECK(ne.nested_ptr() != nullptr);
      try {
        ne.rethrow_nested();
      } catch (int i) {
        inner = i;
      }
    }
    CHECK(inner == 5);
  }
  {
    std::locale loc = std::locale::classic();
    CHECK(std::has_facet<std::ctype<char>>(loc));
    CHECK(std::use_facet<std::ctype<char>>(loc).toupper('a') == 'A');
    CHECK(std::use_facet<std::numpunct<char>>(loc).decimal_point() == '.');
    std::ostringstream os;
    os << 12 << ' ' << 1.5;
    CHECK(os.str() == "12 1.5");
  }
  {
    std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    try {
      throw std::system_error(ec, "x");
    } catch (const std::system_error& e) {
      CHECK(e.code() == std::errc::invalid_argument);
      CHECK(e.code().category() == std::generic_category());
    }
  }
  {
    CHECK(std::format("{:*^7}", "ab") == "**ab***");
    std::smatch m;
    std::string s = "key=value";
    CHECK(std::regex_search(s, m, std::regex("(\\w+)=(\\w+)")));
    CHECK(m[2] == "value");
  }
  return 0;
}
