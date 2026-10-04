// [unique.ptr.single.asgn]: operator=(unique_ptr&&) calls reset(u.release()) followed by
// get_deleter() = std::forward<D>(u.get_deleter()), returns *this, leaves u.get() ==
// nullptr (unless this == addressof(u), when u.get() is unchanged). The converting
// operator=(unique_ptr<U, E>&&) behaves likewise. operator=(nullptr_t) is reset().
// /1,/6: constrained on is_move_assignable_v<D> / convertibility and is_assignable_v<D&, E&&>.
#include <memory>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Del {
  int id = 0;
  int* log = nullptr;  // id of the deleter that deleted, last
  constexpr Del() = default;
  constexpr Del(int i, int* l) : id(i), log(l) {}
  constexpr void operator()(int* p) const {
    if (log) *log = id;
    delete p;
  }
};

struct Base {
  constexpr virtual ~Base() = default;
};
struct Derived : Base {};

constexpr bool test() {
  int log = 0;
  {
    std::unique_ptr<int, Del> a(new int(1), Del(1, &log));
    std::unique_ptr<int, Del> b(new int(2), Del(2, &log));
    int* braw = b.get();
    std::unique_ptr<int, Del>& r = (a = std::move(b));
    if (&r != &a) return false;
    if (log != 1) return false;  // a's old pointer deleted with a's old deleter
    if (a.get() != braw || b.get() != nullptr) return false;
    if (a.get_deleter().id != 2) return false;  // deleter assigned afterwards
    log = 0;
    a = std::move(a);  // self-move: no effect on the pointer
    if (a.get() != braw || log != 0) return false;
    auto& r2 = (a = nullptr);
    if (&r2 != &a || a.get() != nullptr || log != 2) return false;
  }
  {
    // Reference deleters: the referenced deleter is copy-assigned.
    Del d1(1, &log), d2(2, &log);
    std::unique_ptr<int, Del&> a(new int(1), d1);
    std::unique_ptr<int, Del&> b(new int(2), d2);
    a = std::move(b);
    if (log != 1) return false;
    if (&a.get_deleter() != &d1 || d1.id != 2) return false;
    a.reset();
  }
  {
    Derived* raw = new Derived;
    std::unique_ptr<Derived> d(raw);
    std::unique_ptr<Base> b(new Base);
    b = std::move(d);
    if (b.get() != raw || d.get() != nullptr) return false;
    std::unique_ptr<const int> ci;
    ci = std::unique_ptr<int>(new int(4));
    if (*ci != 4) return false;
  }
  return true;
}

struct NotAssignable {
  NotAssignable() = default;
  NotAssignable(NotAssignable&&) = default;
  NotAssignable& operator=(NotAssignable&&) = delete;
  void operator()(int*) const {}
};
static_assert(!std::is_move_assignable_v<std::unique_ptr<int, NotAssignable>>);
static_assert(std::is_assignable_v<std::unique_ptr<const int>&, std::unique_ptr<int>&&>);
static_assert(!std::is_assignable_v<std::unique_ptr<int>&, std::unique_ptr<const int>&&>);
static_assert(!std::is_assignable_v<std::unique_ptr<int>&, std::unique_ptr<int[]>&&>);
static_assert(!std::is_assignable_v<std::unique_ptr<int>&, std::unique_ptr<int>&>);
static_assert(std::is_nothrow_assignable_v<std::unique_ptr<const int>&, std::unique_ptr<int>&&>);
static_assert(std::is_same_v<decltype(std::declval<std::unique_ptr<int>&>() = nullptr), std::unique_ptr<int>&>);

int main() {
  CHECK(test());
  static_assert(test());
  return 0;
}
