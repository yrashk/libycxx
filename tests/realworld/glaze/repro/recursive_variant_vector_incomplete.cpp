#include <variant>
#include <vector>
struct nest_a;
struct nest_b;
using node = std::variant<nest_a, nest_b>;
struct nest_a {
  std::vector<node> x{};
};
struct nest_b {
  std::vector<node> x{};
};
int main() {
  node n{};
  return n.index();
}
