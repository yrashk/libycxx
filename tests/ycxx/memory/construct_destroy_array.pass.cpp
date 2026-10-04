// [specialized.construct]/1-3: construct_at is constrained on !is_unbounded_array_v<T>; for
// an array type it "Mandates: sizeof...(Args) is zero" and is equivalent to
// ::new (voidify(*location)) T[1](), i.e. the elements are value-initialized.
// [specialized.destroy]/1: destroy_at on an array is destroy(begin(*location), end(*location)),
// i.e. elements are destroyed in order from first to last.
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include "check.hpp"

struct Logged {
  int id = 0;
  int* log = nullptr;
  int* pos = nullptr;
  constexpr ~Logged() { if (log) log[(*pos)++] = id; }
};
struct Counter {
  static int live;
  int v;
  Counter() : v(7) { ++live; }
  ~Counter() { --live; }
};
int Counter::live = 0;

template <class T, class... Args>
concept CanConstructAt = requires(T* p, Args&&... args) { std::construct_at(p, static_cast<Args&&>(args)...); };

static_assert(CanConstructAt<int[3]>);
static_assert(CanConstructAt<int[2][2]>);
static_assert(!CanConstructAt<int[]>);              // Constraints: is_unbounded_array_v<T> is false
static_assert(!CanConstructAt<int[][2]>);
static_assert(std::is_same_v<decltype(std::construct_at(std::declval<int(*)[3]>())), int(*)[3]>);
static_assert(!CanConstructAt<int, int*>);
static_assert(CanConstructAt<int, long>);

// destroy_at on an array destroys the elements first to last (constant-evaluable: the
// elements' destructors are constexpr).
union LoggedArray {
  Logged arr[3];
  constexpr LoggedArray(int* log, int* pos) : arr{{1, log, pos}, {2, log, pos}, {3, log, pos}} {}
  constexpr ~LoggedArray() {}
};
union LoggedMatrix {
  Logged m[2][2];
  constexpr LoggedMatrix(int* log, int* pos) : m{{{10, log, pos}, {11, log, pos}}, {{12, log, pos}, {13, log, pos}}} {}
  constexpr ~LoggedMatrix() {}
};

constexpr bool test_destroy_order() {
  int log[7] = {};
  int pos = 0;
  LoggedArray a(log, &pos);
  std::destroy_at(&a.arr);
  if (pos != 3 || log[0] != 1 || log[1] != 2 || log[2] != 3) return false;
  LoggedMatrix m(log, &pos);
  std::destroy_at(&m.m);
  if (pos != 7 || log[3] != 10 || log[4] != 11 || log[5] != 12 || log[6] != 13) return false;
  return true;
}
static_assert(test_destroy_order());

int main() {
  CHECK(test_destroy_order());
  {
    alignas(int) unsigned char buf[sizeof(int[4])];
    std::memset(buf, 0xFF, sizeof buf);
    int (*p)[4] = std::construct_at(reinterpret_cast<int(*)[4]>(buf));
    CHECK(static_cast<void*>(p) == static_cast<void*>(buf));
    CHECK((*p)[0] == 0 && (*p)[1] == 0 && (*p)[2] == 0 && (*p)[3] == 0);   // value-initialized
    std::destroy_at(p);
  }
  {
    alignas(int) unsigned char buf[sizeof(int[2][3])];
    std::memset(buf, 0xAB, sizeof buf);
    int (*p)[2][3] = std::construct_at(reinterpret_cast<int(*)[2][3]>(buf));
    CHECK((*p)[1][2] == 0 && (*p)[0][0] == 0);
    std::destroy_at(p);
  }
  {
    alignas(Counter) unsigned char buf[sizeof(Counter[3])];
    Counter (*p)[3] = std::construct_at(reinterpret_cast<Counter(*)[3]>(buf));
    CHECK(Counter::live == 3 && (*p)[2].v == 7);
    std::destroy_at(p);
    CHECK(Counter::live == 0);
  }
  {
    int log[4] = {};
    int pos = 0;
    alignas(Logged) unsigned char buf[sizeof(Logged[4])];
    Logged (*p)[4] = std::construct_at(reinterpret_cast<Logged(*)[4]>(buf));
    CHECK((*p)[3].log == nullptr && (*p)[3].id == 0);
    for (int i = 0; i < 4; ++i) { (*p)[i].id = i; (*p)[i].log = log; (*p)[i].pos = &pos; }
    std::destroy_at(p);
    CHECK(pos == 4 && log[0] == 0 && log[1] == 1 && log[2] == 2 && log[3] == 3);
  }
  return 0;
}
