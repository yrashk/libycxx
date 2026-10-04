// [new.syn]: the replaceable global allocation and deallocation functions and the
// non-allocating forms, with their noexcept-ness:
//   void* operator new(size_t); void* operator new(size_t, align_val_t);
//   void* operator new(size_t, const nothrow_t&) noexcept; (and aligned) ... array forms alike
//   void operator delete(void*) noexcept; (void*, size_t), (void*, align_val_t),
//   (void*, size_t, align_val_t), (void*, const nothrow_t&), (void*, align_val_t, const
//   nothrow_t&): all noexcept; array forms alike.
//   constexpr void* operator new(size_t, void*) noexcept; constexpr void* operator new[](size_t,
//   void*) noexcept; void operator delete(void*, void*) noexcept; void operator delete[](void*,
//   void*) noexcept.
#include <new>
#include <cstddef>
#include <type_traits>
#include <utility>

void* p = nullptr;
constexpr std::size_t n = 1;
constexpr std::align_val_t a{16};
const std::nothrow_t& nt = std::nothrow;

static_assert(std::is_same_v<decltype(::operator new(n)), void*>);
static_assert(std::is_same_v<decltype(::operator new(n, a)), void*>);
static_assert(std::is_same_v<decltype(::operator new[](n)), void*>);
static_assert(std::is_same_v<decltype(::operator new[](n, a)), void*>);
static_assert(!noexcept(::operator new(n)) && !noexcept(::operator new(n, a)));
static_assert(!noexcept(::operator new[](n)) && !noexcept(::operator new[](n, a)));
static_assert(noexcept(::operator new(n, nt)) && noexcept(::operator new(n, a, nt)));
static_assert(noexcept(::operator new[](n, nt)) && noexcept(::operator new[](n, a, nt)));
static_assert(std::is_same_v<decltype(::operator new(n, nt)), void*>);
static_assert(std::is_same_v<decltype(::operator new[](n, a, nt)), void*>);

static_assert(noexcept(::operator delete(p)) && noexcept(::operator delete(p, n)));
static_assert(noexcept(::operator delete(p, a)) && noexcept(::operator delete(p, n, a)));
static_assert(noexcept(::operator delete(p, nt)) && noexcept(::operator delete(p, a, nt)));
static_assert(noexcept(::operator delete[](p)) && noexcept(::operator delete[](p, n)));
static_assert(noexcept(::operator delete[](p, a)) && noexcept(::operator delete[](p, n, a)));
static_assert(noexcept(::operator delete[](p, nt)) && noexcept(::operator delete[](p, a, nt)));
static_assert(std::is_same_v<decltype(::operator delete(p)), void>);
static_assert(std::is_same_v<decltype(::operator delete[](p, n, a)), void>);

static_assert(noexcept(::operator new(n, p)) && noexcept(::operator new[](n, p)));
static_assert(std::is_same_v<decltype(::operator new(n, p)), void*>);
static_assert(noexcept(::operator delete(p, p)) && noexcept(::operator delete[](p, p)));
static_assert(std::is_same_v<decltype(::operator delete(p, p)), void>);
