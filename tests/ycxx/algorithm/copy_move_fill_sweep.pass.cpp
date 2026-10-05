// [alg.copy]: copy assigns *(result + n) = *(first + n) "for each non-negative integer n < N"
// in increasing order (so the destination may overlap the source on the left: Preconditions
// only exclude result in [first, last)); copy_backward assigns *(result - n) = *(last - n) for
// n = 1..N (result not in (first, last]), returns result - N; copy_n likewise; "Complexity:
// Exactly N assignments". [alg.move]: the same with std::move. [alg.fill]: fill/fill_n assign
// value to every element, i.e. the value converted to the element type (bool(256) is true).
// Swept over every length 0..20, source offset and shift in both allowed directions, for
// trivially copyable element types, a type with a counted user-provided assignment, copies
// between different element types (conversions), volatile elements, and reverse iterators;
// against a model computed element by element.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <type_traits>
#include "check.hpp"

static long assigns = 0;
struct Counted {
  int v;
  Counted(int x = 0) : v(x) {}
  Counted(const Counted&) = default;
  Counted& operator=(const Counted& o) {
    ++assigns;
    v = o.v;
    return *this;
  }
  bool operator==(int x) const { return v == x; }
};
struct Pod {
  int a;
  char c;
  Pod(int x = 0) : a(x), c(static_cast<char>(x)) {}
  bool operator==(int x) const { return a == x && c == static_cast<char>(x); }
};

constexpr int M = 48;

template <class T>
int val(const T& t) {
  if constexpr (std::is_arithmetic_v<T>) return static_cast<int>(t);
  else return t.v;
}
template <class T>
bool same(const T& t, int x) {
  if constexpr (std::is_arithmetic_v<T>) return t == static_cast<T>(x);
  else return t == x;
}

// overlapping copies/moves within one array of T
template <class T>
void overlap() {
  T a[M];
  int model[M];
  auto reset = [&] {
    for (int i = 0; i < M; ++i) {
      a[i] = T(i + 1);
      model[i] = i + 1;
    }
  };
  for (int n = 0; n <= 20; ++n)
    for (int src = 0; src + n <= M; src += 3)
      for (int shift = -n - 2; shift <= n + 2; ++shift) {
        const int dst = src + shift;
        if (dst < 0 || dst + n > M) continue;
        for (int alg = 0; alg < 8; ++alg) {
          const bool backward = alg % 2 == 1;
          // copy/move (forward) need dst <= src or no overlap; *_backward need dst >= src
          if (!backward && shift > 0 && shift < n) continue;
          if (backward && shift < 0 && -shift < n) continue;
          reset();
          long a0 = assigns;
          T* r = nullptr;
          switch (alg) {
            case 0: r = std::copy(a + src, a + src + n, a + dst); break;
            case 1: r = std::copy_backward(a + src, a + src + n, a + dst + n); break;
            case 2: r = std::move(a + src, a + src + n, a + dst); break;
            case 3: r = std::move_backward(a + src, a + src + n, a + dst + n); break;
            case 4: r = std::ranges::copy(a + src, a + src + n, a + dst).out; break;
            case 5: r = std::ranges::copy_backward(a + src, a + src + n, a + dst + n).out; break;
            case 6: r = std::copy_n(a + src, n, a + dst); break;
            case 7: r = std::ranges::move_backward(a + src, a + src + n, a + dst + n).out; break;
          }
          if constexpr (std::is_same_v<T, Counted>) CHECK(assigns - a0 == n);
          CHECK(r == (backward ? a + dst : a + dst + n));
          // the model, element by element in the algorithm's order
          if (!backward)
            for (int k = 0; k < n; ++k) model[dst + k] = model[src + k];
          else
            for (int k = 1; k <= n; ++k) model[dst + n - k] = model[src + n - k];
          for (int i = 0; i < M; ++i) CHECK(same(a[i], model[i]));
        }
      }
}

