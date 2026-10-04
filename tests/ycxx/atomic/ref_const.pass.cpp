// [atomics.ref.generic.general], [atomics.ref.ops]: atomic_ref<const T> references a const
// object and supports load / wait; store, exchange, compare_exchange, notify and the fetch
// operations have "Constraints: is_const_v<T> is false." /9: the converting constructor
// template<class U> atomic_ref(const atomic_ref<U>&) is constrained on "T and U are similar
// types, and is_convertible_v<U*, T*> is true".
#include <atomic>
#include "check.hpp"

template<class R> concept can_store = requires(R r) { r.store(1); };
template<class R> concept can_assign = requires(R r) { r = 1; };
template<class R> concept can_exchange = requires(R r) { r.exchange(1); };
template<class R> concept can_cas = requires(R r, int& e) { r.compare_exchange_strong(e, 1); };
template<class R> concept can_fetch_add = requires(R r) { r.fetch_add(1); };
template<class R> concept can_increment = requires(R r) { ++r; };
template<class R> concept can_notify = requires(R r) { r.notify_one(); };
template<class R> concept can_load = requires(R r) { r.load(); r.wait(1); };

using CR = std::atomic_ref<const int>;
static_assert(can_load<CR>);
static_assert(!can_store<CR> && !can_assign<CR> && !can_exchange<CR> && !can_cas<CR>);
static_assert(!can_fetch_add<CR> && !can_increment<CR> && !can_notify<CR>);
static_assert(can_store<std::atomic_ref<int>> && can_fetch_add<std::atomic_ref<int>>);

static_assert(std::is_constructible_v<CR, const std::atomic_ref<int>&>);
static_assert(std::is_convertible_v<const std::atomic_ref<int>&, CR>);
static_assert(!std::is_constructible_v<std::atomic_ref<int>, const CR&>);
static_assert(!std::is_constructible_v<std::atomic_ref<long>, const std::atomic_ref<int>&>);
static_assert(!std::is_constructible_v<std::atomic_ref<unsigned>, const std::atomic_ref<int>&>);

int main() {
  alignas(CR::required_alignment) const int c = 42;
  CR r(c);
  CHECK(r.load() == 42);
  CHECK(static_cast<int>(r) == 42);
  r.wait(0);

  alignas(CR::required_alignment) int x = 3;
  std::atomic_ref<int> w(x);
  CR ro = w;
  w.store(4);
  CHECK(ro.load() == 4);
  return 0;
}
