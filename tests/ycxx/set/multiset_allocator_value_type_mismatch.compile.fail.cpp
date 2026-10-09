// EXPECT-ERROR: error: static assertion failed[^\n]*std::multiset: Allocator::value_type must be Key \(\[container\.alloc\.reqmts\]\)
// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A multiset whose allocator's
// value_type differs from the multiset's value_type is ill-formed.
#include <set>
#include <memory>
#include <functional>
// COUNTERPART: libstdcxx:23_containers/multiset/requirements/explicit_instantiation/3.cc

std::multiset<int, std::less<int>, std::allocator<char>> c;

int main() {}