// copies between different types and with reverse iterators / volatile elements
static void conversions() {
  int src[20];
  for (int i = 0; i < 20; ++i) src[i] = (i * 37) - 300;  // values outside char's range too
  long l[20];
  double d[20];
  signed char c[20];
  unsigned char uc[20];
  bool b[20];
  volatile int vi[20];
  std::copy(src, src + 20, l);
  std::copy(src, src + 20, d);
  std::copy(src, src + 20, c);
  std::copy(src, src + 20, uc);
  std::copy(src, src + 20, b);
  std::copy(src, src + 20, vi);
  for (int i = 0; i < 20; ++i) {
    CHECK(l[i] == src[i] && d[i] == src[i]);
    CHECK(c[i] == static_cast<signed char>(src[i]) && uc[i] == static_cast<unsigned char>(src[i]));
    CHECK(b[i] == (src[i] != 0));
    CHECK(vi[i] == src[i]);
  }
  // back from volatile, and unsigned char -> char widening/narrowing
  int back[20];
  std::copy(vi, vi + 20, back);
  CHECK(std::equal(back, back + 20, src));
  int w[20];
  std::copy(uc, uc + 20, w);
  for (int i = 0; i < 20; ++i) CHECK(w[i] == static_cast<unsigned char>(src[i]));
  // reverse iterators: the order is reversed, not just the range copied
  int r[20];
  std::copy(std::make_reverse_iterator(src + 20), std::make_reverse_iterator(src), r);
  for (int i = 0; i < 20; ++i) CHECK(r[i] == src[19 - i]);
  int r2[20] = {};
  std::copy(src, src + 20, std::make_reverse_iterator(r2 + 20));
  for (int i = 0; i < 20; ++i) CHECK(r2[i] == src[19 - i]);
  // overlapping through reverse iterators: shifting right by 3 is a forward copy in reverse
  int s[30];
  for (int i = 0; i < 30; ++i) s[i] = i;
  std::copy(std::make_reverse_iterator(s + 20), std::make_reverse_iterator(s), std::make_reverse_iterator(s + 23));
  for (int i = 0; i < 30; ++i) CHECK(s[i] == (i < 3 ? i : i < 23 ? i - 3 : i));
}

static void fills() {
  bool b[40];
  std::memset(b, 0, sizeof b);
  std::fill(b, b + 40, 256);  // bool(256) == true
  const bool t = true;
  for (bool& x : b) CHECK(std::memcmp(&x, &t, 1) == 0);
  std::fill_n(b, 40, 0);
  for (bool x : b) CHECK(!x);
  std::fill_n(b + 3, 30, 2);
  for (int i = 0; i < 40; ++i) CHECK(std::memcmp(&b[i], i >= 3 && i < 33 ? &t : &b[0], 1) == 0);
  std::ranges::fill(b, 512);
  for (bool& x : b) CHECK(std::memcmp(&x, &t, 1) == 0);

  signed char c[40];
  std::fill(c, c + 40, 300);  // 300 converted: 44
  for (signed char x : c) CHECK(x == static_cast<signed char>(300));
  unsigned char u[40];
  std::ranges::fill(u, -1);
  for (unsigned char x : u) CHECK(x == 255);
  volatile char vc[40];
  std::fill(vc, vc + 40, 'q');
  for (int i = 0; i < 40; ++i) CHECK(vc[i] == 'q');
  double d[40];
  std::fill(d, d + 40, -0.0);
  for (double x : d) CHECK(x == 0 && std::signbit(x));
  for (int len = 0; len <= 33; ++len)
    for (int off = 0; off + len <= 40; off += 5) {
      std::fill(c, c + 40, 1);
      signed char* e = std::fill_n(c + off, len, 'z');
      CHECK(e == c + off + len);
      for (int i = 0; i < 40; ++i) CHECK(c[i] == (i >= off && i < off + len ? 'z' : 1));
    }
}

int main() {
  overlap<char>();
  overlap<int>();
  overlap<double>();
  overlap<Pod>();
  overlap<Counted>();
  conversions();
  fills();
}
