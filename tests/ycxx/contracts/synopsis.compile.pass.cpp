// [contracts.syn]: the enumerations assertion_kind, evaluation_semantic, detection_mode (scoped,
// with the listed values), class contract_violation (no user-accessible constructor, copy
// operations deleted, the noexcept observers with the listed return types) and
// invoke_default_contract_violation_handler(const contract_violation&).
// Needs no compiler support for contract assertions.
#include <contracts>
#include <source_location>
#include <type_traits>

namespace c = std::contracts;

template <class E>
constexpr bool scoped_enum = std::is_enum_v<E> && !std::is_convertible_v<E, int>;
static_assert(scoped_enum<c::assertion_kind>);
static_assert(scoped_enum<c::evaluation_semantic>);
static_assert(scoped_enum<c::detection_mode>);

static_assert(static_cast<int>(c::assertion_kind::pre) == 1);
static_assert(static_cast<int>(c::assertion_kind::post) == 2);
static_assert(static_cast<int>(c::assertion_kind::assert) == 3);
static_assert(static_cast<int>(c::evaluation_semantic::ignore) == 1);
static_assert(static_cast<int>(c::evaluation_semantic::observe) == 2);
static_assert(static_cast<int>(c::evaluation_semantic::enforce) == 3);
static_assert(static_cast<int>(c::evaluation_semantic::quick_enforce) == 4);
static_assert(static_cast<int>(c::detection_mode::predicate_false) == 1);
static_assert(static_cast<int>(c::detection_mode::evaluation_exception) == 2);

using V = c::contract_violation;
static_assert(std::is_class_v<V>);
static_assert(!std::is_default_constructible_v<V>);
static_assert(!std::is_copy_constructible_v<V> && !std::is_move_constructible_v<V>);
static_assert(!std::is_copy_assignable_v<V> && !std::is_move_assignable_v<V>);
static_assert(std::is_destructible_v<V>);

static_assert(std::is_same_v<decltype(std::declval<const V&>().comment()), const char*>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().detection_mode()), c::detection_mode>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().is_terminating()), bool>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().kind()), c::assertion_kind>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().location()), std::source_location>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().semantic()), c::evaluation_semantic>);
static_assert(noexcept(std::declval<const V&>().comment()));
static_assert(noexcept(std::declval<const V&>().detection_mode()));
static_assert(noexcept(std::declval<const V&>().is_terminating()));
static_assert(noexcept(std::declval<const V&>().kind()));
static_assert(noexcept(std::declval<const V&>().location()));
static_assert(noexcept(std::declval<const V&>().semantic()));

// (an implementation may add noexcept, [res.on.exception.handling]/5, so the type of its address is
// not checked)
static_assert(std::is_same_v<decltype(c::invoke_default_contract_violation_handler(std::declval<const V&>())), void>);

int main() {}
