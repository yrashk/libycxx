// [res.on.data.races]/2-3: a library function "shall not directly or indirectly access objects
// accessible by threads other than the current thread unless the objects are accessed directly
// or indirectly via the function's arguments, including this", and "shall not directly or
// indirectly modify" them "unless the objects are accessed directly or indirectly via the
// function's non-const arguments, including this". [container.requirements.dataraces]/1: "for
// purposes of avoiding data races ([res.on.data.races]), implementations shall consider the
// following functions to be const: begin, end, rbegin, rend, front, back, data, find,
// lower_bound, upper_bound, equal_range, at and, except in associative or unordered
// associative containers, operator[]"; /2: no data race when "the contents of the contained
// object in different elements in the same container, excepting vector<bool>, are modified
// concurrently".
// Several threads use the same containers and strings at once through const access only
// (lookups, iteration, copies, comparisons, hashing, formatting), through the functions
// listed as const above on the non-const objects, by modifying different elements of the same
// containers (including adjacent chars), and with distinct objects of the same types modified;
// every result is checked. Meant to be run under TSan.
// FLAGS: -pthread
#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <flat_map>
#include <flat_set>
#include <format>
#include <forward_list>
#include <functional>
#include <latch>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "check.hpp"

constexpr int K = 4;
constexpr int N = 500;

struct Shared {
  std::vector<int> v;
  std::deque<int> d;
  std::list<int> l;
  std::forward_list<int> fl;
  std::map<int, std::string> m;
  std::multimap<int, int> mm;
  std::set<std::string> s;
  std::unordered_map<std::string, int> um;
  std::unordered_multiset<int> ums;
  std::flat_map<int, int> fm;
  std::flat_set<int> fs;
  std::string str;
  std::wstring wstr;
  std::vector<bool> vb;
  Shared() {
    for (int i = 0; i < N; ++i) {
      v.push_back(i);
      d.push_back(i);
      l.push_back(i);
      fl.push_front(N - 1 - i);
      m.emplace(i, std::to_string(i));
      mm.emplace(i % 50, i);
      s.insert(std::to_string(i));
      um.emplace(std::to_string(i), i);
      ums.insert(i % 37);
      fm.emplace(i, 2 * i);
      fs.insert(3 * i);
      str += static_cast<char>('a' + i % 26);
      wstr += static_cast<wchar_t>(L'a' + i % 26);
      vb.push_back(i % 3 == 0);
    }
  }
};

