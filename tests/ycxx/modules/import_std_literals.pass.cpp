// [std.modules]/2: the inline namespaces std::literals::*_literals are exported with their
// literal operators, so the using-directives of [string.literals], [time.duration.literals],
// [complex.literals] and [string.view.literals] work after `import std;`, and std::chrono's own
// using-directive of chrono_literals ([time.syn]) still makes the operators members of chrono.
// MODULES: std
import std;
#include "module_check.hpp"

int f1() {
  using namespace std::literals;
  return ("ab"s + "c"s).size() == 3 && "xy"sv.size() == 2 && (1h + 30min).count() == 90;
}
int f2() {
  using namespace std::string_literals;
  using namespace std::chrono_literals;
  using namespace std::complex_literals;
  return "q"s == std::string("q") && 2s == std::chrono::seconds(2) && (1.0i).imag() == 1.0;
}
int f3() {
  using namespace std::chrono;
  return 5ms == milliseconds(5) && 2024y == year(2024);
}
int f4() {
  using namespace std::literals::string_view_literals;
  return "abc"sv.substr(1) == "bc";
}
int f5() {
  using std::operator""s; // both the string and the chrono overloads
  return "a"s.size() == 1 && (3s).count() == 3;
}

int main() {
  CHECK(f1() && f2() && f3() && f4() && f5());
  return 0;
}
