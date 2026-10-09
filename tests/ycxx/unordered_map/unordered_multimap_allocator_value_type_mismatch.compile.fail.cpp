// EXPECT-ERROR: error: static assertion failed[^\n]*std::unordered_multimap: Allocator::value_type must be pair<const Key, T> \(\[container\.alloc\.reqmts\]\)
// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A unordered_multimap whose allocator's
// value_type differs from the unordered_multimap's value_type is ill-formed.
#include <unordered_map>
#include <memory>
#include <functional>
// COUNTERPART: libstdcxx:23_containers/unordered_multimap/requirements/explicit_instantiation/3.cc

std::unordered_multimap<int, double, std::hash<int>, std::equal_to<int>, std::allocator<char>> c;

int main() {}
