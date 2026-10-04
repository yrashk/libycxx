// [func.wrap.move.ctor]/14 and [func.wrap.copy.ctor]/16: the in_place_type constructors give
// "a target object of type VT direct-non-list-initialized with std::forward<Args>(args)...";
// /20 and /22: the initializer_list forms use "ilist, std::forward<Args>(args)...". So rvalue
// arguments are moved (a move-only argument is accepted), lvalue arguments are copied, and
// reference parameters of the target's constructor bind to the caller's objects.
#include <functional>
#include <initializer_list>
#include <utility>
#include "check.hpp"

struct Token {
  static inline int copies = 0, moves = 0;
  int v;
  explicit Token(int x) : v(x) {}
  Token(const Token& o) : v(o.v) { ++copies; }
  Token(Token&& o) noexcept : v(o.v) {
    o.v = 0;
    ++moves;
  }
};
struct MoveOnlyArg {
  int v;
  explicit MoveOnlyArg(int x) : v(x) {}
  MoveOnlyArg(MoveOnlyArg&& o) noexcept : v(o.v) { o.v = 0; }
  MoveOnlyArg(const MoveOnlyArg&) = delete;
};
struct Target {
  int sum;
  Target(Token t, MoveOnlyArg&& m) : sum(t.v + m.v) { MoveOnlyArg sink(std::move(m)); }
  Target(std::initializer_list<int> il, Token t, int& out) : sum(t.v) {
    for (int x : il) sum += x;
    out = sum;
  }
  Target(const Target&) = default;
  int operator()() const { return sum; }
};

int main() {
  Token t(3);
  MoveOnlyArg m(4);
  Token::copies = Token::moves = 0;
  std::move_only_function<int() const> a(std::in_place_type<Target>, t, std::move(m));
  CHECK(a() == 7 && m.v == 0 && t.v == 3 && Token::copies == 1);

  Token u(5);
  Token::copies = 0;
  std::move_only_function<int() const> b(std::in_place_type<Target>, std::move(u), MoveOnlyArg(1));
  CHECK(b() == 6 && u.v == 0 && Token::copies == 0);

  int out = 0;
  std::move_only_function<int() const> c(std::in_place_type<Target>, {1, 2}, Token(10), out);
  CHECK(c() == 13 && out == 13);  // the int& parameter bound to out

  MoveOnlyArg m2(2);
  Token v(6);
  Token::copies = 0;
  std::copyable_function<int() const> d(std::in_place_type<Target>, std::move(v), std::move(m2));
  CHECK(d() == 8 && v.v == 0 && m2.v == 0 && Token::copies == 0);
  int out2 = 0;
  std::copyable_function<int() const> e(std::in_place_type<Target>, {4}, t, out2);
  CHECK(e() == 7 && out2 == 7 && t.v == 3);
  std::copyable_function<int() const> e2 = e;
  CHECK(e2() == 7);
  return 0;
}
