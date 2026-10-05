// Test-harness shim (see bits/c++config.h), included by testsuite_common_types.h, which builds
// its type lists (integral_types, atomic_integrals, atomics_tl, limits_tl, ...) at namespace
// scope with __gnu_cxx::typelist's node, null_type, transform, append and the
// _GLIBCXX_TYPELIST_CHAINn macros. This is a minimal harness-side type list with those names, so
// that the helper's non-template declarations compile. libstdc++'s typelist library itself
// (apply, apply_generator, ...) is an extension and is not reproduced: tests that call it name
// __gnu_cxx and stay skipped (libstdcxx_format.py).
#pragma once

namespace __gnu_cxx::typelist {
struct null_type {};

// A list is node<Root>; Root is null_type or chain<Head, Tail-root>.
template <class Head, class Tail>
struct chain {};

template <class Root>
struct node {
  using root = Root;
};

template <class... Ts>
struct __harness_make_chain {
  using type = null_type;
};
template <class T, class... Ts>
struct __harness_make_chain<T, Ts...> {
  using type = chain<T, typename __harness_make_chain<Ts...>::type>;
};

// transform<node<R>, F>::type: the list of F<T>::type for each T of the list.
template <class Root, template <class> class F>
struct __harness_transform {
  using type = null_type;
};
template <class Head, class Tail, template <class> class F>
struct __harness_transform<chain<Head, Tail>, F> {
  using type = chain<typename F<Head>::type, typename __harness_transform<Tail, F>::type>;
};
template <class List, template <class> class F>
struct transform;
template <class Root, template <class> class F>
struct transform<node<Root>, F> {
  using type = node<typename __harness_transform<Root, F>::type>;
};

// append<node<R1>, node<R2>>::type: the elements of the first list, then those of the second.
template <class Root1, class Root2>
struct __harness_append {
  using type = Root2;
};
template <class Head, class Tail, class Root2>
struct __harness_append<chain<Head, Tail>, Root2> {
  using type = chain<Head, typename __harness_append<Tail, Root2>::type>;
};
template <class List1, class List2>
struct append;
template <class Root1, class Root2>
struct append<node<Root1>, node<Root2>> {
  using type = node<typename __harness_append<Root1, Root2>::type>;
};
} // namespace __gnu_cxx::typelist

#define _GLIBCXX_TYPELIST_CHAIN(...) typename ::__gnu_cxx::typelist::__harness_make_chain<__VA_ARGS__>::type
#define _GLIBCXX_TYPELIST_CHAIN1(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN2(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN3(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN4(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN5(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN6(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN7(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN8(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN9(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN10(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN11(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN12(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN13(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN14(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
#define _GLIBCXX_TYPELIST_CHAIN15(...) _GLIBCXX_TYPELIST_CHAIN(__VA_ARGS__)
