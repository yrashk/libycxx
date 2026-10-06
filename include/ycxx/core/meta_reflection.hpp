// libycxx core: <meta>, the reflection library ([meta.reflection], P2996 and its follow-ups).
//
// The metafunctions are the compiler's (DECISIONS §13). GCC 16 (-freflection) evaluates a call
// to any `consteval` function or function template declared in namespace std::meta, without a
// definition, whose name it knows ("unknown metafunction" otherwise); it checks the return type,
// builds the result (vector<info> from a braced list, string_view from a const char*,
// source_location, member_offset, access_context, strong_ordering), throws meta::exception
// through its (string_view, info, source_location) constructor, and reads the arguments through
// ordinary expressions (ranges::begin/end on a reflection_range, `static_cast<bool>(__opt)` and
// `*__opt` on the optionals of data_member_options, access_context::scope() and
// designating_class()). So this header declares those functions only, and defines the classes
// the compiler builds or reads and the functions the compiler does not provide:
// access_context's factories, define_static_*, is_string_literal and the apply traits.
//
// `^^` cannot be parsed without reflection, so everything is under _YCXX_HAS_REFLECTION (Clang 23
// has none): the header is then empty.
#pragma once

#include <ycxx/config.hpp>

#if _YCXX_HAS_REFLECTION

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/source_location.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/variant.hpp>
#include <ycxx/core/vector.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace meta {
using info = decltype(^^::);
class exception;
}} // namespace std::meta

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::meta {

// The ordinary literal encoding is UTF-8 ([meta.reflection.exception] transcodes between it and
// UTF-8). Otherwise only ASCII is taken to be shared by both.
consteval bool __literal_is_utf8() {
  constexpr char s[] = "\u00e9";
  return sizeof(s) == 3 && static_cast<unsigned char>(s[0]) == 0xc3 && static_cast<unsigned char>(s[1]) == 0xa9;
}
template <class _CharT>
consteval bool __is_ascii(std::basic_string_view<_CharT> s) {
  for (_CharT c : s)
    if (static_cast<unsigned>(c) >= 0x80)
      return false;
  return true;
}
// Not a constant expression: called where the draft's "Constant When" does not hold.
inline void __not_representable_in_utf8() {}

// The extent of define_static_array's span: ranges::size(r) when that is a constant expression,
// which it is when the size does not depend on the object (arrays, array, span<T, N>): asked of
// a reference to an unknown object (P2280), never defined.
template <class _Tp>
extern _Tp& __unknown_object;
template <std::size_t _Np>
struct __size_tag {};
template <class _Rp>
concept __constant_sized =
    requires { typename __size_tag<static_cast<std::size_t>(std::ranges::size(__unknown_object<_Rp>))>; };
template <class _Rp>
consteval std::size_t __static_array_extent() {
  if constexpr (__constant_sized<_Rp>)
    return static_cast<std::size_t>(std::ranges::size(__unknown_object<_Rp>));
  else
    return std::dynamic_extent;
}

[[noreturn]] consteval void raise(std::string_view what, std::meta::info from, std::source_location where);

}} // namespace __ycxx::__detail::meta

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// meta::exception holds reflections, so (in GCC 16) every member function of it must be
// consteval; what() is constexpr and virtual, so it lives in this base, which holds no
// reflection, and meta::exception inherits it as its final overrider.
class __meta_exception_what : public std::exception {
protected:
  std::optional<std::string> __what_;

public:
  constexpr __meta_exception_what() noexcept = default;
  constexpr __meta_exception_what(const __meta_exception_what&) = default;
  constexpr __meta_exception_what(__meta_exception_what&&) = default;
  constexpr __meta_exception_what& operator=(const __meta_exception_what&) = default;
  constexpr __meta_exception_what& operator=(__meta_exception_what&&) = default;
  constexpr const char* what() const noexcept override { return __what_->c_str(); }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

// [meta.string.literal]
consteval bool is_string_literal(const char* p) { return __builtin_is_string_literal(p); }
consteval bool is_string_literal(const wchar_t* p) { return __builtin_is_string_literal(p); }
consteval bool is_string_literal(const char8_t* p) { return __builtin_is_string_literal(p); }
consteval bool is_string_literal(const char16_t* p) { return __builtin_is_string_literal(p); }
consteval bool is_string_literal(const char32_t* p) { return __builtin_is_string_literal(p); }

namespace meta {

// [meta.reflection.exception]
class exception : public __ycxx::__adl_free::__meta_exception_what {
  u8string __u8what_;
  info __from_;
  source_location __where_;

public:
  consteval exception(u8string_view what, info from, source_location where = source_location::current()) noexcept
      : __u8what_(what), __from_(from), __where_(where) {
    if (::__ycxx::__detail::meta::__literal_is_utf8() || ::__ycxx::__detail::meta::__is_ascii(what))
      __what_.emplace(what.begin(), what.end());
  }
  consteval exception(string_view what, info from, source_location where = source_location::current()) noexcept
      : __from_(from), __where_(where) {
    if (!::__ycxx::__detail::meta::__literal_is_utf8() && !::__ycxx::__detail::meta::__is_ascii(what))
      ::__ycxx::__detail::meta::__not_representable_in_utf8();
    __what_.emplace(what);
    __u8what_.assign(what.begin(), what.end());
  }
  exception(const exception&) = default;
  exception(exception&&) = default;
  exception& operator=(const exception&) = default;
  exception& operator=(exception&&) = default;
  consteval u8string_view u8what() const noexcept { return __u8what_; }
  consteval info from() const noexcept { return __from_; }
  consteval source_location where() const noexcept { return __where_; }
};

// [meta.reflection.operators]. The compiler finds the enumerators by name; the values are ours.
enum class operators {
  op_new = 1,
  op_delete,
  op_array_new,
  op_array_delete,
  op_co_await,
  op_parentheses,
  op_square_brackets,
  op_arrow,
  op_arrow_star,
  op_tilde,
  op_exclamation,
  op_plus,
  op_minus,
  op_star,
  op_slash,
  op_percent,
  op_caret,
  op_ampersand,
  op_equals,
  op_pipe,
  op_plus_equals,
  op_minus_equals,
  op_star_equals,
  op_slash_equals,
  op_percent_equals,
  op_caret_equals,
  op_ampersand_equals,
  op_pipe_equals,
  op_equals_equals,
  op_exclamation_equals,
  op_less,
  op_greater,
  op_less_equals,
  op_greater_equals,
  op_spaceship,
  op_ampersand_ampersand,
  op_pipe_pipe,
  op_less_less,
  op_greater_greater,
  op_less_less_equals,
  op_greater_greater_equals,
  op_plus_plus,
  op_minus_minus,
  op_comma,
};
using enum operators;
consteval operators operator_of(info r);
consteval string_view symbol_of(operators op);
consteval u8string_view u8symbol_of(operators op);

// [meta.reflection.names]
consteval bool has_identifier(info r);
consteval string_view identifier_of(info r);
consteval u8string_view u8identifier_of(info r);
consteval string_view display_string_of(info r);
consteval u8string_view u8display_string_of(info r);
consteval source_location source_location_of(info r);

// [meta.reflection.queries]
consteval info type_of(info r);
consteval info object_of(info r);
consteval info constant_of(info r);
consteval bool is_public(info r);
consteval bool is_protected(info r);
consteval bool is_private(info r);
consteval bool is_virtual(info r);
consteval bool is_pure_virtual(info r);
consteval bool is_override(info r);
consteval bool is_final(info r);
consteval bool is_deleted(info r);
consteval bool is_defaulted(info r);
consteval bool is_user_provided(info r);
consteval bool is_user_declared(info r);
consteval bool is_explicit(info r);
consteval bool is_noexcept(info r);
consteval bool is_bit_field(info r);
consteval bool is_enumerator(info r);
consteval bool is_annotation(info r);
consteval bool is_const(info r);
consteval bool is_volatile(info r);
consteval bool is_mutable_member(info r);
consteval bool is_lvalue_reference_qualified(info r);
consteval bool is_rvalue_reference_qualified(info r);
consteval bool has_static_storage_duration(info r);
consteval bool has_thread_storage_duration(info r);
consteval bool has_automatic_storage_duration(info r);
consteval bool has_internal_linkage(info r);
consteval bool has_module_linkage(info r);
consteval bool has_external_linkage(info r);
consteval bool has_c_language_linkage(info r);
consteval bool has_linkage(info r);
consteval bool is_complete_type(info r);
consteval bool is_enumerable_type(info r);
consteval bool is_variable(info r);
consteval bool is_type(info r);
consteval bool is_namespace(info r);
consteval bool is_type_alias(info r);
consteval bool is_namespace_alias(info r);
consteval bool is_function(info r);
consteval bool is_conversion_function(info r);
consteval bool is_operator_function(info r);
consteval bool is_literal_operator(info r);
consteval bool is_special_member_function(info r);
consteval bool is_constructor(info r);
consteval bool is_default_constructor(info r);
consteval bool is_copy_constructor(info r);
consteval bool is_move_constructor(info r);
consteval bool is_assignment(info r);
consteval bool is_copy_assignment(info r);
consteval bool is_move_assignment(info r);
consteval bool is_destructor(info r);
consteval bool is_function_parameter(info r);
consteval bool is_explicit_object_parameter(info r);
consteval bool has_default_argument(info r);
consteval bool is_vararg_function(info r);
consteval bool is_template(info r);
consteval bool is_function_template(info r);
consteval bool is_variable_template(info r);
consteval bool is_class_template(info r);
consteval bool is_alias_template(info r);
consteval bool is_conversion_function_template(info r);
consteval bool is_operator_function_template(info r);
consteval bool is_literal_operator_template(info r);
consteval bool is_constructor_template(info r);
consteval bool is_concept(info r);
consteval bool is_value(info r);
consteval bool is_object(info r);
consteval bool is_structured_binding(info r);
consteval bool is_class_member(info r);
consteval bool is_namespace_member(info r);
consteval bool is_nonstatic_data_member(info r);
consteval bool is_static_member(info r);
consteval bool is_base(info r);
consteval bool has_default_member_initializer(info r);
consteval bool has_parent(info r);
consteval info parent_of(info r);
consteval info dealias(info r);
consteval bool has_template_arguments(info r);
consteval info template_of(info r);
consteval vector<info> template_arguments_of(info r);
consteval vector<info> parameters_of(info r);
consteval info variable_of(info r);
consteval info return_type_of(info r);

// [meta.reflection.access.context]. The compiler builds access_context::current() from the two
// members in this order and reads an argument through scope() and designating_class().
struct access_context {
  access_context() = delete;
  consteval info scope() const { return __scope_; }
  consteval info designating_class() const { return __designating_class_; }
  static consteval access_context current() noexcept;
  static consteval access_context unprivileged() noexcept { return access_context(^^::, info()); }
  static consteval access_context unchecked() noexcept { return access_context(info(), info()); }
  consteval access_context via(info __cls) const;

private:
  consteval access_context(info scope, info __cls) noexcept : __scope_(scope), __designating_class_(__cls) {}

public:
  // Public, so that access_context is a structural type ([meta.reflection.access.context]/3).
  info __scope_;
  info __designating_class_;
};

// [meta.reflection.access.queries]
consteval bool is_accessible(info r, access_context __ctx);
consteval bool has_inaccessible_nonstatic_data_members(info r, access_context __ctx);
consteval bool has_inaccessible_bases(info r, access_context __ctx);
consteval bool has_inaccessible_subobjects(info r, access_context __ctx);

// [meta.reflection.scope]
consteval info current_function();
consteval info current_class();
consteval info current_namespace();

// [meta.reflection.member.queries]
consteval vector<info> members_of(info r, access_context __ctx);
consteval vector<info> bases_of(info type, access_context __ctx);
consteval vector<info> static_data_members_of(info type, access_context __ctx);
consteval vector<info> nonstatic_data_members_of(info type, access_context __ctx);
consteval vector<info> subobjects_of(info type, access_context __ctx);
consteval vector<info> enumerators_of(info __type_enum);

// [meta.reflection.layout]
struct member_offset {
  ptrdiff_t bytes;
  ptrdiff_t bits;
  constexpr ptrdiff_t total_bits() const { return bytes * __CHAR_BIT__ + bits; }
  auto operator<=>(const member_offset&) const = default;
};
consteval member_offset offset_of(info r);
consteval size_t size_of(info r);
consteval size_t alignment_of(info r);
consteval size_t bit_size_of(info r);

// [meta.reflection.annotation]
consteval vector<info> annotations_of(info __item);
consteval vector<info> annotations_of_with_type(info __item, info type);

// [meta.reflection.extract]
template <class _Tp>
consteval _Tp extract(info r);

// [meta.reflection.substitute]
template <class _Rp>
concept reflection_range = ranges::input_range<_Rp> && same_as<ranges::range_value_t<_Rp>, info> &&
                           same_as<remove_cvref_t<ranges::range_reference_t<_Rp>>, info>;
template <reflection_range _Rp = initializer_list<info>>
consteval bool can_substitute(info __templ, _Rp&& __arguments);
template <reflection_range _Rp = initializer_list<info>>
consteval info substitute(info __templ, _Rp&& __arguments);

// [meta.reflection.result]
template <class _Tp>
consteval info reflect_constant(_Tp __expr);
template <class _Tp>
consteval info reflect_object(_Tp& __expr);
template <class _Tp>
consteval info reflect_function(_Tp& __fn);

// [meta.reflection.define.aggregate]. The compiler reads name_type through the members
// _M_is_u8, _M_u8s and _M_s (the names are its contract, like source_location::__impl's);
// they hold the draft's variant<u8string, string>.
struct data_member_options {
  struct __name_type {
    template <class _Tp>
      requires constructible_from<u8string, _Tp>
    consteval __name_type(_Tp&& value) : _M_is_u8(true), _M_u8s(static_cast<_Tp&&>(value)) {}
    template <class _Tp>
      requires constructible_from<string, _Tp>
    consteval __name_type(_Tp&& value) : _M_is_u8(false), _M_s(static_cast<_Tp&&>(value)) {}

