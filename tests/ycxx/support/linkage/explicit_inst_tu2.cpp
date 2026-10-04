// Second translation unit of linkage/explicit_instantiation.pass.cpp: the explicit
// instantiation DEFINITIONS ([temp.explicit]/2, /11: every member defined at this point whose
// constraints are satisfied is instantiated).
#include "linkage/explicit_inst.hpp"

template class std::vector<P>;
template class std::deque<P>;
template class std::list<P>;
template class std::forward_list<P>;
template class std::map<P, P>;
template class std::multimap<P, P>;
template class std::set<P>;
template class std::multiset<P>;
template class std::unordered_map<P, P>;
template class std::unordered_multimap<P, P>;
template class std::unordered_set<P>;
template class std::unordered_multiset<P>;
template class std::flat_map<P, P>;
template class std::flat_set<P>;
template class std::inplace_vector<P, 4>;
template class std::array<P, 3>;
template class std::stack<P>;
template class std::queue<P>;
template class std::priority_queue<P>;
template class std::basic_string<char, std::char_traits<char>, Alloc<char>>;
template class std::vector<int, Alloc<int>>;
template class std::optional<P>;
template class std::variant<P, int>;
template class std::tuple<P&, int&>;
template class std::pair<P&, int&>;
template class std::expected<P, int>;
template class std::expected<int, P>;
template class std::unique_ptr<P>;
template class std::unique_ptr<P[]>;
template class std::shared_ptr<P>;
template class std::weak_ptr<P>;
template class std::function<P(P)>;
template class std::span<P>;
template class std::mdspan<P, std::dextents<std::size_t, 2>>;
template class std::atomic<Q>;
template class std::reference_wrapper<P>;

std::vector<P> tu2_vector() { return {P(1), P(2), P(3)}; }
std::map<P, P> tu2_map() { return {{P(1), P(10)}, {P(2), P(20)}}; }
std::basic_string<char, std::char_traits<char>, Alloc<char>> tu2_string() {
  return "a string long enough to need the allocator";
}
