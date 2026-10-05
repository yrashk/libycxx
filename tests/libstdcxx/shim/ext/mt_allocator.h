// Test-harness shim: declared only; see ext/new_allocator.h. testsuite_common_types.h names
// __mt_alloc<T, __common_pool_policy<__pool, Thread>>.
#pragma once
namespace __gnu_cxx {
template <bool Thread>
class __pool;
template <template <bool> class PoolT, bool Thread>
struct __common_pool_policy;
template <class T, class Policy>
class __mt_alloc;
} // namespace __gnu_cxx
