// [associative.reqmts.general], [container.alloc.reqmts]: the allocator-extended move constructor
// and the move assignment with unequal allocators that do not propagate move the elements one by
// one. When moving an element throws part of the way through, the source is still a valid
// container ([lib.types.movedfrom], [res.on.exception.handling]): every element it holds can be
// found by lookup and it iterates in order, although the keys already moved out of it are
// moved-from.
#include <map>
#include <set>
#include <string>
#include <utility>
#include "check.hpp"
#include "test_allocators.hpp"

static int moves_until_throw = -1;
struct Mapped {
  int v;
  Mapped(int x) : v(x) {}
  Mapped(const Mapped& o) : v(o.v) {}
  Mapped(Mapped&& o) : v(o.v) {
    if (moves_until_throw >= 0 && moves_until_throw-- == 0)
      throw 1;
  }
};

// A key that is long enough not to fit in a short string, so that moving it empties it.
struct Key {
  std::string s;
  Key(const char* c) : s(std::string(40, 'k') + c) {}
  Key(const Key&) = default;
  Key(Key&& o) : s(std::move(o.s)) {
    if (moves_until_throw >= 0 && moves_until_throw-- == 0)
      throw 1;
  }
  Key& operator=(const Key&) = default;
  Key& operator=(Key&&) = default;
  friend bool operator<(const Key& a, const Key& b) { return a.s < b.s; }
};

template <class C>
void check_findable(const C& c) {
  std::size_t n = 0;
  for (auto& v : c) {
    if constexpr (requires { v.first; })
      CHECK(c.find(v.first) != c.end());
    else
      CHECK(c.find(v) != c.end());
    ++n;
  }
  CHECK(n == c.size());
}

template <class C, class Make>
void run(Make make) {
  using A = typename C::allocator_type;
  for (int k = 0; k != 5; ++k) {
    {
      C src = make(A(1));
      moves_until_throw = k;
      try {
        C dst(std::move(src), A(2));
        CHECK(false);
      } catch (int) {
      }
      moves_until_throw = -1;
      check_findable(src);
    }
    {
      C src = make(A(1));
      C dst(A(2));
      moves_until_throw = k;
      try {
        dst = std::move(src);
        CHECK(false);
      } catch (int) {
      }
      moves_until_throw = -1;
      check_findable(src);
      check_findable(dst);
    }
  }
}

int main() {
  using MA = IdAlloc<std::pair<const std::string, Mapped>>;
  run<std::map<std::string, Mapped, std::less<>, MA>>([](MA a) {
    std::map<std::string, Mapped, std::less<>, MA> m(a);
    for (const char* k : {"a", "b", "c", "d", "e", "f"})
      m.try_emplace(std::string(40, 'k') + k, k[0]);
    return m;
  });
  using MMA = IdAlloc<std::pair<const std::string, Mapped>>;
  run<std::multimap<std::string, Mapped, std::less<>, MMA>>([](MMA a) {
    std::multimap<std::string, Mapped, std::less<>, MMA> m(a);
    for (const char* k : {"a", "b", "b", "c", "d", "e"})
      m.emplace(std::string(40, 'k') + k, k[0]);
    return m;
  });
  using SA = IdAlloc<Key>;
  run<std::set<Key, std::less<>, SA>>([](SA a) {
    std::set<Key, std::less<>, SA> s(a);
    for (const char* k : {"a", "b", "c", "d", "e", "f"})
      s.emplace(k);
    return s;
  });
}
