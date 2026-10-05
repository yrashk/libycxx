// [deque.overview] synopsis: operator=(deque&&) is
// noexcept(allocator_traits<Allocator>::is_always_equal::value); swap(deque&) likewise;
// the non-member swap is noexcept(noexcept(x.swap(y))) ([deque.syn]); get_allocator, begin,
// end, rbegin, rend, cbegin, cend, crbegin, crend, empty, size, max_size and clear are
// noexcept. [container.reqmts]: the move constructor exists and an allocator-aware container
// can be moved; [res.on.exception.handling] lets implementations add noexcept but not drop it.
// REQUIRES: exceptions
#include <deque>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"

using D = std::deque<int>;
using DI = std::deque<int, IdAlloc<int>>;  // is_always_equal is false
D& d() noexcept;
const D& cd() noexcept;

static_assert(std::is_nothrow_move_assignable_v<D>);
static_assert(noexcept(d().swap(d())));
static_assert(noexcept(std::swap(d(), d())));
static_assert(noexcept(swap(d(), d())));
static_assert(noexcept(d().get_allocator()));
static_assert(noexcept(d().begin()) && noexcept(d().end()) && noexcept(cd().begin()) && noexcept(cd().end()));
static_assert(noexcept(d().rbegin()) && noexcept(d().rend()) && noexcept(cd().rbegin()) && noexcept(cd().rend()));
static_assert(noexcept(d().cbegin()) && noexcept(d().cend()) && noexcept(d().crbegin()) && noexcept(d().crend()));
static_assert(noexcept(cd().empty()) && noexcept(cd().size()) && noexcept(cd().max_size()));
static_assert(noexcept(d().clear()));
static_assert(std::is_nothrow_swappable_v<D>);

// with an allocator that is not always equal, nothing is promised (but must compile)
static_assert(std::is_move_assignable_v<DI>);
static_assert(std::is_swappable_v<DI>);

int main() { return 0; }
