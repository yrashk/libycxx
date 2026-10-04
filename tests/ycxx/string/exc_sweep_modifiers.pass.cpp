// Exception-injection sweep over basic_string's modifying members with an allocator whose
// allocate throws at its k-th call and source iterators whose operations throw at their k-th
// call, for every k until the operation completes.
//   [string.require]/2: "If any member function or operator of basic_string throws an
//     exception, that function or operator has no other effect on the basic_string object."
//     This is the strong guarantee for EVERY member, including the iterator-pair and range
//     members (append(first, last), insert_range, replace_with_range, assign_range, ...) when
//     the source iterator throws part-way.
//   [string.require]/3, [allocator.requirements.general]: every block allocated is deallocated
//     exactly once with the n passed to allocate (checked by the harness after each run).
// Each operation is run on a short string and on one longer than any small-buffer
// optimization, with pieces long enough to force reallocation.
#include <string>
#include "exc_harness.hpp"

using namespace exh;
using S = std::basic_string<char, std::char_traits<char>, alloc<char>>;

static const char piece[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const int plen = sizeof piece - 1;

template <class Cat>
static range<Cat, char> rng() {
  return range<Cat, char>{piece, piece + plen};
}

static S make(int variant) {
  return variant == 0 ? S("short") : S("a rather long initial string, longer than any SSO buffer");
}

template <class F>
static void go(const char* name, std::initializer_list<Kind> kinds, F op) {
  for (Kind k : kinds)
    for (int variant = 0; variant < 2; ++variant)
      sweep(name, k, [&] {
        S s = make(variant);
        snap before = snap::of(s);
        bool threw = attempt([&] { op(s); });
        if (threw) EXH_EXPECT(snap::of(s) == before, "[string.require]/2 violated: the string changed");
        EXH_EXPECT(s.c_str()[s.size()] == '\0', "c_str() not null-terminated");
        return threw;
      });
}

int main() {
  const auto A = {allocation};
  const auto AI = {allocation, iter_inc, iter_deref, iter_cmp};
  static const S other(piece);
  go("append(const char*)", A, [](S& s) { s.append(piece); });
  go("append(n, c)", A, [](S& s) { s.append(100, 'x'); });
  go("append(const S&)", A, [](S& s) { s.append(other); });
  go("append(string_view)", A, [](S& s) { s.append(std::string_view(piece)); });
  go("append(input first, last)", AI, [](S& s) {
    auto r = rng<in_tag>();
    s.append(r.begin(), r.end());
  });
  go("append(fwd first, last)", AI, [](S& s) {
    auto r = rng<fwd_tag>();
    s.append(r.begin(), r.end());
  });
  go("append(ra first, last)", AI, [](S& s) {
    auto r = rng<ra_tag>();
    s.append(r.begin(), r.end());
  });
  go("append_range(input)", AI, [](S& s) { s.append_range(rng<in_tag>()); });
  go("append_range(fwd)", AI, [](S& s) { s.append_range(rng<fwd_tag>()); });
  go("append_range(ra)", AI, [](S& s) { s.append_range(rng<ra_tag>()); });
  go("operator+=(const char*)", A, [](S& s) { s += piece; });
  for (int variant = 0; variant < 2; ++variant)
    sweep("push_back x80 (the failing call has no effect)", allocation, [&] {
      S s = make(variant);
      snap before = snap::of(s);
      bool threw = attempt([&] {
        for (int i = 0; i < 80; ++i) {
          before = snap::of(s);
          s.push_back('p');
        }
      });
      if (threw) EXH_EXPECT(snap::of(s) == before, "[string.require]/2 violated: push_back changed the string");
      return threw;
    });
  go("insert(1, const char*)", A, [](S& s) { s.insert(1, piece); });
  go("insert(p, n, c)", A, [](S& s) { s.insert(s.begin() + 1, 90, 'q'); });
  go("insert(p, input first, last)", AI, [](S& s) {
    auto r = rng<in_tag>();
    s.insert(s.begin() + 2, r.begin(), r.end());
  });
  go("insert(p, fwd first, last)", AI, [](S& s) {
    auto r = rng<fwd_tag>();
    s.insert(s.begin() + 2, r.begin(), r.end());
  });
  go("insert(p, ra first, last)", AI, [](S& s) {
    auto r = rng<ra_tag>();
    s.insert(s.begin() + 2, r.begin(), r.end());
  });
  go("insert_range(p, input)", AI, [](S& s) { s.insert_range(s.begin() + 2, rng<in_tag>()); });
  go("insert_range(p, ra)", AI, [](S& s) { s.insert_range(s.begin() + 2, rng<ra_tag>()); });
  go("replace(1, 2, const char*)", A, [](S& s) { s.replace(1, 2, piece); });
  go("replace(i1, i2, input first, last)", AI, [](S& s) {
    auto r = rng<in_tag>();
    s.replace(s.begin() + 1, s.begin() + 3, r.begin(), r.end());
  });
  go("replace(i1, i2, fwd first, last)", AI, [](S& s) {
    auto r = rng<fwd_tag>();
    s.replace(s.begin() + 1, s.begin() + 3, r.begin(), r.end());
  });
  go("replace_with_range(i1, i2, input)", AI,
     [](S& s) { s.replace_with_range(s.begin() + 1, s.begin() + 3, rng<in_tag>()); });
  go("replace_with_range(i1, i2, ra)", AI,
     [](S& s) { s.replace_with_range(s.begin() + 1, s.begin() + 3, rng<ra_tag>()); });
  go("assign(const char*)", A, [](S& s) { s.assign(piece); });
  go("assign(input first, last)", AI, [](S& s) {
    auto r = rng<in_tag>();
    s.assign(r.begin(), r.end());
  });
  go("assign(ra first, last)", AI, [](S& s) {
    auto r = rng<ra_tag>();
    s.assign(r.begin(), r.end());
  });
  go("assign_range(input)", AI, [](S& s) { s.assign_range(rng<in_tag>()); });
  go("assign_range(fwd)", AI, [](S& s) { s.assign_range(rng<fwd_tag>()); });
  go("operator=(const S&)", A, [](S& s) { s = other; });
  go("operator=(const char*)", A, [](S& s) { s = piece; });
  for (int variant = 0; variant < 2; ++variant)
    sweep("operator=(S&&), unequal non-propagating allocator", allocation, [&] {
      using A2 = alloc<char, false>;
      using S2 = std::basic_string<char, std::char_traits<char>, A2>;
      S2 a(piece, A2(1));
      S2 b(variant ? "a rather long initial string, longer than any SSO buffer" : "short", A2(2));
      snap before = snap::of(b);
      bool threw = attempt([&] { b = std::move(a); });
      if (threw) EXH_EXPECT(snap::of(b) == before, "[string.require]/2 violated: move assignment changed the string");
      return threw;
    });
  go("resize(200)", A, [](S& s) { s.resize(200, 'r'); });
  go("reserve(300)", A, [](S& s) { s.reserve(300); });
  go("swap with a copy", A, [](S& s) {
    S t(s);
    s.swap(t);
  });
  go("operator+(S, const char*)", A, [](S& s) {
    S t = s + piece;
    (void)t;
  });
  go("S(input first, last)", AI, [](S&) {
    auto r = rng<in_tag>();
    S t(r.begin(), r.end());
  });
  go("S(from_range, fwd)", AI, [](S&) { S t(std::from_range, rng<fwd_tag>()); });
  go("S(const S&)", A, [](S&) { S t(other); });
  go("substr", A, [](S& s) {
    S t = (s + piece).substr(3);
    (void)t;
  });
  go("getline-free: S(n, c)", A, [](S&) { S t(500, 'z'); });

  // shrink_to_fit after a reserve: a non-binding request ([string.capacity]); an allocation
  // failure may be handled by not shrinking, but the contents never change.
  for (int variant = 0; variant < 2; ++variant)
    sweep("shrink_to_fit", allocation, [&] {
      S s = make(variant);
      s.reserve(400);
      snap before = snap::of(s);
      bool threw = attempt([&] { s.shrink_to_fit(); });
      EXH_EXPECT(snap::of(s) == before, "shrink_to_fit changed the contents");
      return threw;
    }, options{.may_swallow = true});
  return finish();
}
