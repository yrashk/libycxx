// Shared header of linkage/explicit_instantiation.pass.cpp and
// support/linkage/explicit_inst_tu2.cpp: a program-defined value type and the explicit
// instantiation DECLARATIONS ([temp.explicit]/2) of standard class templates for it; the
// definitions are in explicit_inst_tu2.cpp. [namespace.std]/5: allowed, since each declaration
// depends on a program-defined type and the type meets the requirements. (tuple<P, int> and
// pair<P, P> are not instantiated: their swap(const tuple&) const / swap(const pair&) const
// Mandate is_swappable_v<const P> ([tuple.swap]/2.2, [pairs.pair]/51.2), so an explicit
// instantiation definition, which instantiates those members, would be ill-formed; tuple<P&,
// int&> and pair<P&, int&> are used instead.)
#pragma once
#include <array>
#include <atomic>
#include <compare>
#include <cstddef>
#include <deque>
#include <expected>
#include <flat_map>
#include <flat_set>
#include <forward_list>
#include <functional>
#include <inplace_vector>
#include <list>
#include <map>
#include <mdspan>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

struct P {
  int v = 0;
  std::string s;
  P() = default;
  P(int x) : v(x), s(std::to_string(x)) {}
  friend bool operator==(const P&, const P&) = default;
  friend auto operator<=>(const P&, const P&) = default;
};
template <>
struct std::hash<P> {
  std::size_t operator()(const P& p) const noexcept { return std::hash<int>{}(p.v); }
};
struct Q {  // trivially copyable, for atomic
  int a, b;
};

template <class T>
struct Alloc {  // a program-defined allocator
  using value_type = T;
  Alloc() = default;
  template <class U>
  Alloc(const Alloc<U>&) noexcept {}
  T* allocate(std::size_t n) { return std::allocator<T>().allocate(n); }
  void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
  friend bool operator==(const Alloc&, const Alloc&) = default;
};

extern template class std::vector<P>;
extern template class std::deque<P>;
extern template class std::list<P>;
extern template class std::forward_list<P>;
extern template class std::map<P, P>;
extern template class std::multimap<P, P>;
extern template class std::set<P>;
extern template class std::multiset<P>;
extern template class std::unordered_map<P, P>;
extern template class std::unordered_multimap<P, P>;
extern template class std::unordered_set<P>;
extern template class std::unordered_multiset<P>;
extern template class std::flat_map<P, P>;
extern template class std::flat_set<P>;
extern template class std::inplace_vector<P, 4>;
extern template class std::array<P, 3>;
extern template class std::stack<P>;
extern template class std::queue<P>;
extern template class std::priority_queue<P>;
extern template class std::basic_string<char, std::char_traits<char>, Alloc<char>>;
extern template class std::vector<int, Alloc<int>>;
extern template class std::optional<P>;
extern template class std::variant<P, int>;
extern template class std::tuple<P&, int&>;
extern template class std::pair<P&, int&>;
extern template class std::expected<P, int>;
extern template class std::expected<int, P>;
extern template class std::unique_ptr<P>;
extern template class std::unique_ptr<P[]>;
extern template class std::shared_ptr<P>;
extern template class std::weak_ptr<P>;
extern template class std::function<P(P)>;
extern template class std::span<P>;
extern template class std::mdspan<P, std::dextents<std::size_t, 2>>;
extern template class std::atomic<Q>;
extern template class std::reference_wrapper<P>;

// Implemented in explicit_inst_tu2.cpp.
std::vector<P> tu2_vector();
std::map<P, P> tu2_map();
std::basic_string<char, std::char_traits<char>, Alloc<char>> tu2_string();
