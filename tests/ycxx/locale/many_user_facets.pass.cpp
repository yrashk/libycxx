// Many user-defined facet types, each with its own locale::id, in one locale and spread over
// threads that install facet types for the first time concurrently.
//   [locale.facet]/1: any class derived from locale::facet with a public "static ::std::locale::id
//     id;" is a facet; the draft sets no limit on how many facet types a program defines.
//   [locale.id]/1: locale::id "provides identification of a locale facet interface, used as an
//     index for lookup"; /2 (note) ids may be initialized the first time a facet is installed.
//   [locale.cons]/8 locale(const locale& other, Facet* f): "the facet f is installed" (the
//     result has all of other's facets plus f); [locale.members] combine<Facet>(other).
//   [locale.global.templates]/2 use_facet "Returns: A reference to the corresponding facet of
//     loc, if present", /3 bad_cast otherwise; /5 has_facet.
//   [locale.facet]/3: for refs == 0 the facet is deleted "when the last locale object containing
//     the facet is destroyed".
//   [res.on.data.races]: installing facets into distinct locale objects in different threads
//     does not race.
// 700 facet types (one locale with 400 of them, 300 split over 6 threads); a facet type whose
// id collided with another's would be reported as present in a locale that does not have it,
// or would return the other type's facet.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <atomic>
#include <locale>
#include <thread>
#include <typeinfo>
#include <utility>
#include <vector>
#include "check.hpp"

static std::atomic<int> live{0};

template<int N>
struct F : std::locale::facet {
  static std::locale::id id;
  int value;
  explicit F(int v) : value(v) { live.fetch_add(1); }
  ~F() override { live.fetch_sub(1); }
};
template<int N> std::locale::id F<N>::id;

template<int N> bool has_own(const std::locale& l) {
  return std::has_facet<F<N>>(l) && std::use_facet<F<N>>(l).value == N;
}

template<int Base, int... N> std::locale add_range(std::locale l, std::integer_sequence<int, N...>) {
  ((l = std::locale(l, new F<Base + N>(Base + N))), ...);
  return l;
}
template<int Base, int... N> int count_own(const std::locale& l, std::integer_sequence<int, N...>) {
  return (0 + ... + int(has_own<Base + N>(l)));
}
template<int Base, int... N> int count_present(const std::locale& l, std::integer_sequence<int, N...>) {
  return (0 + ... + int(std::has_facet<F<Base + N>>(l)));
}
template<int Base, int... N> int count_bad_cast(const std::locale& l, std::integer_sequence<int, N...>) {
  int n = 0;
  auto one = [&]<int K>() {
    try {
      (void)std::use_facet<F<K>>(l);
    } catch (const std::bad_cast&) {
      ++n;
    }
  };
  (one.template operator()<Base + N>(), ...);
  return n;
}

constexpr int Threads = 6, PerThread = 50;
using Seq200 = std::make_integer_sequence<int, 200>;
using SeqT = std::make_integer_sequence<int, PerThread>;

template<int T> void thread_body(std::locale* out, std::atomic<int>* go, std::atomic<int>* bad) {
  while (go->load() == 0) {
  }
  std::locale l = add_range<1000 + T * PerThread>(std::locale::classic(), SeqT{});
  if (count_own<1000 + T * PerThread>(l, SeqT{}) != PerThread) bad->fetch_add(1);
  *out = l;
}

int main() {
  {
    std::locale l = std::locale::classic();
    l = add_range<0>(l, Seq200{});
    l = add_range<200>(l, Seq200{});
    CHECK(live.load() == 400);
    CHECK((count_own<0>(l, Seq200{}) == 200));
    CHECK((count_own<200>(l, Seq200{}) == 200));
    CHECK((count_present<0>(std::locale::classic(), Seq200{}) == 0));
    CHECK((count_bad_cast<200>(std::locale::classic(), Seq200{}) == 200));
    CHECK((count_present<600>(l, Seq200{}) == 0));  // types not installed anywhere yet

    // Replacing one facet: the old one is deleted once no locale has it.
    std::locale before = l;
    l = std::locale(l, new F<7>(-7));
    CHECK(std::use_facet<F<7>>(l).value == -7);
    CHECK(std::use_facet<F<7>>(before).value == 7);
    CHECK(live.load() == 401);
    before = std::locale::classic();
    CHECK(live.load() == 400);

    // combine takes exactly the one facet.
    std::locale c = std::locale::classic().combine<F<399>>(l);
    CHECK(has_own<399>(c));
    CHECK((count_present<0>(c, Seq200{}) == 0));
  }
  CHECK(live.load() == 0);

  std::locale results[Threads];
  std::atomic<int> go{0}, bad{0};
  {
    auto start = [&]<int... T>(std::integer_sequence<int, T...>) {
      std::vector<std::thread> ts;
      (ts.emplace_back(thread_body<T>, &results[T], &go, &bad), ...);
      go.store(1);
      for (auto& t : ts) t.join();
    };
    start(std::make_integer_sequence<int, Threads>{});
  }
  CHECK(bad.load() == 0);
  CHECK(live.load() == Threads * PerThread);
  auto check_thread = [&]<int T>() {
    for (int u = 0; u < Threads; ++u) {
      const int n = count_present<1000 + T * PerThread>(results[u], SeqT{});
      CHECK(n == (u == T ? PerThread : 0));
    }
    CHECK((count_own<1000 + T * PerThread>(results[T], SeqT{}) == PerThread));
  };
  [&]<int... T>(std::integer_sequence<int, T...>) { (check_thread.template operator()<T>(), ...); }(std::make_integer_sequence<int, Threads>{});
  for (auto& r : results) r = std::locale::classic();
  CHECK(live.load() == 0);
  return 0;
}
