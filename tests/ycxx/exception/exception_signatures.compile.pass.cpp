// [exception.syn]: exception_ptr current_exception() noexcept; [[noreturn]] void
// rethrow_exception(exception_ptr p); template<class E> exception_ptr make_exception_ptr(E e)
// noexcept; template<class E> optional<const E&> exception_ptr_cast(const exception_ptr&)
// noexcept; template<class T> [[noreturn]] void throw_with_nested(T&& t); template<class E>
// void rethrow_if_nested(const E& e). [except.nested]: nested_exception's special members
// (noexcept default/copy, virtual destructor), nested_ptr() const noexcept.
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>

struct S {
  int i;
};

static_assert(std::is_same_v<decltype(std::current_exception()), std::exception_ptr>);
static_assert(std::is_same_v<decltype(std::rethrow_exception(std::exception_ptr())), void>);
static_assert(std::is_same_v<decltype(std::make_exception_ptr(S{})), std::exception_ptr>);
static_assert(noexcept(std::make_exception_ptr(S{})));
static_assert(std::is_same_v<decltype(std::throw_with_nested(S{})), void>);
static_assert(std::is_same_v<decltype(std::throw_with_nested(1)), void>);
static_assert(std::is_same_v<decltype(std::rethrow_if_nested(1)), void>);
static_assert(std::is_same_v<decltype(std::rethrow_if_nested(S{})), void>);
static_assert(std::is_same_v<decltype(std::exception_ptr_cast<S>(std::declval<const std::exception_ptr&>())),
                             std::optional<const S&>>);

// nested_exception
static_assert(std::is_nothrow_default_constructible_v<std::nested_exception>);
static_assert(std::is_nothrow_copy_constructible_v<std::nested_exception>);
static_assert(std::is_nothrow_copy_assignable_v<std::nested_exception>);
static_assert(std::has_virtual_destructor_v<std::nested_exception>);
static_assert(!std::is_abstract_v<std::nested_exception>);
static_assert(!std::is_base_of_v<std::exception, std::nested_exception>);

// rethrow_exception takes its argument by value: an rvalue or an lvalue both work
std::exception_ptr ep;
void (*rethrow_ptr)(std::exception_ptr) = &std::rethrow_exception;
std::exception_ptr (*current_ptr)() noexcept = &std::current_exception;
template <class E>
concept rethrow_if_nested_ok = requires(const E& e) { std::rethrow_if_nested(e); };
static_assert(rethrow_if_nested_ok<int>);
static_assert(rethrow_if_nested_ok<int*>);
static_assert(rethrow_if_nested_ok<S>);
static_assert(rethrow_if_nested_ok<std::nested_exception>);
