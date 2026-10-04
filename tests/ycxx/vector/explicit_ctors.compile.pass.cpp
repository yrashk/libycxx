// [vector.overview]: "constexpr explicit vector(const Allocator&) noexcept;",
// "constexpr explicit vector(size_type n, const Allocator& = Allocator());" — explicit, so
// neither an allocator nor a size converts implicitly to a vector (copy-initialization and
// copy-list-initialization from them are ill-formed); the initializer_list constructor
// "constexpr vector(initializer_list<T>, const Allocator& = Allocator());" is not explicit,
// so = {..} works; the copy and move constructors are implicit.
#include <vector>
#include <memory>
#include <initializer_list>
#include <type_traits>
#include <utility>

using V = std::vector<int>;
static_assert(!std::is_convertible_v<std::allocator<int>, V>);
static_assert(std::is_constructible_v<V, std::allocator<int>>);
static_assert(!std::is_convertible_v<std::size_t, V>);
static_assert(!std::is_convertible_v<int, V>);
static_assert(std::is_constructible_v<V, std::size_t>);
static_assert(std::is_constructible_v<V, std::size_t, std::allocator<int>>);
static_assert(std::is_convertible_v<std::initializer_list<int>, V>);
static_assert(std::is_convertible_v<const V&, V> && std::is_convertible_v<V&&, V>);
static_assert(!std::is_convertible_v<int*, V>);

template <class T>
concept copy_list_init_from_size = requires { [](T) {}({std::size_t(3)}); };
// {3} picks the initializer_list constructor for vector<int> ...
static_assert(copy_list_init_from_size<std::vector<int>>);
// ... but for a T not constructible from size_t, only explicit vector(size_type) remains,
// which copy-list-initialization cannot use.
struct NotFromSize {
  NotFromSize() = default;
};
static_assert(!copy_list_init_from_size<std::vector<NotFromSize>>);
static_assert(std::is_constructible_v<std::vector<NotFromSize>, std::size_t>);

void take(const V&);
template <class A>
concept passes = requires(A a) { take(a); };
static_assert(!passes<std::allocator<int>>);
static_assert(!passes<std::size_t>);
static_assert(passes<V>);
