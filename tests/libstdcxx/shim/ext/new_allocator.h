// Test-harness shim (see bits/c++config.h). testsuite_common_types.h includes the five
// extension-allocator headers (ext/new_allocator.h, malloc_allocator.h, mt_allocator.h,
// bitmap_allocator.h, pool_allocator.h) for its allocator_policies typelist (and the vectors,
// lists, deques, strings, maps, sets, ... typelists built on it). Those allocators are libstdc++
// extensions with no standard behaviour to reproduce: they are declared (so the helper parses)
// but never defined, and no test outside the excluded ext/ directory uses those typelists.
#pragma once
namespace __gnu_cxx {
template <class T>
class new_allocator;
} // namespace __gnu_cxx
