// erase_if for the associative and unordered containers. [map.erasure], [multimap.erasure],
// [set.erasure], [multiset.erasure], [unord.map.erasure], [unord.multimap.erasure],
// [unord.set.erasure], [unord.multiset.erasure]: "Effects: Equivalent to:
//   auto original_size = c.size();
//   for (auto i = c.begin(), last = c.end(); i != last; ) {
//     if (pred(*i)) { i = c.erase(i); } else { ++i; } }
//   return original_size - c.size();"
// So pred sees every element once, in iteration order, and the result is the number removed.
// "The erase members shall invalidate only iterators and references to the erased elements"
// ([associative.reqmts.general]/175, [unord.req.general]/242), so the elements kept are the
// same objects, in the same order.
// COUNTERPART: libstdcxx:23_containers/(map|multimap|set|multiset|unordered_map|unordered_multimap|unordered_set|unordered_multiset)/debug/erase_if.cc
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "check.hpp"

int key_of(int k) { return k; }
template <class A, class B>
int key_of(const std::pair<A, B>& p) { return p.first; }

template <class C, class Make>
void run(Make make) {
  C c;
  for (int i = 0; i < 20; ++i) c.insert(make(i % 10));  // the multi containers hold duplicates
  const std::size_t before = c.size();
  std::vector<const typename C::value_type*> kept;
  std::vector<int> kept_keys;
  for (auto& v : c)
    if (key_of(v) % 3 != 0) {
      kept.push_back(&v);
      kept_keys.push_back(key_of(v));
    }
  std::vector<int> order;
  for (auto& v : c) order.push_back(key_of(v));

  std::vector<int> seen;
  const auto n = std::erase_if(c, [&](const typename C::value_type& v) {
    seen.push_back(key_of(v));
    return key_of(v) % 3 == 0;
  });
  static_assert(std::is_same_v<decltype(n), const typename C::size_type>);
  CHECK(seen == order);  // every element once, in iteration order
  CHECK(n == before - c.size());
  CHECK(c.size() == kept.size());
  std::size_t i = 0;
  for (auto& v : c) {  // the kept elements are the same objects, in the same order
    CHECK(key_of(v) % 3 != 0);
    CHECK(&v == kept[i] && key_of(v) == kept_keys[i]);
    ++i;
  }
  CHECK(std::erase_if(c, [](const auto&) { return false; }) == 0);
  CHECK(std::erase_if(c, [](const auto&) { return true; }) == kept.size() && c.empty());
}

int main() {
  auto kv = [](int k) { return std::pair<const int, std::string>(k, std::to_string(k)); };
  auto k = [](int x) { return x; };
  run<std::map<int, std::string>>(kv);
  run<std::multimap<int, std::string>>(kv);
  run<std::set<int>>(k);
  run<std::multiset<int>>(k);
  run<std::unordered_map<int, std::string>>(kv);
  run<std::unordered_multimap<int, std::string>>(kv);
  run<std::unordered_set<int>>(k);
  run<std::unordered_multiset<int>>(k);
}
