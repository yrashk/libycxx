// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A unordered_multiset whose allocator's
// value_type differs from the unordered_multiset's value_type is ill-formed.
#include <unordered_set>
#include <memory>
#include <functional>
// COUNTERPART: libstdcxx:23_containers/unordered_multiset/requirements/explicit_instantiation/3.cc

std::unordered_multiset<int, std::hash<int>, std::equal_to<int>, std::allocator<unsigned>> c;

int main() {}
