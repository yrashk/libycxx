// [specialized.addressof]: template<class T> constexpr T* addressof(T& r) noexcept; "Returns:
// The actual address of the object or function referenced by r, even in the presence of an
// overloaded operator&." "An expression addressof(E) is a constant subexpression if E is an
// lvalue constant subexpression." [memory.syn]: "template<class T> const T* addressof(const
// T&&) = delete;"
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Evil {
  int v;
  constexpr Evil* operator&() const { return nullptr; }
};
struct DeletedAmp {
  void operator&() const = delete;
};
void func() {}

template <class T>
concept can_addressof = requires(T&& t) { std::addressof(std::forward<T>(t)); };

static_assert(can_addressof<int&>);
static_assert(!can_addressof<int>);
static_assert(!can_addressof<const int>);
static_assert(can_addressof<DeletedAmp&>);
static_assert(noexcept(std::addressof(std::declval<int&>())));
static_assert(std::is_same_v<decltype(std::addressof(std::declval<const Evil&>())), const Evil*>);
static_assert(std::is_same_v<decltype(std::addressof(func)), void (*)()>);

constexpr Evil global_evil{1};
static_assert(std::addressof(global_evil) != nullptr);
static_assert(std::addressof(global_evil)->v == 1);

constexpr bool test() {
  Evil e{3};
  if (&e != nullptr) return false;
  Evil* p = std::addressof(e);
  if (p == nullptr || p->v != 3) return false;
  int x = 0;
  if (std::addressof(x) != &x) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  DeletedAmp d;
  CHECK(std::addressof(d) != nullptr);
  CHECK(std::addressof(func) == &func);
  return 0;
}
