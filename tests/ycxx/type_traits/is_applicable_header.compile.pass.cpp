// [meta.type.synop]: <type_traits> declares is_applicable, is_nothrow_applicable, apply_result,
// apply_result_t, is_applicable_v and is_nothrow_applicable_v. Only <type_traits> is included
// here (is_applicable.compile.pass.cpp checks their meaning with <tuple>).
#include <type_traits>

template <class Fn, class Tuple>
concept declared = requires {
  typename std::is_applicable<Fn, Tuple>;
  typename std::is_nothrow_applicable<Fn, Tuple>;
  typename std::apply_result<Fn, Tuple>;
  std::is_applicable_v<Fn, Tuple>;
  std::is_nothrow_applicable_v<Fn, Tuple>;
};
static_assert(declared<int (*)(), int>);
static_assert(!std::is_applicable_v<int (*)(), int>);   // int is not tuple-like ([meta.rel])
static_assert(!std::is_nothrow_applicable_v<int (*)(), int>);
static_assert(std::is_base_of_v<std::false_type, std::is_applicable<int (*)(), int>>);
template <class T>
concept has_type = requires { typename T::type; };
static_assert(!has_type<std::apply_result<int (*)(), int>>);
template <class Fn, class Tuple>
using result = std::apply_result_t<Fn, Tuple>;
