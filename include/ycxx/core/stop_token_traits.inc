// Traits for the native and portable execution environments.
#pragma once

namespace __stop_token_detail {
template <template <class> class>
struct __check_type_alias_exists;

// A const _Token object of unknown value: unstoppable_token's stand-in for its requires-parameter,
// which has the same type. Declared only, as nothing ever evaluates it at run time. An object
// rather than a reference: a constant evaluation may copy an empty object or take the address
// of an unknown object, but not of the referent of an unknown reference.
template <class _Token>
extern const _Token __unknown_token;

} // namespace __stop_token_detail

template <class _Tp, class _CallbackFn>
using stop_callback_for_t = typename _Tp::template callback_type<_CallbackFn>;

// [stoptoken.concepts]
template <class _Token>
concept stoppable_token = requires(const _Token __tok) {
  typename __stop_token_detail::__check_type_alias_exists<_Token::template callback_type>;
  { __tok.stop_requested() } noexcept -> ::std::same_as<bool>;
  { __tok.stop_possible() } noexcept -> ::std::same_as<bool>;
  { _Token(__tok) } noexcept;
} && ::std::copyable<_Token> && ::std::equality_comparable<_Token>;

// COMPILER-BUG(gcc): GCC 16 ICEs (tsubst_expr) in an importer of the std module when a
// requires-parameter, of object or reference type, appears in a template argument inside a
// requires-expression (nested, simple or type requirement), as in the draft's
// `requires(const Token tok) { requires ::std::bool_constant<(!tok.stop_possible())>::value; }`.
// A namespace-scope const _Token of unknown value takes tok's place: like tok, it is an object
// not usable in constant expressions ([expr.const]), so the call is a constant expression in the
// same cases. Own test: modules/import_std_unstoppable_token_gcc_ice.
template <class _Token>
concept unstoppable_token = stoppable_token<_Token> && requires {
  requires ::std::bool_constant<(!__stop_token_detail::__unknown_token<_Token>.stop_possible())>::value;
};

// [stoptoken.never]
class never_stop_token {
  struct __callback_type_ {
    explicit __callback_type_(never_stop_token, auto&&) noexcept {}
  };

public:
  template <class>
  using callback_type = __callback_type_;
  static constexpr bool stop_requested() noexcept { return false; }
  static constexpr bool stop_possible() noexcept { return false; }
  bool operator==(const never_stop_token&) const = default;
};
