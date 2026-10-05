// Test-harness shim (see bits/c++config.h), included by testsuite_allocator.h. Only its
// CustomPointerAlloc uses <ext/pointer.h>: its pointer type is libstdc++'s fancy pointer
// __gnu_cxx::_Pointer_adapter<_Std_pointer_impl<T>>, an extension whose behaviour the standard
// does not define. The templates are deliberately empty: complete, because GCC rejects the
// non-dependent parameter type `const_void_pointer` of CustomPointerAlloc::allocate even in the
// uninstantiated helper if it is incomplete, but with no members, so any use of
// CustomPointerAlloc fails to compile. Tests that use it stay skipped (tests/libstdcxx/skip.txt).
#pragma once

namespace __gnu_cxx {
template <class T>
class _Std_pointer_impl {};
template <class Storage>
class _Pointer_adapter {};
} // namespace __gnu_cxx
