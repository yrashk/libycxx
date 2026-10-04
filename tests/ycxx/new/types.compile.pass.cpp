// [new.syn]: destroying_delete_t { explicit destroying_delete_t() = default; } and the
// inline constexpr object destroying_delete; enum class align_val_t : size_t {};
// nothrow_t { explicit nothrow_t() = default; } and extern const nothrow_t nothrow;
// new_handler = void (*)(); get/set_new_handler noexcept; launder constexpr noexcept;
// hardware_{destructive,constructive}_interference_size are inline constexpr size_t.
#include <new>
#include <cstddef>
#include <type_traits>

static_assert(std::is_enum_v<std::align_val_t>);
static_assert(!std::is_convertible_v<std::align_val_t, std::size_t>);  // scoped
static_assert(std::is_same_v<std::underlying_type_t<std::align_val_t>, std::size_t>);
static_assert(std::is_empty_v<std::nothrow_t> && std::is_empty_v<std::destroying_delete_t>);
static_assert(std::is_default_constructible_v<std::nothrow_t>);
static_assert(std::is_same_v<decltype(std::nothrow), const std::nothrow_t>);
static_assert(std::is_same_v<decltype(std::destroying_delete), const std::destroying_delete_t>);
static_assert(std::is_same_v<std::new_handler, void (*)()>);
static_assert(noexcept(std::get_new_handler()));
static_assert(noexcept(std::set_new_handler(nullptr)));
static_assert(std::is_same_v<decltype(std::set_new_handler(nullptr)), std::new_handler>);
static_assert(std::is_same_v<decltype(std::hardware_destructive_interference_size), const std::size_t>);
static_assert(std::is_same_v<decltype(std::hardware_constructive_interference_size), const std::size_t>);
static_assert(std::hardware_destructive_interference_size >= alignof(std::max_align_t));
static_assert(std::hardware_constructive_interference_size >= alignof(std::max_align_t));
static_assert(std::is_base_of_v<std::exception, std::bad_alloc>);
static_assert(std::is_base_of_v<std::bad_alloc, std::bad_array_new_length>);

// explicit default constructors: copy-list-initialisation from {} is ill-formed
template <class T>
concept implicit_from_braces = requires(void (*f)(T)) { f({}); };
static_assert(!implicit_from_braces<std::nothrow_t>);
static_assert(!implicit_from_braces<std::destroying_delete_t>);

// launder
static_assert(noexcept(std::launder(static_cast<int*>(nullptr))));
static_assert(std::is_same_v<decltype(std::launder(static_cast<const int*>(nullptr))), const int*>);
constexpr int k = 5;
static_assert(*std::launder(&k) == 5);
