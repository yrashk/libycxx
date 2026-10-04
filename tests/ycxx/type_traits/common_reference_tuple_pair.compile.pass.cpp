// [meta.trans.other]/6 common_reference; [tuple.common.ref] basic_common_reference and
// common_type for tuple-like types where one is a tuple; [utility.syn] basic_common_reference
// and common_type specializations for pair.
#include <tuple>
#include <type_traits>
#include <utility>

template <class... T> concept HasCommonRef = requires { typename std::common_reference<T...>::type; };
template <class... T> concept HasCommonType = requires { typename std::common_type<T...>::type; };

// tuple vs tuple: element-wise common_reference_t<TQual<Ti>, UQual<Ui>>.
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int&>, std::tuple<const int&>>,
                             std::tuple<const int&>>);
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int&, long>, std::tuple<const int&, long&>>,
                             std::tuple<const int&, long>>);
static_assert(std::is_same_v<std::common_reference_t<std::tuple<>, std::tuple<>>, std::tuple<>>);
// Reference-qualified tuples: TQual/UQual apply to the elements ([meta.trans.other] XREF).
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int>&, std::tuple<int>&>, std::tuple<int>&>);
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int>&, const std::tuple<int>&>, const std::tuple<int>&>);
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int>&, std::tuple<long>&>, std::tuple<long>>);
// Size mismatch: no member type.
static_assert(!HasCommonRef<std::tuple<int&>, std::tuple<int&, int&>>);
static_assert(!HasCommonType<std::tuple<int>, std::tuple<int, int>>);

// pair vs pair.
static_assert(std::is_same_v<std::common_reference_t<std::pair<int&, long&>, std::pair<const int&, long>>,
                             std::pair<const int&, long>>);
static_assert(std::is_same_v<std::common_reference_t<std::pair<int&, int&>, std::pair<int&, int&>>,
                             std::pair<int&, int&>>);

// tuple vs pair (one of them is a tuple): result is a tuple.
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int&, int&>, std::pair<const int&, int>>,
                             std::tuple<const int&, int>>);
static_assert(std::is_same_v<std::common_reference_t<std::pair<int&, int&>, std::tuple<const int&, int>>,
                             std::tuple<const int&, int>>);
static_assert(!HasCommonRef<std::tuple<int&>, std::pair<int&, int&>>);

// common_type.
static_assert(std::is_same_v<std::common_type_t<std::tuple<int>, std::tuple<long>>, std::tuple<long>>);
static_assert(std::is_same_v<std::common_type_t<std::tuple<int, float>, std::tuple<char, double>>,
                             std::tuple<int, double>>);
static_assert(std::is_same_v<std::common_type_t<std::pair<int, short>, std::pair<long, int>>,
                             std::pair<long, int>>);
static_assert(std::is_same_v<std::common_type_t<std::tuple<int, short>, std::pair<long, int>>,
                             std::tuple<long, int>>);
static_assert(std::is_same_v<std::common_type_t<std::pair<long, int>, std::tuple<int, short>>,
                             std::tuple<long, int>>);
static_assert(std::is_same_v<std::common_type_t<std::tuple<int&>, std::tuple<const int&>>, std::tuple<int>>);
static_assert(!HasCommonType<std::tuple<int*>, std::tuple<double>>);
