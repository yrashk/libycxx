// [container.alloc.reqmts]/2 (Note 2) and [mem.poly.allocator.mem]/14: allocator-aware
// containers construct their elements with allocator_traits::construct, which for
// polymorphic_allocator is uses-allocator construction ([allocator.uses.construction]); for a
// map's value_type pair<const Key, T> each member gets the container's resource.
// So the pmr::string / pmr::vector keys and mapped values of pmr::map, pmr::multimap,
// pmr::unordered_map, pmr::set, pmr::flat_map and pmr::vector<pmr::vector<pmr::string>>
// (three levels) use the outermost container's resource however the element is created:
// insert(value) with a value using another resource, emplace, emplace_hint, try_emplace
// ([map.modifiers]/7: "constructed ... from piecewise_construct, forward_as_tuple(k),
// forward_as_tuple(args...)"), insert_or_assign, operator[], insert of an initializer_list,
// range insert, vector::insert(pos, n, value), resize(n, value), assign, and growth of the
// inner containers afterwards (they keep the resource they were given).
// [flat.map.cons.alloc]: flat_map's allocator-extended constructors pass the allocator to the
// key and mapped containers, and their elements in turn use it.
#include <memory_resource>
#include <flat_map>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "check.hpp"
#include "recording_resource.hpp"

using S = std::pmr::string;
static const char* const L1 = "first key, long enough not to fit any small-string buffer 0123456789";
static const char* const L2 = "second key, long enough not to fit any small-string buffer 0123456789";
static const char* const L3 = "third key, long enough not to fit any small-string buffer 01234567890";
static const char* const L4 = "fourth key, long enough not to fit any small-string buffer 0123456789";

static bool on(const S& s, std::pmr::memory_resource* r) { return s.get_allocator().resource() == r; }
static bool on(const std::pmr::vector<S>& v, std::pmr::memory_resource* r) {
  if (v.get_allocator().resource() != r) return false;
  for (const S& s : v)
    if (!on(s, r)) return false;
  return true;
}

template <class Map>
void map_like(RecordingResource& r, RecordingResource& other) {
  Map m(&r);
  S k1(L1, &other), v1(L2, &other);
  m.insert(std::pair<const S, S>(k1, v1));
  m.emplace(L2, L3);
  m.emplace_hint(m.end(), S(L3, &other), S(L4, &other));
  if constexpr (requires { m.try_emplace(S(L4, &other), L1); }) {
    m.try_emplace(S(L4, &other), L1);
    m.insert_or_assign(S("fifth key, long enough to defeat small-string buffers 0123456789", &other), S(L2, &other));
    m.insert_or_assign(k1, S(L3, &other));  // existing key: the value is assigned
    m[S("sixth key, long enough to defeat small-string buffers 0123456789", &other)] = S(L4, &other);
    m[L1].append(200, 'x');  // grows after assignment, still in r
  }
  m.insert({{S("seventh key, long enough to defeat small-string buffers 0123456789", &other), S(L1, &other)}});
  std::pair<S, S> range[] = {{S("eighth key, long enough to defeat small-string buffers 0123456789", &other), S(L1, &other)}};
  m.insert(range, range + 1);
  CHECK(m.size() >= 5);
  for (const auto& [k, v] : m) CHECK(on(k, &r) && on(v, &r));
}

int main() {
  RecordingResource r, other, dflt;
  std::pmr::memory_resource* old = std::pmr::set_default_resource(&dflt);
  map_like<std::pmr::map<S, S>>(r, other);
  map_like<std::pmr::multimap<S, S>>(r, other);
  map_like<std::pmr::unordered_map<S, S>>(r, other);
  map_like<std::pmr::unordered_multimap<S, S>>(r, other);
  CHECK(r.outstanding == 0);

  {  // pmr::set: keys constructed with the resource
    std::pmr::set<S> s(&r);
    S k(L1, &other);
    s.insert(k);
    s.emplace(L2);
    s.insert(S(L3, &other));
    s.emplace_hint(s.begin(), L4);
    for (const S& e : s) CHECK(on(e, &r));
  }

  {  // map<S, vector<S>>: three levels through operator[] and later growth
    std::pmr::map<S, std::pmr::vector<S>> m(&r);
    auto& v = m[L1];
    CHECK(v.get_allocator().resource() == &r);
    for (int i = 0; i < 20; ++i) v.emplace_back(L2);
    S outside(L3, &other);
    v.push_back(outside);
    v.insert(v.begin(), 3, outside);
    m.try_emplace(L2, 5, outside);  // vector<S>(5, outside, alloc)
    for (const auto& [k, vv] : m) CHECK(on(k, &r) && on(vv, &r));
    CHECK(m[L2].size() == 5);
  }

  {  // vector<vector<vector<S>>>
    using V1 = std::pmr::vector<S>;
    using V2 = std::pmr::vector<V1>;
    std::pmr::vector<V2> v(&r);
    V1 inner_other({S(L1, &other), S(L2, &other)}, &other);
    V2 mid_other({inner_other, inner_other}, &other);
    v.push_back(mid_other);
    v.resize(4, mid_other);
    v.insert(v.begin() + 1, 2, mid_other);
    v.emplace(v.begin(), 3);  // V2(3, alloc): three empty V1
    v[0][1].emplace_back(L3);
    v.assign(3, mid_other);
    v[2].resize(10, inner_other);
    v[1].emplace_back(4, S(L4, &other));
    for (const V2& mid : v) {
      CHECK(mid.get_allocator().resource() == &r);
      for (const V1& in : mid) CHECK(on(in, &r));
    }
    CHECK(v.size() == 3 && v[2].size() == 10 && v[1].back().size() == 4);
  }
  CHECK(r.outstanding == 0);

  {  // flat_map with pmr containers
    using FM = std::flat_map<S, S, std::less<>, std::pmr::vector<S>, std::pmr::vector<S>>;
    std::pmr::polymorphic_allocator<> pa(&r);
    FM m(pa);
    m.emplace(L1, L2);
    m.try_emplace(S(L2, &other), S(L3, &other));
    m.insert({S(L3, &other), S(L4, &other)});
    m[S(L4, &other)] = S(L1, &other);
    m.insert_or_assign(L1, S(L4, &other));
    CHECK(m.keys().get_allocator().resource() == &r && m.values().get_allocator().resource() == &r);
    for (const S& k : m.keys()) CHECK(on(k, &r));
    for (const S& v : m.values()) CHECK(on(v, &r));
    CHECK(m.size() == 4 && m[L1] == L4);
  }
  CHECK(r.outstanding == 0);  // (temporaries such as m[L1]'s key may use the default resource)
  std::pmr::set_default_resource(old);
  return 0;
}
