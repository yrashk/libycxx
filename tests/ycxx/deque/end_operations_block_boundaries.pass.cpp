// push_front/push_back/pop_front/pop_back on deques of element types of many sizes (so that
// the elements of any internal block layout are crossed at every offset), checked after every
// operation against an array model together with the iterator arithmetic of
// [random.access.iterators] (a + n, b - a, a < b, ==, [], --/++), [container.reqmts]
// (size() == distance(begin(), end()), empty() iff begin() == end()) and the validity rules of
// [deque.modifiers]/1,/4: an insertion at either end invalidates iterators but "has no effect
// on the validity of references"; "An erase operation that erases the first element of a deque
// but not the last element invalidates only iterators and references to the erased elements"
// (so a saved end() still equals end() after pop_front), and one that "erases the last
// element" invalidates only the past-the-end iterator and the erased elements (so a saved
// begin() and iterators to the remaining elements stay valid after pop_back).
// Patterns: fill at one end and drain at either end, starting from deques whose first element
// was moved through a block by earlier push/pop pairs; alternating ends; sliding windows of
// fixed size in both directions; growth after becoming empty.
#include <deque>
#include <cstddef>
#include <iterator>
#include "check.hpp"

template <std::size_t Sz>
struct Elem {
  int v;
  unsigned char pad[Sz - sizeof(int)];
  Elem(int x) : v(x) {
    for (auto& c : pad) c = static_cast<unsigned char>(x);
  }
  bool ok(int x) const {
    if (v != x) return false;
    for (auto c : pad)
      if (c != static_cast<unsigned char>(x)) return false;
    return true;
  }
};
struct Word {
  int v;
  Word(int x) : v(x) {}
  bool ok(int x) const { return v == x; }
};
struct Byte {
  unsigned char v;
  Byte(int x) : v(static_cast<unsigned char>(x)) {}
  bool ok(int x) const { return v == static_cast<unsigned char>(x); }
};

// The model: values model[head, tail).
constexpr int CAP = 1 << 17;
static int model[2 * CAP];
static int head, tail;
static int next_value;
static long checks;

template <class D>
void verify(const D& d) {
  const long n = tail - head;
  CHECK(static_cast<long>(d.size()) == n);
  CHECK(d.empty() == (n == 0));
  CHECK((d.begin() == d.end()) == (n == 0));
  CHECK((d.cbegin() != d.cend()) == (n != 0));
  CHECK(d.end() - d.begin() == n);
  CHECK(d.begin() - d.end() == -n);
  CHECK(std::distance(d.begin(), d.end()) == n);
  CHECK(d.begin() + n == d.end());
  CHECK(n + d.begin() == d.end());
  CHECK(d.end() - n == d.begin());
  CHECK(d.rbegin() + n == d.rend());
  CHECK(d.begin() <= d.end() && d.end() >= d.begin());
  CHECK((d.begin() < d.end()) == (n != 0));
  ++checks;
  if (n > 0) {
    CHECK(d.front().ok(model[head]));
    CHECK(d.back().ok(model[tail - 1]));
  }
  if (n > 0 && (checks % 5 == 0 || n < 4)) {
    CHECK(d.front().ok(model[head]));
    CHECK(d.back().ok(model[tail - 1]));
    CHECK((d.end() - 1)->ok(model[tail - 1]));
    auto last = d.end();
    --last;
    CHECK(last == d.begin() + (n - 1));
    CHECK(d.rbegin()->ok(model[tail - 1]));
    long idx[] = {0, 1, n / 3, n / 2, n - 2, n - 1};
    for (long i : idx) {
      if (i < 0 || i >= n) continue;
      auto it = d.begin() + i;
      CHECK(it->ok(model[head + i]));
      CHECK(d.begin()[i].ok(model[head + i]));
      CHECK(d[static_cast<std::size_t>(i)].ok(model[head + i]));
      CHECK(it - d.begin() == i);
      CHECK(d.end() - it == n - i);
      CHECK(d.begin() + i == it && it - i == d.begin());
      auto j = it;
      ++j;
      CHECK(j == d.begin() + (i + 1));
      CHECK(j - it == 1 && it < j && j > it);
      --j;
      CHECK(j == it);
      auto k = d.end();
      k -= (n - i);
      CHECK(k == it);
    }
  }
  if (checks % 211 == 0 || n < 3) {
    long i = 0;
    for (auto it = d.begin(); it != d.end(); ++it, ++i) CHECK(it->ok(model[head + i]));
    CHECK(i == n);
    for (auto it = d.end(); it != d.begin();) {
      --it;
      --i;
      CHECK(it->ok(model[head + i]));
    }
    CHECK(i == 0);
  }
}

