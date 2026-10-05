// Test-harness shim (see bits/c++config.h), included by testsuite_allocator.h. Its
// tracker_allocator, uneq_allocator and propagating_allocator use
// __gnu_cxx::__alloc_traits<Alloc> only for what std::allocator_traits<Alloc> provides
// ([allocator.traits]): pointer, size_type, allocate, construct, destroy, deallocate, and
// rebind<U>::other, which is allocator_traits<Alloc>::rebind_alloc<U>. This harness-side
// equivalent provides exactly that; libstdc++'s extras are not reproduced.
#pragma once
#include <memory>

namespace __gnu_cxx {
template <class Alloc>
struct __alloc_traits : std::allocator_traits<Alloc> {
  template <class U>
  struct rebind {
    using other = typename std::allocator_traits<Alloc>::template rebind_alloc<U>;
  };
};
} // namespace __gnu_cxx
