// Test-harness shim (see c++config.h), included by testsuite_containers.h. Its
// iterator_concept_checks call, after `using namespace __gnu_cxx;`,
//   __function_requires<_ForwardIteratorConcept<It>>()           (and _Bidirectional..., _RandomAccess...)
//   __function_requires<_Mutable_ForwardIteratorConcept<It>>()   (likewise)
// libstdc++'s concept-check classes. This is a harness-side equivalent written from the
// Cpp17 iterator requirements ([iterator.cpp17]): each concept is a bool constant, and
// __function_requires static_asserts it, so a container iterator that does not meet the
// requirements fails to compile, as with the original checks. A mutable iterator additionally has
// reference == value_type& and accepts *r = *r ([iterator.requirements.general]/5).
#pragma once
#include <concepts>
#include <iterator>
#include <type_traits>

namespace __gnu_cxx {
namespace __harness_concepts {
template <class X>
using traits = std::iterator_traits<X>;

// [iterator.iterators] Cpp17Iterator and [input.iterators] Cpp17InputIterator.
template <class X>
concept cpp17_input = std::is_copy_constructible_v<X> && std::is_copy_assignable_v<X> &&
                      std::is_destructible_v<X> && std::is_swappable_v<X&> &&
                      std::signed_integral<typename traits<X>::difference_type> &&
                      std::derived_from<typename traits<X>::iterator_category, std::input_iterator_tag> &&
                      requires(X r, const X a, const X b) {
                        *r;
                        { ++r } -> std::same_as<X&>;
                        { a == b } -> std::convertible_to<bool>;
                        { a != b } -> std::convertible_to<bool>;
                        { *a } -> std::convertible_to<typename traits<X>::value_type>;
                        (void)r++;
                        { *r++ } -> std::convertible_to<typename traits<X>::value_type>;
                      };

// [forward.iterators] Cpp17ForwardIterator.
template <class X>
concept cpp17_forward =
    cpp17_input<X> && std::is_default_constructible_v<X> &&
    std::derived_from<typename traits<X>::iterator_category, std::forward_iterator_tag> &&
    std::is_reference_v<typename traits<X>::reference> &&
    std::same_as<std::remove_cvref_t<typename traits<X>::reference>, typename traits<X>::value_type> &&
    requires(X r) {
      { r++ } -> std::convertible_to<const X&>;
      { *r++ } -> std::same_as<typename traits<X>::reference>;
    };

// [bidirectional.iterators] Cpp17BidirectionalIterator.
template <class X>
concept cpp17_bidirectional =
    cpp17_forward<X> &&
    std::derived_from<typename traits<X>::iterator_category, std::bidirectional_iterator_tag> &&
    requires(X r) {
      { --r } -> std::same_as<X&>;
      { r-- } -> std::convertible_to<const X&>;
      { *r-- } -> std::same_as<typename traits<X>::reference>;
    };

// [random.access.iterators] Cpp17RandomAccessIterator.
template <class X>
concept cpp17_random_access =
    cpp17_bidirectional<X> &&
    std::derived_from<typename traits<X>::iterator_category, std::random_access_iterator_tag> &&
    requires(X r, const X a, const X b, typename traits<X>::difference_type n) {
      { r += n } -> std::same_as<X&>;
      { a + n } -> std::same_as<X>;
      { n + a } -> std::same_as<X>;
      { r -= n } -> std::same_as<X&>;
      { a - n } -> std::same_as<X>;
      { b - a } -> std::same_as<typename traits<X>::difference_type>;
      { a[n] } -> std::convertible_to<typename traits<X>::reference>;
      { a < b } -> std::convertible_to<bool>;
      { a > b } -> std::convertible_to<bool>;
      { a >= b } -> std::convertible_to<bool>;
      { a <= b } -> std::convertible_to<bool>;
    };

// A mutable iterator ([iterator.requirements.general]/5).
template <class X>
concept cpp17_mutable = std::same_as<typename traits<X>::reference, typename traits<X>::value_type&> &&
                        requires(X r) { *r = *r; };
} // namespace __harness_concepts

template <class Concept>
constexpr void __function_requires() {
  static_assert(Concept::value, "iterator does not meet the Cpp17 iterator requirements");
}

template <class X>
struct _ForwardIteratorConcept : std::bool_constant<__harness_concepts::cpp17_forward<X>> {};
template <class X>
struct _BidirectionalIteratorConcept : std::bool_constant<__harness_concepts::cpp17_bidirectional<X>> {};
template <class X>
struct _RandomAccessIteratorConcept : std::bool_constant<__harness_concepts::cpp17_random_access<X>> {};
template <class X>
struct _Mutable_ForwardIteratorConcept
    : std::bool_constant<__harness_concepts::cpp17_forward<X> && __harness_concepts::cpp17_mutable<X>> {};
template <class X>
struct _Mutable_BidirectionalIteratorConcept
    : std::bool_constant<__harness_concepts::cpp17_bidirectional<X> && __harness_concepts::cpp17_mutable<X>> {};
template <class X>
struct _Mutable_RandomAccessIteratorConcept
    : std::bool_constant<__harness_concepts::cpp17_random_access<X> && __harness_concepts::cpp17_mutable<X>> {};
} // namespace __gnu_cxx
