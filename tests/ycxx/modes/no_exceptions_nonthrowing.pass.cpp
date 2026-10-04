// Built with -fno-exceptions. The draft does not describe a program without exceptions (its
// behavior when the library would throw is implementation-defined, see
// modes/no_exceptions_terminates.pass.cpp), but every operation that does not throw keeps its
// specified effects. Checked here, each with its paragraph:
//   [any.class], [any.modifiers], [any.nonmembers]/9-10 (pointer any_cast: no exception)
//   [any.observers]/3: type() returns typeid(T) of the contained value, typeid(void) when empty
//   [func.wrap.func.targ]/1-3: target_type() is typeid(F); target<T>() points at the target iff
//     target_type() == typeid(T)
//   [type.index.members]: type_index compares and hashes as its type_info
//   [util.smartptr.shared.cast]/9-11: dynamic_pointer_cast; empty result if the cast fails
//   [util.smartptr.getdeleter]/1: get_deleter
//   [propagation]/7: current_exception() outside a handler is null; /12 make_exception_ptr(e)
//     "refers to a copy of e"; /14 exception_ptr_cast observes it (no throw involved)
//   [variant.status]/1: valueless_by_exception() is false unless an exception was thrown;
//     [variant.get] get_if; [variant.visit]
//   [sequence.reqmts]/120-121 (at() in range), [array.overview], [string.access]
//   [optional.observe] value() on an engaged optional, value_or
//   [expected.object.obs] value() with a value
//   [format.functions] format with a valid format string, [charconv]
#include <any>
#include <array>
#include <charconv>
#include <deque>
#include <exception>
#include <expected>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>
#include "check.hpp"

// FLAGS: -fno-exceptions

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {
  int v = 7;
};
struct Other : Base {};

struct Fn {
  int k;
  int operator()(int x) const { return x + k; }
};
int twice(int x) { return 2 * x; }

struct Deleter {
  int* counter;
  void operator()(int* p) const {
    ++*counter;
    delete p;
  }
};

int main() {
  // any
  {
    std::any a;
    CHECK(!a.has_value());
    CHECK(a.type() == typeid(void));
    a = 5;
    CHECK(a.has_value());
    CHECK(a.type() == typeid(int));
    CHECK(std::any_cast<int>(&a) != nullptr && *std::any_cast<int>(&a) == 5);
    CHECK(std::any_cast<long>(&a) == nullptr);
    CHECK(std::any_cast<int>(a) == 5);  // the value form, with the right type, does not throw
    a.emplace<std::string>(100, 'x');  // larger than any small buffer
    CHECK(a.type() == typeid(std::string));
    CHECK(std::any_cast<std::string&>(a).size() == 100);
    std::any b = std::make_any<std::vector<int>>({1, 2, 3});
    a.swap(b);
    CHECK(std::any_cast<std::vector<int>>(&a)->size() == 3);
    CHECK(std::any_cast<std::string>(&b)->size() == 100);
    b.reset();
    CHECK(!b.has_value() && b.type() == typeid(void));
  }
  // function::target / target_type
  {
    std::function<int(int)> f = Fn{3};
    CHECK(f.target_type() == typeid(Fn));
    CHECK(f.target<Fn>() != nullptr && f.target<Fn>()->k == 3);
    CHECK(f.target<int (*)(int)>() == nullptr);
    CHECK(f(1) == 4);
    f = twice;
    CHECK(f.target_type() == typeid(int (*)(int)));
    CHECK(f.target<int (*)(int)>() != nullptr && *f.target<int (*)(int)>() == &twice);
    CHECK(f.target<Fn>() == nullptr);
    const std::function<int(int)> e = nullptr;  // see function/const_default_init
    CHECK(e.target_type() == typeid(void));
    CHECK(e.target<Fn>() == nullptr);
  }
  // type_index
  {
    std::type_index i(typeid(int)), j(typeid(long)), k(typeid(int));
    CHECK(i == k && i != j);
    CHECK((i < j) == typeid(int).before(typeid(long)));
    CHECK(i.hash_code() == typeid(int).hash_code());
    CHECK(std::hash<std::type_index>()(i) == typeid(int).hash_code());
    std::unordered_map<std::type_index, int> m{{typeid(int), 1}, {typeid(double), 2}};
    CHECK(m.at(typeid(double)) == 2);
  }
  // dynamic_pointer_cast, get_deleter
  {
    std::shared_ptr<Base> b = std::make_shared<Derived>();
    auto d = std::dynamic_pointer_cast<Derived>(b);
    CHECK(d && d->v == 7 && d.use_count() == 2);
    auto o = std::dynamic_pointer_cast<Other>(b);
    CHECK(!o && o.use_count() == 0);
    int n = 0;
    {
      std::shared_ptr<int> p(new int(4), Deleter{&n});
      Deleter* dp = std::get_deleter<Deleter>(p);
      CHECK(dp != nullptr && dp->counter == &n);
      CHECK(std::get_deleter<std::default_delete<int>>(p) == nullptr);
    }
    CHECK(n == 1);
  }
  // exception_ptr
  {
    CHECK(std::current_exception() == nullptr);
    std::exception_ptr p = std::make_exception_ptr(42);
    CHECK(p != nullptr);
    auto c = std::exception_ptr_cast<int>(p);
    CHECK(c && *c == 42);
    CHECK(!std::exception_ptr_cast<long>(p));
    std::exception_ptr q = p;
    CHECK(q == p);
    q = nullptr;
    CHECK(q == nullptr && p != nullptr);
  }
  // variant
  {
    std::variant<int, std::string> v = 3;
    CHECK(!v.valueless_by_exception() && v.index() == 0);
    v = std::string(50, 'y');
    CHECK(v.index() == 1 && !v.valueless_by_exception());
    CHECK(std::get_if<int>(&v) == nullptr);
    CHECK(std::get<std::string>(v).size() == 50);
    CHECK(std::visit([](const auto& x) { return sizeof(x); }, v) == sizeof(std::string));
    v.emplace<0>(9);
    CHECK(std::get<0>(v) == 9);
  }
  // in-range at(), optional/expected value()
  {
    std::vector<int> vec{1, 2, 3};
    std::deque<int> dq{4, 5};
    std::array<int, 2> arr{6, 7};
    std::string s = "abc";
    std::map<int, int> m{{1, 10}};
    CHECK(vec.at(2) == 3 && dq.at(1) == 5 && arr.at(0) == 6 && s.at(1) == 'b' && m.at(1) == 10);
    std::optional<int> o = 8;
    CHECK(o.value() == 8 && std::optional<int>().value_or(1) == 1);
    std::expected<int, int> ex = 11;
    CHECK(ex.value() == 11);
    std::expected<int, int> un = std::unexpected(2);
    CHECK(un.error() == 2 && un.value_or(0) == 0);
  }
  // format, charconv
  {
    CHECK(std::format("{:>5}|{:x}", 42, 255) == "   42|ff");
    char buf[32];
    auto r = std::to_chars(buf, buf + 32, 1.5);
    CHECK(std::string(buf, r.ptr) == "1.5");
    int x = 0;
    auto fr = std::from_chars(buf, r.ptr, x);
    CHECK(x == 1 && fr.ptr == buf + 1);
  }
  return 0;
}
