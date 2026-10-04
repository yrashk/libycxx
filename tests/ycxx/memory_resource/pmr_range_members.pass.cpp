// [container.alloc.reqmts]/2 (Note 2), [sequence.reqmts], [associative.reqmts.general],
// [unord.req.general], [flat.map.modifiers], [flat.set.modifiers]: the C++23 range members --
// from_range constructors with an allocator, append_range, prepend_range, insert_range,
// insert_range_after, assign_range -- and ranges::to<C>(r, alloc) ([range.utility.conv.to]:
// the trailing arguments go to the container's constructor) construct the elements with
// allocator_traits::construct, i.e. for pmr containers by uses-allocator construction with
// the container's polymorphic_allocator ([mem.poly.allocator.mem]/14). So every pmr::string
// element (and map key and mapped value, flat_map keys()/values() and flat_set elements) uses
// the container's resource, whether the source range holds const char* or pmr::strings
// using another resource.
#include <memory_resource>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <map>
#include <set>
#include <unordered_map>
#include <flat_map>
#include <flat_set>
#include <string>
#include <ranges>
#include "check.hpp"
#include "recording_resource.hpp"
using S = std::pmr::string;
static const char* const L = "a string that is too long for any small-string buffer, 0123456789";
static RecordingResource r, other;
static bool on(const S& s) { return s.get_allocator().resource() == &r; }
template <class C>
static bool all(const C& c) { for (auto& e : c) if (!on(e)) return false; return true; }
template <class C>
static bool allk(const C& c) { for (auto& [k,v] : c) if (!on(k) || !on(v)) return false; return true; }
int main() {
  std::vector<S> src{S(L, &other), S(L, &other)};  // strings using another resource
  std::vector<const char*> plain{L, L};
  { std::pmr::vector<S> v(&r); v.append_range(src); v.insert_range(v.begin(), plain); v.assign_range(src); v.append_range(plain); CHECK(all(v)); }
  { std::pmr::vector<S> v(std::from_range, plain, &r); CHECK(all(v)); }
  { auto v = std::ranges::to<std::pmr::vector<S>>(plain, &r); CHECK(all(v)); }
  { auto v = std::ranges::to<std::pmr::vector<S>>(plain | std::views::transform([](const char* s){return s;}), &r); CHECK(all(v)); }
  { std::pmr::deque<S> d(&r); d.append_range(src); d.prepend_range(plain); d.insert_range(d.begin()+1, src); CHECK(all(d)); }
  { std::pmr::list<S> l(&r); l.append_range(src); l.prepend_range(plain); l.insert_range(l.begin(), src); CHECK(all(l)); }
  { std::pmr::forward_list<S> l(&r); l.prepend_range(src); l.insert_range_after(l.begin(), plain); CHECK(all(l)); }
  { std::pmr::set<S> s(&r); s.insert_range(plain); CHECK(all(s)); }
  { std::pmr::map<S,S> m(&r); std::vector<std::pair<const char*,const char*>> pv{{"k1 long long long long long long long", L}}; m.insert_range(pv); CHECK(allk(m)); }
  { std::pmr::unordered_map<S,S> m(&r); std::vector<std::pair<const char*,const char*>> pv{{"k1 long long long long long long long", L}}; m.insert_range(pv); CHECK(allk(m)); }
  { auto m = std::ranges::to<std::pmr::map<S,S>>(std::vector<std::pair<const char*,const char*>>{{"k1 long long long long long long long", L}}, &r); CHECK(allk(m)); }
  { std::pmr::multiset<S> s(std::from_range, plain, std::less<S>{}, &r); CHECK(all(s)); }
  { std::pmr::string s(&r); s.append_range(std::string_view(L)); s.insert_range(s.begin(), std::string_view(L)); CHECK(s.get_allocator().resource()==&r); }
  { std::pmr::polymorphic_allocator<> pa0(&r); std::flat_set<S, std::less<>, std::pmr::vector<S>> fs(pa0); fs.insert_range(plain); fs.insert(S(L, &other)); CHECK(all(fs)); CHECK(fs.size()==1); }
  { using FM = std::flat_map<S,S,std::less<>,std::pmr::vector<S>,std::pmr::vector<S>>; std::pmr::polymorphic_allocator<> pa(&r); FM fm(pa); std::vector<std::pair<const char*,const char*>> pv{{"k1 long long long long long long long", L},{"k0 long long long long long long long long", L}}; fm.insert_range(pv); for (auto& k : fm.keys()) if(!on(k)) CHECK(false); for (auto& v : fm.values()) if(!on(v)) CHECK(false); CHECK(fm.size()==2); }
  { using FM = std::flat_map<S,S,std::less<>,std::pmr::vector<S>,std::pmr::vector<S>>; std::pmr::polymorphic_allocator<> pa(&r); std::vector<std::pair<const char*,const char*>> pv{{"k1 long long long long long long long", L}}; FM fm(std::from_range, pv, pa); for (auto& k : fm.keys()) if(!on(k)) CHECK(false); CHECK(fm.keys().get_allocator().resource()==&r); }
  CHECK(r.outstanding == 0 && other.outstanding == 0);
  return 0;
}