template <class D>
void push_back(D& d) {
  d.push_back(next_value);
  model[tail++] = next_value++;
}
template <class D>
void push_front(D& d) {
  d.push_front(next_value);
  model[--head] = next_value++;
}

// Drains at the front: a saved end() stays valid until the last element goes, and so do
// iterators to the remaining elements.
template <class D>
void drain_front(D& d) {
  auto e = d.end();
  auto last = d.end();
  if (!d.empty()) --last;
  while (d.size() > 1) {
    d.pop_front();
    ++head;
    CHECK(e == d.end());
    CHECK(e - last == 1);
    CHECK(last->ok(model[tail - 1]));
    CHECK(last - d.begin() == static_cast<long>(d.size()) - 1);
    verify(d);
  }
  if (!d.empty()) {
    d.pop_front();
    ++head;
  }
  verify(d);
  CHECK(d.begin() == d.end());
}

// Drains at the back: a saved begin() stays valid until the last element goes.
template <class D>
void drain_back(D& d) {
  auto b = d.begin();
  while (d.size() > 1) {
    auto* first = &d.front();
    d.pop_back();
    --tail;
    CHECK(b == d.begin());
    CHECK(&*b == first);
    CHECK(d.end() - b == static_cast<long>(d.size()));
    verify(d);
  }
  if (!d.empty()) {
    d.pop_back();
    --tail;
  }
  verify(d);
  CHECK(d.begin() == d.end());
}

template <class T>
void run(int maxn) {
  using D = std::deque<T>;
  const int sizes[] = {0, 1, 2, 3, 5, 8, maxn / 7, maxn / 3, maxn + 1};
  const int shifts[] = {0, 1, 7, maxn / 2 + 3};
  for (int shift : shifts)
    for (int n : sizes)
      for (int pattern = 0; pattern < 6; ++pattern) {
        D d;
        head = tail = CAP;
        // move the position of the first element through the block structure
        for (int i = 0; i < shift; ++i) {
          if (pattern % 2 == 0) {
            push_back(d);
            d.pop_front();
            ++head;
          } else {
            push_front(d);
            d.pop_back();
            --tail;
          }
        }
        verify(d);
        CHECK(d.begin() == d.end());
        // fill, holding references that must stay valid ([deque.modifiers]/1)
        const T* first_ref = nullptr;
        int first_val = 0;
        for (int i = 0; i < n; ++i) {
          switch (pattern) {
            case 0: case 1: push_back(d); break;
            case 2: case 3: push_front(d); break;
            default: (i % 2 ? push_back(d) : push_front(d)); break;
          }
          if (i == 0) {
            first_ref = &d.front();
            first_val = model[head];
          }
          CHECK(first_ref->ok(first_val));
          verify(d);
        }
        if (pattern % 2 == 0) drain_front(d);
        else drain_back(d);
        // grows again after being emptied
        for (int i = 0; i < 3; ++i) {
          push_back(d);
          push_front(d);
          verify(d);
        }
      }

  // Sliding windows of fixed width, forwards (push_back + pop_front) and backwards.
  for (int width : {0, 1, 2, 3, 9}) {
    for (int dir = 0; dir < 2; ++dir) {
      D d;
      head = tail = CAP;
      for (int i = 0; i < width; ++i) dir ? push_front(d) : push_back(d);
      auto e = d.end();
      for (int step = 0; step < 3 * maxn; ++step) {
        if (dir == 0) {
          push_back(d);
          verify(d);
          d.pop_front();
          ++head;
          if (width > 0) {
            e = d.end();  // push_back invalidated the old one; pop_front keeps this one
            CHECK(e == d.end());
          }
        } else {
          push_front(d);
          auto b = d.begin();
          d.pop_back();
          --tail;
          if (width > 0) CHECK(b == d.begin());
        }
        verify(d);
      }
    }
  }
}

int main() {
  run<Byte>(5000);
  run<Word>(1500);
  run<Elem<8>>(900);
  run<Elem<24>>(400);
  run<Elem<64>>(200);
  run<Elem<200>>(80);
  run<Elem<600>>(40);
  run<Elem<5000>>(12);
}
