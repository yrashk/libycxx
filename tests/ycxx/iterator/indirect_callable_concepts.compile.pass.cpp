// [indirectcallable.traits]/1: indirect-value-t<T> is invoke_result_t<Proj&, indirect-value-t<I>>
// for T = projected<I, Proj>, otherwise iter_value_t<T>&.
// [indirectcallable.indirectinvocable]/1: indirectly_unary_invocable,
// indirectly_regular_unary_invocable, indirect_unary_predicate, indirect_binary_predicate,
// indirect_equivalence_relation and indirect_strict_weak_order require the callable to accept
// both indirect-value-t<I> and iter_reference_t<I> (and every mix of them for two iterators),
// and copy_constructible<F>.
// [projected]/1: projected<I, Proj>::value_type is remove_cvref_t<indirect_result_t<Proj&, I>>,
// operator* returns indirect_result_t<Proj&, I>, difference_type is present only if I models
// weakly_incrementable; projected requires indirectly_readable<I> and
// indirectly_regular_unary_invocable<Proj, I>.
// [indirectcallable.indirectresult] (in [iterator.synopsis]): indirect_result_t<F, Is...> is
// invoke_result_t<F, iter_reference_t<Is>...>, requiring invocable<F, iter_reference_t<Is>...>.
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

using It = std::vector<int>::iterator;
using CIt = std::vector<int>::const_iterator;
using ZIt = std::ranges::iterator_t<decltype(std::views::zip(std::declval<std::vector<int>&>(), std::declval<std::vector<int>&>()))>;
using Val = std::tuple<int, int>;
using Ref = std::tuple<int&, int&>;
template <class I, class P>
concept has_projected = requires { typename std::projected<I, P>; };
template <class F, class I>
concept has_indirect_result = requires { typename std::indirect_result_t<F, I>; };
template <class T>
concept has_difference_type = requires { typename T::difference_type; };

static_assert(std::is_same_v<std::iter_value_t<ZIt>, Val> && std::is_same_v<std::iter_reference_t<ZIt>, Ref>);

// accepts only the reference type of the zip iterator, not its value type
struct ref_only {
  int operator()(Ref) const;
  int operator()(Val&) const = delete;
};
// accepts both, with different results
struct both {
  int operator()(Ref) const;
  long operator()(Val&) const;
};
// a predicate accepting exactly int
struct int_only {
  bool operator()(std::same_as<int> auto) const;
};
struct int_or_long {
  bool operator()(int) const;
  bool operator()(long) const;
};

// (1.2) indirect-value-t<ZIt> is tuple<int, int>&: a callable must accept it
static_assert(std::invocable<ref_only&, Ref>);
static_assert(!std::indirectly_unary_invocable<ref_only, ZIt>);
static_assert(!std::indirectly_regular_unary_invocable<ref_only, ZIt>);
static_assert(std::indirectly_unary_invocable<both, ZIt>);
static_assert(!has_projected<ZIt, ref_only>);

// projected<ZIt, both>: value_type from the reference's result (int); indirect-value-t of the
// projection is the result of projecting the value (long, (1.1)), so a predicate must take both
using PZ = std::projected<ZIt, both>;
static_assert(std::is_same_v<std::iter_value_t<PZ>, int>);
static_assert(std::is_same_v<std::iter_reference_t<PZ>, int>);
static_assert(std::is_same_v<std::iter_difference_t<PZ>, std::iter_difference_t<ZIt>>);
static_assert(std::indirectly_readable<PZ>);
static_assert(!std::indirect_unary_predicate<int_only, PZ>);
static_assert(std::indirect_unary_predicate<int_or_long, PZ>);
static_assert(std::indirect_unary_predicate<int_only, std::projected<It, std::identity>>);

// projected's members
using PI = std::projected<It, std::negate<>>;
static_assert(std::is_same_v<std::iter_value_t<PI>, int>);
static_assert(std::is_same_v<decltype(*std::declval<PI>()), int>);
static_assert(std::is_same_v<std::iter_difference_t<PI>, std::ptrdiff_t>);
struct Member { int m; const long& f() const; };
using PM = std::projected<std::vector<Member>::iterator, const long& (Member::*)() const>;
static_assert(std::is_same_v<std::iter_reference_t<PM>, const long&> && std::is_same_v<std::iter_value_t<PM>, long>);
using PD = std::projected<std::vector<Member>::const_iterator, int Member::*>;
static_assert(std::is_same_v<std::iter_reference_t<PD>, const int&>);
// difference_type only when I is weakly_incrementable
struct readable_only {
  using value_type = int;
  int& operator*() const;
};
static_assert(std::indirectly_readable<readable_only> && !std::weakly_incrementable<readable_only>);
using PR = std::projected<readable_only, std::identity>;
static_assert(!has_difference_type<PR> && has_difference_type<PI>);
static_assert(std::indirectly_readable<PR>);
// projected requires a regular invocable projection of an indirectly_readable type
static_assert(!has_projected<int, std::identity>);
static_assert(!has_projected<It, int (*)(std::string)>);
static_assert(has_projected<It, int (*)(long)>);

// indirect_result_t
static_assert(std::is_same_v<std::indirect_result_t<std::negate<>, It>, int>);
static_assert(std::is_same_v<std::indirect_result_t<std::plus<>, It, CIt>, int>);
static_assert(std::is_same_v<std::indirect_result_t<int Member::*, std::vector<Member>::iterator>, int&>);
static_assert(!has_indirect_result<std::negate<>, int> && !has_indirect_result<std::negate<>, std::vector<std::string>::iterator>);

// copy_constructible<F> is required
struct move_only_pred {
  move_only_pred(move_only_pred&&) = default;
  bool operator()(int) const;
};
static_assert(!std::indirect_unary_predicate<move_only_pred, It>);
static_assert(std::indirect_unary_predicate<int_only, It>);

// binary forms: every combination of value and reference
struct binary_ref_only {
  bool operator()(Ref, Ref) const;
  bool operator()(Val&, Ref) const = delete;
  bool operator()(Ref, Val&) const;
  bool operator()(Val&, Val&) const;
};
struct binary_all {
  bool operator()(Ref, Ref) const;
  bool operator()(Val&, Ref) const;
  bool operator()(Ref, Val&) const;
  bool operator()(Val&, Val&) const;
};
static_assert(!std::indirect_binary_predicate<binary_ref_only, ZIt, ZIt>);
static_assert(std::indirect_binary_predicate<binary_all, ZIt, ZIt>);
static_assert(std::indirect_equivalence_relation<std::ranges::equal_to, It, CIt>);
static_assert(std::indirect_strict_weak_order<std::ranges::less, It>);
static_assert(!std::indirect_strict_weak_order<std::ranges::less, std::vector<std::vector<int>*>::iterator, It>);
static_assert(std::indirect_strict_weak_order<std::ranges::less, ZIt>);

int main() {}
