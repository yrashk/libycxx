// A small program using libycxx: exceptions through its own ABI runtime, exception_ptr,
// the function wrappers, optional/variant/expected, string_view and RTTI.
// Prints "libycxx example: ok" and returns 0 when every check holds.
#include <cstdio>
#include <exception>
#include <expected>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <typeinfo>
#include <variant>

namespace {

int failures = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    std::printf("FAILED: %s\n", what);
    ++failures;
  }
}

struct Shape {
  virtual ~Shape() = default;
  virtual double area() const = 0;
};
struct Square : Shape {
  double side;
  explicit Square(double s) : side(s) {}
  double area() const override { return side * side; }
};

std::expected<int, std::string_view> parse_digit(char c) {
  if (c < '0' || c > '9')
    return std::unexpected(std::string_view("not a digit"));
  return c - '0';
}

} // namespace

int main() {
  // Exceptions, caught by base class.
  try {
    throw std::out_of_range("index 7");
  } catch (const std::logic_error& e) {
    check(std::string_view(e.what()) == "index 7", "catch by base class");
  }

  // exception_ptr keeps an exception alive and rethrows it.
  std::exception_ptr saved;
  try {
    throw 42;
  } catch (...) {
    saved = std::current_exception();
  }
  try {
    std::rethrow_exception(saved);
  } catch (int v) {
    check(v == 42, "rethrow_exception");
  }

  // Function wrappers.
  std::function<int(int)> twice = [](int x) { return 2 * x; };
  std::move_only_function<int(int) const> add = [k = 3](int x) { return x + k; };
  check(twice(add(4)) == 14, "function / move_only_function");

  // Vocabulary types.
  std::optional<int> o = parse_digit('7').value_or(-1);
  std::variant<int, std::string_view> v = std::string_view("text");
  check(o == 7 && !parse_digit('x') && std::get<std::string_view>(v).size() == 4, "optional/variant/expected");

  // RTTI and dynamic_cast through the runtime.
  Square sq(3);
  const Shape& s = sq;
  check(typeid(s) == typeid(Square) && dynamic_cast<const Square*>(&s) != nullptr && s.area() == 9,
        "typeid / dynamic_cast");

  if (failures != 0)
    return 1;
  std::printf("libycxx example: ok\n");
  return 0;
}
