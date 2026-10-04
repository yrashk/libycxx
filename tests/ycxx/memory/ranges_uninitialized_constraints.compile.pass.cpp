// [special.mem.concepts]/1: nothrow-input-iterator requires input_iterator<I>,
// is_lvalue_reference_v<iter_reference_t<I>> and same_as<remove_cvref_t<iter_reference_t<I>>,
// iter_value_t<I>>; the range forms return borrowed_iterator_t<R> (ranges::dangling for a
// non-borrowed rvalue). [uninitialized.construct.default]: requires
// default_initializable<iter_value_t<I>>. [uninitialized.copy]: requires
// constructible_from<iter_value_t<O>, iter_reference_t<I>>; the input only needs
// input_iterator. [uninitialized.move]: constructible_from<iter_value_t<O>,
// iter_rvalue_reference_t<I>>. [uninitialized.fill]: constructible_from<iter_value_t<I>,
// const T&>.
#include <memory>
#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <type_traits>

struct NoDefault {
  NoDefault(int);
};
struct MoveOnly {
  MoveOnly(MoveOnly&&);
};
// a forward iterator whose reference is a prvalue: not a nothrow-input-iterator
struct PrvalueIt {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  int* p;
  int operator*() const;
  PrvalueIt& operator++();
  PrvalueIt operator++(int);
  bool operator==(const PrvalueIt&) const;
};
static_assert(std::forward_iterator<PrvalueIt>);
// a forward iterator whose reference type differs from its value type
struct ConstRefLongIt {
  using value_type = long;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  const int& operator*() const;
  ConstRefLongIt& operator++();
  ConstRefLongIt operator++(int);
  bool operator==(const ConstRefLongIt&) const;
};
static_assert(std::forward_iterator<ConstRefLongIt>);

namespace r = std::ranges;
using DefCon = decltype(r::uninitialized_default_construct);
using ValCon = decltype(r::uninitialized_value_construct);
using Copy = decltype(r::uninitialized_copy);
using Move = decltype(r::uninitialized_move);
using Fill = decltype(r::uninitialized_fill);
using Destroy = decltype(r::destroy);

static_assert(std::is_invocable_v<DefCon, int*, int*>);
static_assert(!std::is_invocable_v<DefCon, NoDefault*, NoDefault*>);
static_assert(!std::is_invocable_v<DefCon, PrvalueIt, PrvalueIt>);
static_assert(!std::is_invocable_v<DefCon, ConstRefLongIt, ConstRefLongIt>);
static_assert(!std::is_invocable_v<ValCon, NoDefault*, NoDefault*>);
static_assert(std::is_invocable_v<ValCon, std::span<int>>);

// input side: any input_iterator; output side: nothrow-forward-iterator
static_assert(std::is_invocable_v<Copy, PrvalueIt, PrvalueIt, long*, long*>);
static_assert(!std::is_invocable_v<Copy, int*, int*, PrvalueIt, PrvalueIt>);
static_assert(!std::is_invocable_v<Copy, int*, int*, NoDefault**, NoDefault**>);
static_assert(std::is_invocable_v<Copy, int*, int*, NoDefault*, NoDefault*>);
static_assert(!std::is_invocable_v<Copy, MoveOnly*, MoveOnly*, MoveOnly*, MoveOnly*>);
static_assert(std::is_invocable_v<Move, MoveOnly*, MoveOnly*, MoveOnly*, MoveOnly*>);
static_assert(!std::is_invocable_v<Move, int*, int*, PrvalueIt, PrvalueIt>);
static_assert(std::is_invocable_v<Fill, NoDefault*, NoDefault*, int>);
static_assert(!std::is_invocable_v<Fill, NoDefault*, NoDefault*, int*>);
static_assert(!std::is_invocable_v<Fill, PrvalueIt, PrvalueIt, int>);
static_assert(!std::is_invocable_v<Destroy, PrvalueIt, PrvalueIt>);

// borrowed vs dangling results
static_assert(std::is_same_v<std::invoke_result_t<DefCon, std::span<int>>, std::span<int>::iterator>);
static_assert(std::is_same_v<std::invoke_result_t<DefCon, std::array<int, 2>>, std::ranges::dangling>);
static_assert(std::is_same_v<std::invoke_result_t<ValCon, std::array<int, 2>>, std::ranges::dangling>);
static_assert(std::is_same_v<std::invoke_result_t<Fill, std::array<int, 2>, int>, std::ranges::dangling>);
static_assert(std::is_same_v<std::invoke_result_t<Destroy, std::array<int, 2>>, std::ranges::dangling>);
static_assert(std::is_same_v<std::invoke_result_t<DefCon, std::array<int, 2>&>, std::array<int, 2>::iterator>);
using CR = std::invoke_result_t<Copy, std::array<int, 2>, std::span<long>>;
static_assert(std::is_same_v<CR, std::ranges::uninitialized_copy_result<std::ranges::dangling, std::span<long>::iterator>>);
using MR = std::invoke_result_t<Move, std::span<int>, std::array<long, 2>>;
static_assert(std::is_same_v<MR, std::ranges::uninitialized_move_result<std::span<int>::iterator, std::ranges::dangling>>);
