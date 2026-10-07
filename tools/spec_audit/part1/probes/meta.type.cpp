// [type.traits], [intseq], [ratio]: the traits added or changed since C++20 (is_implicit_lifetime,
// is_virtual_base_of, reference_constructs/converts_from_temporary, is_scoped_enum,
// is_bounded_array, is_layout_compatible, is_pointer_interconvertible_*, is_reflection,
// is_within_lifetime, common_reference/basic_common_reference), integer_sequence (202511L:
// a tuple-like integer_sequence), ratio (the SI prefixes, ratio_equal_v ...).
// FREESTANDING
#include <type_traits>
#include <ratio>
#include <utility>
#include <cstdint>

struct Agg { int a; };
struct NonTrivDtor { ~NonTrivDtor(); };
struct B {};
struct VD : virtual B {};
struct D : B {};
struct L1 { int a; char b; };
struct L2 { int x; char y; };
struct PI { int a; };
enum class SE : int {};
enum UE {};

static_assert(std::is_implicit_lifetime_v<Agg> && std::is_implicit_lifetime_v<int[3]> && !std::is_implicit_lifetime_v<NonTrivDtor>);
static_assert(std::is_virtual_base_of_v<B, VD> && !std::is_virtual_base_of_v<B, D> && !std::is_virtual_base_of_v<B, B>);
static_assert(std::reference_constructs_from_temporary_v<const int&, long> &&
              !std::reference_constructs_from_temporary_v<const int&, int&>);
static_assert(std::reference_converts_from_temporary_v<const int&, long>);
static_assert(std::is_scoped_enum_v<SE> && !std::is_scoped_enum_v<UE> && !std::is_scoped_enum_v<int>);
static_assert(std::is_bounded_array_v<int[2]> && std::is_unbounded_array_v<int[]>);
static_assert(std::is_layout_compatible_v<L1, L2> && !std::is_layout_compatible_v<int, unsigned>);
static_assert(std::is_pointer_interconvertible_base_of_v<B, D>);
static_assert(!std::is_reflection_v<int> && std::is_same_v<std::is_reflection<int>::value_type, bool>);
static_assert(std::is_same_v<std::remove_cvref_t<const int&>, int> && std::is_same_v<std::type_identity_t<int>, int>);
static_assert(std::is_same_v<std::unwrap_ref_decay_t<std::reference_wrapper<int>>, int&>);
static_assert(std::is_same_v<std::common_reference_t<int&, const int&>, const int&>);
static_assert(std::is_same_v<std::common_reference_t<int&&, int&>, const int&>);
static_assert(std::is_nothrow_convertible_v<int, long> && !std::is_nothrow_convertible_v<int, void*>);
static_assert(std::is_nothrow_invocable_r_v<long, int (*)() noexcept> && !std::is_invocable_r_v<void*, int (*)()>);
consteval bool ce() { return std::is_constant_evaluated(); }
static_assert(ce());
static_assert(std::is_aggregate_v<Agg> && std::has_unique_object_representations_v<int>);
static_assert(std::is_same_v<std::make_signed_t<unsigned char>, signed char> && std::is_same_v<std::make_unsigned_t<SE>, unsigned>);
static_assert(std::is_same_v<std::underlying_type_t<SE>, int>);
template <class T>
concept has_underlying = requires { typename std::underlying_type<T>::type; };
static_assert(!has_underlying<int>);   // SFINAE-friendly since LWG 2396
static_assert(std::is_same_v<std::common_type_t<int, long, short>, long>);
template <class... T>
concept has_common = requires { typename std::common_type<T...>::type; };
static_assert(!has_common<int, B>);
// [intseq]
static_assert(std::is_same_v<std::make_index_sequence<3>, std::index_sequence<0, 1, 2>>);
static_assert(std::is_same_v<std::make_integer_sequence<short, 2>, std::integer_sequence<short, 0, 1>>);
static_assert(std::index_sequence_for<int, long>::size() == 2 && noexcept(std::index_sequence<>::size()));
static_assert(std::tuple_size_v<std::index_sequence<4, 5>> == 2 &&
              std::is_same_v<std::tuple_element_t<1, std::index_sequence<4, 5>>, std::size_t>);
static_assert(get<1>(std::index_sequence<4, 5>()) == 5);
static_assert([] { auto [a, b] = std::index_sequence<4, 5>(); return a + b == 9; }());
// [ratio]
static_assert(std::ratio<2, 4>::num == 1 && std::ratio<2, -4>::den == 2);
static_assert(std::is_same_v<std::ratio_add<std::ratio<1, 2>, std::ratio<1, 3>>, std::ratio<5, 6>>);
static_assert(std::ratio_less_v<std::milli, std::centi> && std::ratio_equal_v<std::kilo, std::ratio<1000>>);
static_assert(std::exa::num == 1000000000000000000 && std::atto::den == 1000000000000000000);
static_assert(std::is_same_v<std::ratio<3, 6>::type, std::ratio<1, 2>>);
