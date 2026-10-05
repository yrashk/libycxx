// [alg.heap.operations] push_heap, pop_heap, make_heap, sort_heap with element types whose
// moved-from state is observable (std::string with long values, unique_ptr): pop_heap
// "Swaps the value in the location first with the value in the location last - 1 and makes
// [first, last - 1) into a heap with respect to comp and proj"; the algorithms permute the
// elements (Preconditions: the type meets Cpp17MoveConstructible and Cpp17MoveAssignable), so
// afterwards every element holds one of the original values exactly once -- no moved-from
// value may remain anywhere -- whatever the number of equal keys. Every size 0..70, several
// key distributions, std:: and ranges:: forms.
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "check.hpp"

static unsigned rng = 12345;
static int rnd(int n) {
  rng = rng * 1103515245u + 12345u;
  return static_cast<int>((rng >> 8) % static_cast<unsigned>(n));
}

static int key_for(int i, int pattern) {
  switch (pattern) {
    case 0: return 4;
    case 1: return rnd(2);
    case 2: return rnd(5);
    case 3: return i;
    case 4: return -i;
    default: return rnd(1000);
  }
}

// long strings: a moved-from std::string is (in practice) empty, which no original value is
static std::string make_str(int key, int tag) {
  return std::string(30, static_cast<char>('a' + (key % 26 + 26) % 26)) + "#" + std::to_string(key) + "#" +
         std::to_string(tag);
}
static int str_key(const std::string& s) {
  auto a = s.find('#');
  auto b = s.find('#', a + 1);
  return std::stoi(s.substr(a + 1, b - a - 1));
}
static int str_tag(const std::string& s) { return std::stoi(s.substr(s.rfind('#') + 1)); }

static void strings(int n, int pattern, bool ranges) {
  std::vector<std::string> v;
  for (int i = 0; i < n; ++i) v.push_back(make_str(key_for(i, pattern), i));
  auto less = [](const std::string& a, const std::string& b) { return str_key(a) < str_key(b); };
  auto check_perm = [&] {
    std::vector<int> seen(static_cast<std::size_t>(n), 0);
    for (const auto& s : v) {
      CHECK(s.size() > 30);
      ++seen[static_cast<std::size_t>(str_tag(s))];
    }
    for (int c : seen) CHECK(c == 1);
  };
  for (int k = 1; k <= n; ++k) {
    if (ranges) std::ranges::push_heap(v.begin(), v.begin() + k, {}, str_key);
    else std::push_heap(v.begin(), v.begin() + k, less);
  }
  check_perm();
  CHECK(std::is_heap(v.begin(), v.end(), less));
  for (int k = n; k >= 1; --k) {
    if (ranges) std::ranges::pop_heap(v.begin(), v.begin() + k, {}, str_key);
    else std::pop_heap(v.begin(), v.begin() + k, less);
    CHECK(std::is_heap(v.begin(), v.begin() + (k - 1), less));
  }
  check_perm();
  CHECK(std::is_sorted(v.begin(), v.end(), less));
  if (ranges) std::ranges::make_heap(v, std::ranges::greater{}, str_key);
  else std::make_heap(v.begin(), v.end(), [&](auto& a, auto& b) { return less(b, a); });
  check_perm();
  if (ranges) std::ranges::sort_heap(v, std::ranges::greater{}, str_key);
  else std::sort_heap(v.begin(), v.end(), [&](auto& a, auto& b) { return less(b, a); });
  check_perm();
  for (int i = 1; i < n; ++i) CHECK(str_key(v[static_cast<std::size_t>(i - 1)]) >= str_key(v[static_cast<std::size_t>(i)]));
}

static void uptrs(int n, int pattern) {
  std::vector<std::unique_ptr<int>> v;
  for (int i = 0; i < n; ++i) v.push_back(std::make_unique<int>(key_for(i, pattern) * 1000 + i));
  auto less = [](const std::unique_ptr<int>& a, const std::unique_ptr<int>& b) { return *a / 1000 < *b / 1000; };
  std::make_heap(v.begin(), v.end(), less);
  for (int k = n; k >= 1; --k) std::pop_heap(v.begin(), v.begin() + k, less);
  std::vector<int> seen(static_cast<std::size_t>(n), 0);
  for (auto& p : v) {
    CHECK(p != nullptr);
    int tag = ((*p % 1000) + 1000) % 1000;
    ++seen[static_cast<std::size_t>(tag)];
  }
  for (int c : seen) CHECK(c == 1);
  for (int i = 1; i < n; ++i) CHECK(!less(v[static_cast<std::size_t>(i)], v[static_cast<std::size_t>(i - 1)]));
}

int main() {
  for (int n = 0; n <= 70; ++n)
    for (int p = 0; p < 6; ++p) {
      strings(n, p, false);
      strings(n, p, true);
      uptrs(n, p);
    }
}