  private:
    bool _M_is_u8;
    u8string _M_u8s;
    string _M_s;
  };
  optional<__name_type> name{};
  optional<int> alignment{};
  optional<int> bit_width{};
  bool no_unique_address = false;
  vector<info> annotations{};
};
consteval info data_member_spec(info type, data_member_options options);
consteval bool is_data_member_spec(info r);
template <reflection_range _Rp = initializer_list<info>>
consteval info define_aggregate(info __type_class, _Rp&& __mdescrs);

// [meta.define.static]
template <ranges::input_range _Rp>
consteval info reflect_constant_string(_Rp&& r);
template <ranges::input_range _Rp>
consteval info reflect_constant_array(_Rp&& r);

// [meta.reflection.traits]: associated with [meta.unary.cat]
consteval bool is_void_type(info type);
consteval bool is_null_pointer_type(info type);
consteval bool is_integral_type(info type);
consteval bool is_floating_point_type(info type);
consteval bool is_array_type(info type);
consteval bool is_pointer_type(info type);
consteval bool is_lvalue_reference_type(info type);
consteval bool is_rvalue_reference_type(info type);
consteval bool is_member_object_pointer_type(info type);
consteval bool is_member_function_pointer_type(info type);
consteval bool is_enum_type(info type);
consteval bool is_union_type(info type);
consteval bool is_class_type(info type);
consteval bool is_function_type(info type);
consteval bool is_reflection_type(info type);
// [meta.unary.comp]
consteval bool is_reference_type(info type);
consteval bool is_arithmetic_type(info type);
consteval bool is_fundamental_type(info type);
consteval bool is_object_type(info type);
consteval bool is_scalar_type(info type);
consteval bool is_compound_type(info type);
consteval bool is_member_pointer_type(info type);
// [meta.unary.prop]
consteval bool is_const_type(info type);
consteval bool is_volatile_type(info type);
consteval bool is_trivially_copyable_type(info type);
consteval bool is_standard_layout_type(info type);
consteval bool is_empty_type(info type);
consteval bool is_polymorphic_type(info type);
consteval bool is_abstract_type(info type);
consteval bool is_final_type(info type);
consteval bool is_aggregate_type(info type);
consteval bool is_structural_type(info type);
consteval bool is_signed_type(info type);
consteval bool is_unsigned_type(info type);
consteval bool is_bounded_array_type(info type);
consteval bool is_unbounded_array_type(info type);
consteval bool is_scoped_enum_type(info type);
template <reflection_range _Rp = initializer_list<info>>
consteval bool is_constructible_type(info type, _Rp&& __type_args);
consteval bool is_default_constructible_type(info type);
consteval bool is_copy_constructible_type(info type);
consteval bool is_move_constructible_type(info type);
consteval bool is_assignable_type(info __type_dst, info __type_src);
consteval bool is_copy_assignable_type(info type);
consteval bool is_move_assignable_type(info type);
consteval bool is_swappable_with_type(info __type1, info __type2);
consteval bool is_swappable_type(info type);
consteval bool is_destructible_type(info type);
template <reflection_range _Rp = initializer_list<info>>
consteval bool is_trivially_constructible_type(info type, _Rp&& __type_args);
consteval bool is_trivially_default_constructible_type(info type);
consteval bool is_trivially_copy_constructible_type(info type);
consteval bool is_trivially_move_constructible_type(info type);
consteval bool is_trivially_assignable_type(info __type_dst, info __type_src);
consteval bool is_trivially_copy_assignable_type(info type);
consteval bool is_trivially_move_assignable_type(info type);
consteval bool is_trivially_destructible_type(info type);
template <reflection_range _Rp = initializer_list<info>>
consteval bool is_nothrow_constructible_type(info type, _Rp&& __type_args);
consteval bool is_nothrow_default_constructible_type(info type);
consteval bool is_nothrow_copy_constructible_type(info type);
consteval bool is_nothrow_move_constructible_type(info type);
consteval bool is_nothrow_assignable_type(info __type_dst, info __type_src);
consteval bool is_nothrow_copy_assignable_type(info type);
consteval bool is_nothrow_move_assignable_type(info type);
consteval bool is_nothrow_swappable_with_type(info __type1, info __type2);
consteval bool is_nothrow_swappable_type(info type);
consteval bool is_nothrow_destructible_type(info type);
consteval bool is_implicit_lifetime_type(info type);
consteval bool has_virtual_destructor(info type);
consteval bool has_unique_object_representations(info type);
consteval bool reference_constructs_from_temporary(info __type_dst, info __type_src);
consteval bool reference_converts_from_temporary(info __type_dst, info __type_src);
// [meta.unary.prop.query]
consteval size_t rank(info type);
consteval size_t extent(info type, unsigned i = 0);
// [meta.rel]
consteval bool is_same_type(info __type1, info __type2);
consteval bool is_base_of_type(info __type_base, info __type_derived);
consteval bool is_virtual_base_of_type(info __type_base, info __type_derived);
consteval bool is_convertible_type(info __type_src, info __type_dst);
consteval bool is_nothrow_convertible_type(info __type_src, info __type_dst);
consteval bool is_layout_compatible_type(info __type1, info __type2);
consteval bool is_pointer_interconvertible_base_of_type(info __type_base, info __type_derived);
template <reflection_range _Rp = initializer_list<info>>
consteval bool is_invocable_type(info type, _Rp&& __type_args);
template <reflection_range _Rp = initializer_list<info>>
consteval bool is_invocable_r_type(info __type_result, info type, _Rp&& __type_args);
template <reflection_range _Rp = initializer_list<info>>
consteval bool is_nothrow_invocable_type(info type, _Rp&& __type_args);
template <reflection_range _Rp = initializer_list<info>>
consteval bool is_nothrow_invocable_r_type(info __type_result, info type, _Rp&& __type_args);
// [meta.trans.cv]
consteval info remove_const(info type);
consteval info remove_volatile(info type);
consteval info remove_cv(info type);
consteval info add_const(info type);
consteval info add_volatile(info type);
consteval info add_cv(info type);
// [meta.trans.ref]
consteval info remove_reference(info type);
consteval info add_lvalue_reference(info type);
consteval info add_rvalue_reference(info type);
// [meta.trans.sign]
consteval info make_signed(info type);
consteval info make_unsigned(info type);
// [meta.trans.arr]
consteval info remove_extent(info type);
consteval info remove_all_extents(info type);
// [meta.trans.ptr]
consteval info remove_pointer(info type);
consteval info add_pointer(info type);
// [meta.trans.other]
consteval info remove_cvref(info type);
consteval info decay(info type);
template <reflection_range _Rp = initializer_list<info>>
consteval info common_type(_Rp&& __type_args);
template <reflection_range _Rp = initializer_list<info>>
consteval info common_reference(_Rp&& __type_args);
consteval info underlying_type(info type);
template <reflection_range _Rp = initializer_list<info>>
consteval info invoke_result(info type, _Rp&& __type_args);
consteval info unwrap_reference(info type);
consteval info unwrap_ref_decay(info type);
consteval size_t tuple_size(info type);
consteval info tuple_element(size_t index, info type);
consteval size_t variant_size(info type);
consteval info variant_alternative(size_t index, info type);
consteval strong_ordering type_order(info __type_a, info __type_b);

// The apply traits are not GCC 16 metafunctions: they evaluate the class templates of
// <tuple> through substitute.
consteval bool is_applicable_type(info __fn, info tuple) {
  if (!is_type(__fn) || !is_type(tuple))
    ::__ycxx::__detail::meta::raise("is_applicable_type: argument is not a type", ^^is_applicable_type,
                                source_location::current());
  return extract<bool>(substitute(^^std::is_applicable_v, {__fn, tuple}));
}
consteval bool is_nothrow_applicable_type(info __fn, info tuple) {
  if (!is_type(__fn) || !is_type(tuple))
    ::__ycxx::__detail::meta::raise("is_nothrow_applicable_type: argument is not a type", ^^is_nothrow_applicable_type,
                                source_location::current());
  return extract<bool>(substitute(^^std::is_nothrow_applicable_v, {__fn, tuple}));
}
consteval info apply_result(info __fn, info tuple) {
  if (!is_applicable_type(__fn, tuple))
    ::__ycxx::__detail::meta::raise("apply_result: the function type is not applicable to the tuple type", ^^apply_result,
                                source_location::current());
  return dealias(substitute(^^std::apply_result_t, {__fn, tuple}));
}

consteval access_context access_context::via(info __cls) const {
  if (__cls != info() && !(is_type(__cls) && is_class_type(__cls) && is_complete_type(__cls)))
    ::__ycxx::__detail::meta::raise("access_context::via: argument is not a complete class type", ^^access_context::via,
                                source_location::current());
  return access_context(__scope_, __cls);
}

} // namespace meta

// [meta.define.static]
template <ranges::input_range _Rp>
consteval const ranges::range_value_t<_Rp>* define_static_string(_Rp&& r) {
  return meta::extract<const ranges::range_value_t<_Rp>*>(meta::reflect_constant_string(r));
}
template <ranges::input_range _Rp>
consteval span<const ranges::range_value_t<_Rp>, ::__ycxx::__detail::meta::__static_array_extent<_Rp>()>
define_static_array(_Rp&& r) {
  using _Tp = ranges::range_value_t<_Rp>;
  using _Sp = span<const _Tp, ::__ycxx::__detail::meta::__static_array_extent<_Rp>()>;
  meta::info array = meta::reflect_constant_array(r);
  if (meta::is_array_type(meta::type_of(array)))
    return _Sp(meta::extract<const _Tp*>(array), meta::extent(meta::type_of(array)));
  return _Sp(static_cast<const _Tp*>(nullptr), 0);
}
template <class _Tp>
consteval const remove_cvref_t<_Tp>* define_static_object(_Tp&& t) {
  using _Up = remove_cvref_t<_Tp>;
  if constexpr (meta::is_class_type(^^_Up) || meta::is_union_type(^^_Up))
    return __builtin_addressof(meta::extract<const _Up&>(meta::reflect_constant(static_cast<_Tp&&>(t))));
  else if constexpr (meta::is_array_type(^^_Up))
    return __builtin_addressof(meta::extract<const _Up&>(meta::reflect_constant_array(static_cast<_Tp&&>(t))));
  else
    return std::define_static_array(span(__builtin_addressof(t), 1)).data();
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::meta {
consteval void raise(std::string_view what, std::meta::info from, std::source_location where) {
  ::__ycxx::__detail::__raise_with(ycxx_error_logic_error, "std::meta::exception",
                             [&] { return std::meta::exception(what, from, where); });
}
}} // namespace __ycxx::__detail::meta

#endif // _YCXX_HAS_REFLECTION
