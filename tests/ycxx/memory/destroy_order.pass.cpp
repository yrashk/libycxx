// [specialized.destroy]/1: destroy_at(location): for an array type "equivalent to
// destroy(begin(*location), end(*location))", otherwise "location->~T()". /2: destroy(first,
// last) is "for (; first != last; ++first) destroy_at(addressof(*first));" (front to back).
// /4: destroy_n(first, n) is "for (; n > 0; (void)++first, --n)
// destroy_at(addressof(*first)); return first;". addressof is used, so an overloaded unary &
// is not called.
#include <memory>
#include <new>
#include "check.hpp"

int order[16];
int count = 0;
struct Rec {
  int id;
  ~Rec() { order[count++] = id; }
  void operator&() const = delete;
};

int main() {
  alignas(Rec) unsigned char buf[sizeof(Rec) * 6];
  Rec* r = reinterpret_cast<Rec*>(buf);
  for (int i = 0; i < 6; ++i) ::new (static_cast<void*>(buf + i * sizeof(Rec))) Rec{i};
  r = std::launder(r);
  std::destroy(r, r + 3);
  CHECK(count == 3 && order[0] == 0 && order[1] == 1 && order[2] == 2);
  Rec* end = std::destroy_n(r + 3, 2);
  CHECK(end == r + 5);
  CHECK(count == 5 && order[3] == 3 && order[4] == 4);
  std::destroy_at(r + 5);
  CHECK(count == 6 && order[5] == 5);
  // destroy_n with n == 0 (and a negative n) does nothing and returns first
  CHECK(std::destroy_n(r, 0) == r);
  CHECK(std::destroy_n(r, -1) == r);
  CHECK(count == 6);

  // array: elements destroyed first to last, recursively for nested arrays
  count = 0;
  union Holder {
    Rec m[2][2];
    Holder() : m{{{10}, {11}}, {{12}, {13}}} {}
    ~Holder() {}
  } h;
  std::destroy_at(std::addressof(h.m));
  CHECK(count == 4 && order[0] == 10 && order[1] == 11 && order[2] == 12 && order[3] == 13);
  return 0;
}
