// [support.exception], [support.rtti], [support.contract]: exception (constexpr, P3068),
// exception_ptr (make_exception_ptr/rethrow_exception constexpr, exception_ptr_cast returning
// optional<const E&>), nested_exception, terminate handlers, type_info (constexpr ==),
// bad_cast/bad_typeid, contract_violation's members ([support.contract.violation]).
#include <exception>
#include <contracts>
#include <optional>
#include <source_location>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

// [exception]
static_assert(noexcept(std::exception()) && noexcept(std::declval<const std::exception&>().what()));
static_assert(std::has_virtual_destructor_v<std::exception>);
constexpr bool constexpr_exception() { std::exception e; std::exception f = e; f = e; return f.what() != nullptr; }
static_assert(constexpr_exception());
static_assert(std::is_base_of_v<std::exception, std::bad_exception>);
// [propagation]
static_assert(std::is_nothrow_default_constructible_v<std::exception_ptr> && std::exception_ptr() == nullptr);
static_assert(std::is_same_v<decltype(std::exception_ptr_cast<int>(std::declval<const std::exception_ptr&>())),
                             std::optional<const int&>>);
static_assert(noexcept(std::exception_ptr_cast<int>(std::declval<const std::exception_ptr&>())));
template <class P>
concept castable = requires(P p) { std::exception_ptr_cast<int>(static_cast<P&&>(p)); };
static_assert(!castable<std::exception_ptr>);     // the rvalue overload is deleted
static_assert([] { std::exception_ptr p; return !std::exception_ptr_cast<int>(p).has_value(); }());   // constexpr
static_assert(noexcept(std::make_exception_ptr(1)) && noexcept(std::current_exception()) && noexcept(std::uncaught_exceptions()));
static_assert(std::is_same_v<decltype(std::rethrow_exception(std::exception_ptr())), void>);
// [except.nested]
static_assert(std::is_nothrow_default_constructible_v<std::nested_exception> && std::has_virtual_destructor_v<std::nested_exception>);
static_assert(noexcept(std::declval<const std::nested_exception&>().nested_ptr()));
static_assert(std::is_same_v<decltype(std::rethrow_if_nested(1)), void>);
// [exception.terminate]
static_assert(noexcept(std::set_terminate(nullptr)) && noexcept(std::get_terminate()) && noexcept(std::terminate()));
// [type.info], [type.index], [bad.cast], [bad.typeid]
static_assert(typeid(int) == typeid(const int) && typeid(int) != typeid(long));                     // constexpr ==
static_assert(noexcept(typeid(int).before(typeid(long))) && noexcept(typeid(int).hash_code()) && noexcept(typeid(int).name()));
static_assert(!std::is_copy_constructible_v<std::type_info> && !std::is_copy_assignable_v<std::type_info>);
static_assert(std::is_same_v<decltype(std::type_index(typeid(int)) <=> std::type_index(typeid(int))), std::strong_ordering>);
static_assert(std::is_base_of_v<std::exception, std::bad_cast> && std::is_base_of_v<std::exception, std::bad_typeid>);
static_assert(noexcept(std::bad_cast()) && noexcept(std::bad_typeid()));
// [support.contract]
namespace sc = std::contracts;
static_assert(std::is_scoped_enum_v<sc::assertion_kind> && int(sc::assertion_kind::assert) == 3);
static_assert(int(sc::evaluation_semantic::quick_enforce) == 4 && int(sc::detection_mode::evaluation_exception) == 2);
static_assert(!std::is_copy_constructible_v<sc::contract_violation> && !std::is_copy_assignable_v<sc::contract_violation>);
static_assert(!std::is_default_constructible_v<sc::contract_violation>);
template <class V>
concept violation_members = requires(const V& v) {
  { v.comment() } noexcept -> std::same_as<const char*>;
  { v.detection_mode() } noexcept -> std::same_as<sc::detection_mode>;
  { v.is_terminating() } noexcept -> std::same_as<bool>;
  { v.kind() } noexcept -> std::same_as<sc::assertion_kind>;
  { v.location() } noexcept -> std::same_as<std::source_location>;
  { v.semantic() } noexcept -> std::same_as<sc::evaluation_semantic>;
  sc::invoke_default_contract_violation_handler(v);
};
static_assert(violation_members<sc::contract_violation>);
