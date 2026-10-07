// Spec-coverage probe, part 2 (docs/SPEC_COVERAGE.md), written by hand: the C++26 constexpr
// requirements of [containers], [algorithms] and [strings] that the generated probes do not
// evaluate. Each line ending in `// @M<n> constexpr` evaluates one facility in a constant
// expression (tools/spec_audit/run_probes.py attributes a failure to that line).
#include <algorithm>
#include <array>
#include <deque>
#include <flat_map>
#include <flat_set>
#include <forward_list>
#include <inplace_vector>
#include <iterator>
#include <list>
#include <map>
#include <mdspan>
#include <memory>
#include <numeric>
#include <queue>
#include <ranges>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// [containers]: every container and adaptor of the draft but hive is constexpr (P3372)
static_assert([] { std::deque<int> d{3, 1}; d.push_front(0); d.emplace_back(4); d.pop_front(); return d.size() == 3 && d[0] == 3; }()); // @M1 constexpr
static_assert([] { std::list<int> l{3, 1, 2}; l.sort(); l.reverse(); l.unique(); return l.front() == 3; }()); // @M2 constexpr
static_assert([] { std::forward_list<int> l{3, 1, 2}; l.sort(); l.push_front(0); return l.front() == 0; }()); // @M3 constexpr
static_assert([] { std::map<int, int> m{{2, 1}}; m[3] = 4; m.try_emplace(5, 6); m.erase(2); return m.size() == 2 && m.at(3) == 4; }()); // @M4 constexpr
static_assert([] { std::multimap<int, int> m{{2, 1}, {2, 2}}; return m.count(2) == 2; }()); // @M5 constexpr
static_assert([] { std::set<int> s{3, 1, 2}; s.insert(0); auto n = s.extract(1); return s.size() == 3 && !n.empty(); }()); // @M6 constexpr
static_assert([] { std::multiset<int> s{1, 1}; return s.count(1) == 2; }()); // @M7 constexpr
static_assert([] { std::unordered_map<int, int> m{{1, 2}}; m.rehash(16); m[3] = 4; return m.at(3) == 4 && m.size() == 2; }()); // @M8 constexpr
static_assert([] { std::unordered_multimap<int, int> m{{1, 2}, {1, 3}}; return m.count(1) == 2; }()); // @M9 constexpr
static_assert([] { std::unordered_set<int> s{1, 2}; s.erase(1); return s.size() == 1; }()); // @M10 constexpr
static_assert([] { std::unordered_multiset<int> s{1, 1}; return s.count(1) == 2; }()); // @M11 constexpr
static_assert([] { std::flat_map<int, int> m{{2, 1}}; m[1] = 0; return m.begin()->first == 1; }()); // @M12 constexpr
static_assert([] { std::flat_multimap<int, int> m{{2, 1}, {2, 2}}; return m.count(2) == 2; }()); // @M13 constexpr
static_assert([] { std::flat_set<int> s{3, 1}; s.insert(2); return *s.begin() == 1; }()); // @M14 constexpr
static_assert([] { std::flat_multiset<int> s{1, 1}; return s.count(1) == 2; }()); // @M15 constexpr
static_assert([] { std::priority_queue<int> q; q.push(1); q.push(3); return q.top() == 3; }()); // @M16 constexpr
static_assert([] { std::queue<int> q; q.push(1); q.push(2); q.pop(); return q.front() == 2; }()); // @M17 constexpr
static_assert([] { std::stack<int> s; s.push(1); s.push(2); return s.top() == 2; }()); // @M18 constexpr
static_assert([] { std::inplace_vector<int, 4> v{1, 2}; v.try_push_back(3); v.unchecked_push_back(4); return v.size() == 4 && !v.try_push_back(5).has_value(); }()); // @M19 constexpr
static_assert([] { std::vector<int> v{1, 2}; v.insert_range(v.end(), std::array{3, 4}); return v.size() == 4; }()); // @M20 constexpr
static_assert([] { std::vector<bool> v(3, true); v.flip(); return !v[1]; }()); // @M21 constexpr
static_assert([] { std::array<int, 3> a{3, 1, 2}; std::ranges::sort(a); return a[0] == 1; }()); // @M22 constexpr
static_assert([] { int a[6] = {1, 2, 3, 4, 5, 6}; std::mdspan m(a, 2, 3); return m[1, 2] == 6 && m.extent(1) == 3; }()); // @M23 constexpr
static_assert([] { int a[3] = {1, 2, 3}; std::span s(a); return s.subspan(1).front() == 2 && s.at(2) == 3; }()); // @M24 constexpr
// [algorithms]: stable sorting (P2562) and the specialized memory algorithms (P3508, P3369)
static_assert([] { int a[] = {3, 1, 2, 1}; std::stable_sort(a, a + 4); return a[0] == 1 && a[3] == 3; }()); // @M25 constexpr
static_assert([] { int a[] = {3, 1, 2, 1}; std::ranges::stable_sort(a); return a[0] == 1; }()); // @M26 constexpr
static_assert([] { int a[] = {3, 1, 2, 1}; std::stable_partition(a, a + 4, [](int x) { return x < 2; }); return a[0] == 1; }()); // @M27 constexpr
static_assert([] { int a[] = {3, 1, 2, 1}; std::ranges::stable_partition(a, [](int x) { return x < 2; }); return a[0] == 1; }()); // @M28 constexpr
static_assert([] { int a[] = {1, 3, 2, 4}; std::inplace_merge(a, a + 2, a + 4); return a[1] == 2; }()); // @M29 constexpr
static_assert([] { int a[] = {1, 3, 2, 4}; std::ranges::inplace_merge(a, a + 2); return a[1] == 2; }()); // @M30 constexpr
static_assert([] { std::allocator<int> al; int* p = al.allocate(3); std::uninitialized_fill(p, p + 3, 7); bool ok = p[2] == 7; std::destroy(p, p + 3); al.deallocate(p, 3); return ok; }()); // @M31 constexpr
static_assert([] { std::allocator<int> al; int* p = al.allocate(3); int s[3] = {1, 2, 3}; std::ranges::uninitialized_copy(s, std::ranges::subrange(p, p + 3)); bool ok = p[2] == 3; std::ranges::destroy(p, p + 3); al.deallocate(p, 3); return ok; }()); // @M32 constexpr
static_assert([] { std::allocator<int> al; int* p = al.allocate(2); std::uninitialized_value_construct_n(p, 2); bool ok = p[1] == 0; std::destroy_n(p, 2); al.deallocate(p, 2); return ok; }()); // @M33 constexpr
static_assert([] { std::allocator<int> al; int* p = al.allocate(2); int s[2] = {4, 5}; std::uninitialized_move(s, s + 2, p); bool ok = p[1] == 5; std::ranges::destroy_n(p, 2); al.deallocate(p, 2); return ok; }()); // @M34 constexpr
static_assert([] { std::array a{1, 2, 3}; return std::reduce(a.begin(), a.end()) == 6 && std::ranges::fold_left(a, 0, std::plus{}) == 6; }()); // @M35 constexpr
// [strings]: integral to_string / to_wstring ([string.conversions]), basic_string, char_traits
static_assert([] { return std::to_string(-42) == "-42" && std::to_string(7ull) == "7"; }()); // @M36 constexpr
static_assert([] { return std::to_wstring(-42) == L"-42"; }()); // @M37 constexpr
static_assert([] { std::string s = "hello"; s.append_range(std::string_view(" you")); s.resize_and_overwrite(5, [](char*, std::size_t n) { return n; }); return s == "hello" && s.subview(1, 2) == "el"; }()); // @M38 constexpr
static_assert([] { std::string_view v = "hello"; return v.subview(1, 3) == "ell" && v.contains('l'); }()); // @M39 constexpr
static_assert(std::char_traits<char>::compare("ab", "ac", 2) < 0 && std::char_traits<char16_t>::length(u"abc") == 3); // @M40 constexpr
