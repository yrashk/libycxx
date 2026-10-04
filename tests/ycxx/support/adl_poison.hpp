// ADL-robustness helpers for libycxx's own suite. Written from [contents]/3 ("Whenever an
// unqualified name other than swap, make_error_code, make_error_condition, from_stream, or
// submdspan_mapping is used in the specification of a declaration D in [library] ... its
// meaning is established as-if by performing unqualified name lookup in the context of D"
// -- so argument-dependent lookup must not find a user's function of the same name), and from
// the iterator and value requirements ([iterator.requirements], [utility.arg.requirements]),
// which require neither a comma operator nor a unary & on iterators or values.
// Independent of every other test suite.
#pragma once
#include <compare>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace adl_poison_detail {
template <class...>
inline constexpr bool always_false = false;
}

// Unconstrained, perfectly-forwarding function templates named like functions a library could
// call unqualified (not swap, iter_swap or iter_move, which are customization points found
// by ADL on purpose: [swappable.requirements], [iterator.cust.swap], [iterator.cust.move]).
// They beat every std:: candidate in overload resolution, and instantiating
// one fails to compile, so a test using these types fails to build if the library finds one.
#define YCXX_ADL_POISON_ONE(name)                                                        \
  template <class... Args>                                                               \
  constexpr int name(Args&&...) {                                                        \
    static_assert(::adl_poison_detail::always_false<Args...>, "found by ADL: " #name);   \
    return 0;                                                                            \
  }
#define YCXX_ADL_POISON_ALL                                                              \
  YCXX_ADL_POISON_ONE(move)                                                              \
  YCXX_ADL_POISON_ONE(move_backward)                                                     \
  YCXX_ADL_POISON_ONE(copy)                                                              \
  YCXX_ADL_POISON_ONE(copy_backward)                                                     \
  YCXX_ADL_POISON_ONE(copy_n)                                                            \
  YCXX_ADL_POISON_ONE(forward)                                                           \
  YCXX_ADL_POISON_ONE(addressof)                                                         \
  YCXX_ADL_POISON_ONE(advance)                                                           \
  YCXX_ADL_POISON_ONE(distance)                                                          \
  YCXX_ADL_POISON_ONE(next)                                                              \
  YCXX_ADL_POISON_ONE(prev)                                                              \
  YCXX_ADL_POISON_ONE(min)                                                               \
  YCXX_ADL_POISON_ONE(max)                                                               \
  YCXX_ADL_POISON_ONE(find)                                                              \
  YCXX_ADL_POISON_ONE(find_if)                                                           \
  YCXX_ADL_POISON_ONE(fill)                                                              \
  YCXX_ADL_POISON_ONE(fill_n)                                                            \
  YCXX_ADL_POISON_ONE(sort)                                                              \
  YCXX_ADL_POISON_ONE(reverse)                                                           \
  YCXX_ADL_POISON_ONE(rotate)                                                            \
  YCXX_ADL_POISON_ONE(lower_bound)                                                       \
  YCXX_ADL_POISON_ONE(upper_bound)                                                       \
  YCXX_ADL_POISON_ONE(merge)                                                             \
  YCXX_ADL_POISON_ONE(make_heap)                                                         \
  YCXX_ADL_POISON_ONE(push_heap)                                                         \
  YCXX_ADL_POISON_ONE(pop_heap)                                                          \
  YCXX_ADL_POISON_ONE(sort_heap)                                                         \
  YCXX_ADL_POISON_ONE(partition)                                                         \
  YCXX_ADL_POISON_ONE(invoke)                                                            \
  YCXX_ADL_POISON_ONE(construct_at)                                                      \
  YCXX_ADL_POISON_ONE(destroy_at)                                                        \
  YCXX_ADL_POISON_ONE(destroy)                                                           \
  YCXX_ADL_POISON_ONE(to_address)                                                        \
  YCXX_ADL_POISON_ONE(accumulate)

namespace evil {
YCXX_ADL_POISON_ALL

// Value type with a deleted unary & and a deleted comma operator. Ordered and equality
// comparable through hidden friends. Swappable through std::swap (not poisoned: swap is a
// customization point, [swappable.requirements]).
struct Val {
  int v = 0;
  constexpr Val() = default;
  constexpr Val(int x) : v(x) {}
  void operator&() const = delete;
  template <class U>
  void operator,(U&&) const = delete;
  friend constexpr bool operator==(const Val&, const Val&) = default;
  friend constexpr auto operator<=>(const Val&, const Val&) = default;
  friend constexpr Val operator+(const Val& a, const Val& b) { return Val(a.v + b.v); }
  friend constexpr Val operator-(const Val& a, const Val& b) { return Val(a.v - b.v); }
  friend constexpr Val operator*(const Val& a, const Val& b) { return Val(a.v * b.v); }
  constexpr Val& operator++() {
    ++v;
    return *this;
  }
};

// Instantiating Holder<Incomplete> is ill-formed. Pointers Holder<Incomplete>* are fine as
// long as nothing performs argument-dependent lookup on them that needs the class complete.
template <class T>
struct Holder {
  T t;
};
struct Incomplete;

// Random-access (Cpp17RandomAccessIterator and random_access_iterator) iterator over a
// T array, with a deleted comma operator and a deleted unary &.
template <class T>
struct Iter {
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::random_access_iterator_tag;
  using value_type = std::remove_cv_t<T>;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  T* p = nullptr;
  constexpr Iter() = default;
  constexpr explicit Iter(T* q) : p(q) {}
  constexpr operator Iter<const T>() const requires(!std::is_const_v<T>) { return Iter<const T>(p); }
  constexpr T& operator*() const { return *p; }
  constexpr T& operator[](difference_type n) const { return p[n]; }
  constexpr Iter& operator++() {
    ++p;
    return *this;
  }
  constexpr Iter operator++(int) {
    Iter t = *this;
    ++p;
    return t;
  }
  constexpr Iter& operator--() {
    --p;
    return *this;
  }
  constexpr Iter operator--(int) {
    Iter t = *this;
    --p;
    return t;
  }
  constexpr Iter& operator+=(difference_type n) {
    p += n;
    return *this;
  }
  constexpr Iter& operator-=(difference_type n) {
    p -= n;
    return *this;
  }
  friend constexpr Iter operator+(Iter i, difference_type n) { return Iter(i.p + n); }
  friend constexpr Iter operator+(difference_type n, Iter i) { return Iter(i.p + n); }
  friend constexpr Iter operator-(Iter i, difference_type n) { return Iter(i.p - n); }
  friend constexpr difference_type operator-(Iter a, Iter b) { return a.p - b.p; }
  friend constexpr bool operator==(Iter a, Iter b) { return a.p == b.p; }
  friend constexpr auto operator<=>(Iter a, Iter b) { return a.p <=> b.p; }
  void operator&() const = delete;
  template <class U>
  void operator,(U&&) const = delete;
};
}  // namespace evil

// The same poisoned names in the global namespace, with a value type that lives there.
YCXX_ADL_POISON_ALL

struct GVal {
  int v = 0;
  constexpr GVal() = default;
  constexpr GVal(int x) : v(x) {}
  friend constexpr bool operator==(const GVal&, const GVal&) = default;
  friend constexpr auto operator<=>(const GVal&, const GVal&) = default;
};
