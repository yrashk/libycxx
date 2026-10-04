// [intseq.intseq]: template<class T, T... I> struct integer_sequence { using value_type = T;
// static constexpr size_t size() noexcept { return sizeof...(I); } }; "Mandates: T is an
// integer type." [intseq.make]: make_integer_sequence<T, N> is integer_sequence<T, 0, 1, ...,
// N - 1>. [utility.syn]: index_sequence, make_index_sequence, index_sequence_for.
#include <utility>
#include <cstddef>
#include <type_traits>

static_assert(std::is_same_v<std::integer_sequence<int, 1, 2>::value_type, int>);
static_assert(std::integer_sequence<int, 1, 2, 3>::size() == 3);
static_assert(std::integer_sequence<char>::size() == 0);
static_assert(noexcept(std::integer_sequence<int, 1>::size()));
static_assert(std::is_same_v<decltype(std::integer_sequence<int>::size()), std::size_t>);
static_assert(std::is_empty_v<std::integer_sequence<int, 1, 2>>);

static_assert(std::is_same_v<std::make_integer_sequence<int, 0>, std::integer_sequence<int>>);
static_assert(std::is_same_v<std::make_integer_sequence<int, 1>, std::integer_sequence<int, 0>>);
static_assert(std::is_same_v<std::make_integer_sequence<int, 4>, std::integer_sequence<int, 0, 1, 2, 3>>);
static_assert(std::is_same_v<std::make_integer_sequence<unsigned char, 3>,
                             std::integer_sequence<unsigned char, 0, 1, 2>>);
static_assert(std::is_same_v<std::make_integer_sequence<long long, 2>, std::integer_sequence<long long, 0, 1>>);
static_assert(std::is_same_v<std::make_integer_sequence<bool, 1>, std::integer_sequence<bool, false>>);
static_assert(std::is_same_v<std::make_integer_sequence<char, 2>, std::integer_sequence<char, 0, 1>>);
static_assert(std::make_integer_sequence<short, 1000>::size() == 1000);

static_assert(std::is_same_v<std::index_sequence<1, 2>, std::integer_sequence<std::size_t, 1, 2>>);
static_assert(std::is_same_v<std::make_index_sequence<3>, std::index_sequence<0, 1, 2>>);
static_assert(std::is_same_v<std::index_sequence_for<int, char, void>, std::index_sequence<0, 1, 2>>);
static_assert(std::is_same_v<std::index_sequence_for<>, std::index_sequence<>>);

// usable for pack expansion via deduction
template <class T, T... I>
constexpr T sum(std::integer_sequence<T, I...>) {
  return (T(0) + ... + I);
}
static_assert(sum(std::make_integer_sequence<int, 5>{}) == 10);
static_assert(sum(std::integer_sequence<long, -3, 7>{}) == 4);

// the values are kept as given (not sorted or deduplicated)
static_assert(std::integer_sequence<int, 5, -1, 5>::size() == 3);
static_assert(std::is_same_v<std::integer_sequence<wchar_t, L'a'>::value_type, wchar_t>);