static void reader(Shared& sh, int k) {
  const Shared& c = sh;
  for (int r = 0; r < 20; ++r) {
    const int i = (k * 131 + r * 17) % N;
    // const access
    CHECK(c.v.at(static_cast<std::size_t>(i)) == i && c.v[static_cast<std::size_t>(i)] == i);
    CHECK(std::find(c.v.begin(), c.v.end(), i) - c.v.begin() == i);
    CHECK(c.d[static_cast<std::size_t>(i)] == i && c.d.at(0) == 0 && c.d.back() == N - 1);
    CHECK(*std::next(c.l.begin(), i) == i && c.l.size() == N);
    CHECK(*std::next(c.fl.begin(), i) == i);
    CHECK(c.m.at(i) == std::to_string(i) && c.m.find(i)->second == std::to_string(i));
    CHECK(c.m.lower_bound(i)->first == i && c.m.count(i) == 1 && c.m.contains(i));
    CHECK(c.mm.count(i % 50) == N / 50);
    auto er = c.mm.equal_range(i % 50);
    CHECK(std::distance(er.first, er.second) == N / 50);
    CHECK(c.s.contains(std::to_string(i)) && !c.s.contains("x"));
    CHECK(c.um.at(std::to_string(i)) == i && c.um.find(std::to_string(i))->second == i);
    CHECK(c.um.count("nope") == 0 && c.um.bucket_count() > 0);
    CHECK(c.ums.count(i % 37) == static_cast<std::size_t>(N / 37 + (i % 37 < N % 37 ? 1 : 0)));
    CHECK(c.fm.at(i) == 2 * i && c.fm.find(i)->second == 2 * i && c.fm.keys()[static_cast<std::size_t>(i)] == i);
    CHECK(c.fs.contains(3 * i) && !c.fs.contains(3 * i + 1));
    CHECK(c.str.find(static_cast<char>('a' + i % 26)) == static_cast<std::size_t>(i % 26));
    CHECK(c.str.substr(static_cast<std::size_t>(i), 1)[0] == 'a' + i % 26);
    CHECK(c.str.compare(0, 3, "abc") == 0 && c.str.starts_with("abc") && c.str.c_str()[N] == 0);
    CHECK(std::hash<std::string>{}(c.str) == std::hash<std::string_view>{}(std::string_view(c.str)));
    CHECK(c.wstr.find(L"xyz") == 23 && c.wstr.size() == N);
    CHECK(c.vb[static_cast<std::size_t>(i)] == (i % 3 == 0));
    CHECK(std::count(c.vb.begin(), c.vb.end(), true) == (N + 2) / 3);
    // copies of shared objects
    std::vector<int> v2 = c.v;
    std::map<int, std::string> m2 = c.m;
    std::unordered_map<std::string, int> um2 = c.um;
    std::string s2 = c.str;
    CHECK(v2 == c.v && m2 == c.m && um2 == c.um && s2 == c.str && !(s2 < c.str));
    CHECK((c.v <=> v2) == 0 && (c.d <=> std::deque<int>(c.d)) == 0 && c.l == std::list<int>(c.l));
    CHECK(std::format("{}", c.v.front()) == "0" && std::format("{}", c.str).size() == N);
    CHECK(std::format("{}", std::vector<int>(c.v.begin(), c.v.begin() + 3)) == "[0, 1, 2]");
    // the functions [container.reqmts] treats as const, on the non-const objects
    CHECK(sh.v.data()[i] == i && sh.v.front() == 0 && *sh.v.begin() == 0 && *sh.v.rbegin() == N - 1);
    CHECK(sh.v[static_cast<std::size_t>(i)] == i && sh.d[static_cast<std::size_t>(i)] == i);
    CHECK(sh.m.find(i)->second == std::to_string(i) && sh.m.at(i).size() >= 1);
    CHECK(sh.um.find(std::to_string(i))->second == i && sh.um.at(std::to_string(i)) == i);
    CHECK(sh.fm.find(i)->second == 2 * i && sh.fm.lower_bound(i)->first == i);
    CHECK(sh.str.data()[i] == 'a' + i % 26 && sh.str.at(static_cast<std::size_t>(i)) == 'a' + i % 26);
    // distinct objects of the same types, modified
    std::vector<int> own(c.v.begin(), c.v.begin() + 50);
    own.insert(own.begin(), k);
    own.erase(own.begin() + 10);
    std::unordered_map<std::string, int> uown;
    for (int j = 0; j < 100; ++j) uown[std::to_string(j + k)] = j;
    uown.rehash(500);
    CHECK(uown.size() == 100 && uown.at(std::to_string(k)) == 0);
    std::string sown = c.str;
    sown.append(sown).replace(3, 4, "XY").insert(0, 100, 'q');
    CHECK(sown.size() == 2 * N - 2 + 100);
    std::map<int, std::string> mown = c.m;
    mown.erase(i);
    CHECK(mown.size() == N - 1);
  }
}

struct Elements {
  std::vector<int> v = std::vector<int>(N);
  std::deque<char> d = std::deque<char>(N);
  std::string str = std::string(N, '.');
  std::array<char, N> a{};
  std::map<int, int> m;
  std::unordered_map<int, int> um;
  std::list<short> l = std::list<short>(N);
  Elements() {
    for (int i = 0; i < N; ++i) m[i] = um[i] = 0;
  }
};

// thread k writes the elements i with i % K == k, through iterators and operator[] / at / find
static void writer(Elements& e, int k) {
  for (int round = 1; round <= 3; ++round) {
    auto lit = e.l.begin();
    for (int i = 0; i < N; ++i, ++lit) {
      if (i % K != k) continue;
      const auto u = static_cast<std::size_t>(i);
      e.v[u] = i * round;
      *(e.d.begin() + i) = static_cast<char>(k + round);
      e.str[u] = static_cast<char>('a' + k);
      e.a.at(u) = static_cast<char>(round);
      e.m.find(i)->second = round;
      e.um.at(i) = round * k;
      *lit = static_cast<short>(round);
    }
  }
}

int main() {
  {
    Elements e;
    std::latch go(K);
    std::vector<std::thread> ts;
    for (int k = 0; k < K; ++k)
      ts.emplace_back([&, k] {
        go.arrive_and_wait();
        writer(e, k);
      });
    for (auto& t : ts) t.join();
    auto lit = e.l.begin();
    for (int i = 0; i < N; ++i, ++lit) {
      const auto u = static_cast<std::size_t>(i);
      const int k = i % K;
      CHECK(e.v[u] == 3 * i && e.d[u] == k + 3 && e.str[u] == 'a' + k && e.a[u] == 3);
      CHECK(e.m[i] == 3 && e.um[i] == 3 * k && *lit == 3);
    }
  }
  Shared sh;
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      go.arrive_and_wait();
      reader(sh, k);
    });
  for (auto& t : ts) t.join();
}
