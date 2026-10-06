// The hosted-layers demonstration (demo.hpp). Everything here is ordinary hosted C++: the
// examples differ only in who provides the primitives underneath (host/providers.c,
// limine/providers.c), and neither provides an operating system's.
#include "demo.hpp"

#include <algorithm>
#include <any>
#include <chrono>
#include <exception>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <print>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const char* what) {
  if (!ok) {
    ++failures;
    std::println("  FAILED: {}", what);
  }
}

// An exception hierarchy of the program's own.
struct demo_error : std::runtime_error {
  int code;
  demo_error(const std::string& what, int c) : std::runtime_error(what), code(c) {}
};
struct parse_error : demo_error {
  using demo_error::demo_error;
};

int parse_digit(char c) {
  if (c < '0' || c > '9')
    throw parse_error(std::format("not a digit: '{}'", c), 42);
  return c - '0';
}

// A function-local static (a guarded initialization) and a thread_local with a destructor (the
// thread's only thread: its destructor is registered with __cxa_atexit, DECISIONS §18).
const std::string& greeting() {
  static const std::string s = std::string("hello from ") + "a function-local static";
  return s;
}
thread_local std::string tls_note = "thread_local storage works";

struct shape {
  virtual ~shape() = default;
  virtual double area() const = 0;
  virtual std::string name() const = 0;
};
struct square final : shape {
  double side;
  explicit square(double s) : side(s) {}
  double area() const override { return side * side; }
  std::string name() const override { return "square"; }
};
struct circle final : shape {
  double r;
  explicit circle(double radius) : r(radius) {}
  double area() const override { return 3.14159265358979 * r * r; }
  std::string name() const override { return "circle"; }
};

} // namespace

int hosted_layers_demo(const char* where) {
  failures = 0;
  const auto t0 = std::chrono::steady_clock::now();
  std::println("hosted-layers demo on {}: libycxx with YCXX_PAL=none", where);

  // ---- memory: containers, strings, smart pointers ----
  std::vector<int> squares;
  for (int i = 0; i < 1000; ++i)
    squares.push_back(i * i);
  check(squares.size() == 1000 && squares[999] == 998001, "vector");
  const long long sum = std::accumulate(squares.begin(), squares.end(), 0LL);
  std::println("vector: {} squares, sum {}, first five {}", squares.size(), sum,
               std::vector<int>(squares.begin(), squares.begin() + 5));
  check(sum == 332833500, "accumulate");

  std::map<std::string, int> counts;
  for (std::string w : {"layer", "memory", "console", "layer", "clock", "layer"})
    ++counts[w];
  std::println("map: {}", counts);
  check(counts["layer"] == 3 && counts.size() == 4, "map");

  std::unordered_map<int, std::string> names{{1, "one"}, {2, "two"}, {3, "three"}};
  check(names.at(2) == "two", "unordered_map");

  std::string text = "hosted";
  text += " doesn't have to mean ";
  text.append("every OS primitive");
  std::println("string: \"{}\" ({} characters)", text, text.size());
  check(text.find("OS") == 34, "string::find");

  std::vector<std::unique_ptr<shape>> shapes;
  shapes.push_back(std::make_unique<square>(2.0));
  shapes.push_back(std::make_unique<circle>(1.0));
  std::ranges::sort(shapes, {}, &shape::area);
  for (const auto& s : shapes)
    std::println("unique_ptr<shape>: {} with area {:.4f}", s->name(), s->area());
  check(shapes[0]->name() == "circle", "virtual calls and sort");

  auto shared = std::make_shared<std::vector<std::string>>(3, "x");
  std::weak_ptr<std::vector<std::string>> weak = shared;
  check(shared.use_count() == 1 && weak.lock()->size() == 3, "shared_ptr");

  std::function<int(int)> twice = [](int x) { return 2 * x; };
  std::any boxed = std::string("an any");
  std::optional<double> maybe = 2.5;
  std::variant<int, std::string> alt = std::string("a variant");
  std::println("function {}, any \"{}\", optional {}, variant \"{}\"", twice(21), std::any_cast<std::string>(boxed),
               *maybe, std::get<std::string>(alt));
  check(twice(21) == 42, "function");

  // ---- formatting ----
  std::println("format: [{:>8}] [{:<8}] [{:^8}] [{:08.3f}] [{:#x}] [{:e}] [{}]", "right", "left", "mid", 3.14159,
               255, 6.02214076e23, true);
  check(std::format("{:.2f}", 2.0 / 3.0) == "0.67", "format of a double");
  check(std::format("{:L}", 1234567) == "1234567", "format L (the classic locale)");

  // ---- exceptions ----
  try {
    parse_digit('7');
    parse_digit('x');
    check(false, "throw");
  } catch (const demo_error& e) {
    std::println("caught {} (code {}): {}", typeid(e) == typeid(parse_error) ? "parse_error" : "demo_error", e.code,
                 e.what());
    check(e.code == 42, "catch by base class");
  }
  try {
    (void)squares.at(5000);
    check(false, "vector::at");
  } catch (const std::out_of_range& e) {
    std::println("caught out_of_range: {}", e.what());
  }
  try {
    (void)std::any_cast<int>(boxed);
    check(false, "any_cast");
  } catch (const std::bad_any_cast& e) {
    std::println("caught bad_any_cast: {}", e.what());
  }
  std::exception_ptr saved;
  try {
    throw std::logic_error("kept in an exception_ptr");
  } catch (...) {
    saved = std::current_exception();
  }
  try {
    std::rethrow_exception(saved);
  } catch (const std::logic_error& e) {
    std::println("rethrown: {}", e.what());
  }
  int unwound = 0;
  try {
    struct guard {
      int& n;
      ~guard() { ++n; }
    };
    guard g1{unwound};
    {
      guard g2{unwound};
      throw 7;
    }
  } catch (int v) {
    check(v == 7 && unwound == 2, "destructors run while unwinding");
    std::println("caught int {} after {} destructors ran", v, unwound);
  }

  // ---- statics and thread_local ----
  std::println("{}; {}", greeting(), tls_note);
  check(&greeting() == &greeting(), "function-local static");

  // ---- clock ----
  const auto t1 = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
  check(elapsed.count() >= 0, "steady_clock");
  const auto since_epoch = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  std::println("clock: the demo took {} us; system_clock says {} s since 1970", elapsed.count(),
               since_epoch.count());
  check(since_epoch.count() > 1'600'000'000, "system_clock");

  if (failures == 0)
    std::println("hosted-layers demo: ok");
  else
    std::println("hosted-layers demo: {} checks FAILED", failures);
  return failures;
}
