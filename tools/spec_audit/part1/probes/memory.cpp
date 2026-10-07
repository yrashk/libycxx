// [memory], [smartptr], [mem.composite.types]: pointer traits, to_address, align,
// assume_aligned, is_sufficiently_aligned, start_lifetime_as, allocate_at_least, allocator,
// uses-allocator construction, the specialized algorithms (constexpr), unique_ptr, shared_ptr
// and weak_ptr (constexpr, P3037), owner_hash/owner_equal (P1901), out_ptr/inout_ptr, indirect,
// polymorphic.
#include <memory>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

// [pointer.traits], [pointer.conversion], [ptr.align]
static_assert(std::is_same_v<std::pointer_traits<int*>::rebind<long>, long*>);
static_assert(noexcept(std::pointer_traits<int*>::pointer_to(std::declval<int&>())));
static_assert(std::to_address(static_cast<int*>(nullptr)) == nullptr && noexcept(std::to_address(static_cast<int*>(nullptr))));
alignas(16) constexpr int aligned16[4] = {};
static_assert(std::is_same_v<decltype(std::is_sufficiently_aligned<16>(aligned16)), bool>);   // not constexpr ([ptr.align])
static_assert(std::is_same_v<decltype(std::assume_aligned<16>(aligned16)), const int*>);
static_assert(std::is_same_v<decltype(std::align(1, 1, std::declval<void*&>(), std::declval<std::size_t&>())), void*>);
// [obj.lifetime]
static_assert(std::is_same_v<decltype(std::start_lifetime_as<int>(std::declval<void*>())), int*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as<int>(std::declval<const void*>())), const int*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as_array<int>(std::declval<void*>(), 2)), int*>);
static_assert(noexcept(std::start_lifetime_as<int>(std::declval<void*>())));
// [allocator.traits], [default.allocator], [allocator.uses.construction]
static_assert([] {
  std::allocator<int> a;
  auto r = a.allocate_at_least(3);
  bool ok = r.count >= 3;
  a.deallocate(r.ptr, r.count);
  auto t = std::allocator_traits<std::allocator<int>>::allocate_at_least(a, 2);
  a.deallocate(t.ptr, t.count);
  return ok && std::is_same_v<decltype(r), std::allocation_result<int*>>;
}());
static_assert(std::allocator_traits<std::allocator<int>>::is_always_equal::value);
static_assert(std::uses_allocator_v<std::tuple<int>, std::allocator<int>>);
static_assert(std::make_obj_using_allocator<std::pair<int, int>>(std::allocator<int>(), 1, 2).second == 2);
static_assert(std::tuple_size_v<decltype(std::uses_allocator_construction_args<int>(std::allocator<int>(), 1))> == 1);
// [specialized.algorithms]: constexpr construct_at, destroy, uninitialized_* (raw_memory_algorithms)
static_assert([] {
  std::allocator<int> a;
  int* p = a.allocate(3);
  std::uninitialized_fill_n(p, 3, 7);
  std::destroy(p, p + 3);
  std::uninitialized_value_construct_n(p, 3);
  int v = p[1];
  std::ranges::destroy_n(p, 3);
  std::construct_at(p, 9);
  v += *p;
  std::destroy_at(p);
  a.deallocate(p, 3);
  return v == 9;
}());
// [unique.ptr]
static_assert([] { auto p = std::make_unique<int>(3); auto q = std::move(p); return !p && *q == 3; }());
static_assert(std::is_same_v<decltype(std::make_unique_for_overwrite<int[]>(2)), std::unique_ptr<int[]>>);
static_assert(noexcept(std::declval<std::unique_ptr<int>&>().get()) && noexcept(std::declval<std::unique_ptr<int>&>().reset()));
static_assert(!std::is_copy_constructible_v<std::unique_ptr<int>> && sizeof(std::unique_ptr<int>) == sizeof(int*));
static_assert(std::is_same_v<decltype(std::unique_ptr<int>() <=> std::unique_ptr<int>()), std::strong_ordering>);
// [util.smartptr.shared], [util.smartptr.weak]: constexpr (P3037)
static_assert([] {
  auto p = std::make_shared<int>(5);
  std::shared_ptr<int> q = p;
  std::weak_ptr<int> w = q;
  bool ok = p.use_count() == 2 && *w.lock() == 5 && !w.expired();
  q.reset();
  return ok && p.use_count() == 1;
}());
static_assert(std::is_same_v<decltype(std::make_shared_for_overwrite<int[]>(2)), std::shared_ptr<int[]>>);
static_assert(std::is_same_v<decltype(std::allocate_shared<int>(std::allocator<int>(), 1)), std::shared_ptr<int>>);
static_assert(std::is_same_v<decltype(std::declval<std::shared_ptr<int[]>&>()[0]), int&>);
// [util.smartptr.owner.hash], [util.smartptr.owner.equal], owner_* members (P1901)
static_assert(noexcept(std::declval<std::shared_ptr<int>&>().owner_hash()) &&
              std::is_same_v<decltype(std::declval<std::weak_ptr<int>&>().owner_hash()), std::size_t>);
static_assert(noexcept(std::declval<std::shared_ptr<int>&>().owner_equal(std::declval<std::weak_ptr<long>&>())));
static_assert(std::is_invocable_r_v<bool, std::owner_equal, std::shared_ptr<int>, std::weak_ptr<long>>);
static_assert(std::is_invocable_r_v<std::size_t, std::owner_hash, std::weak_ptr<int>>);
static_assert(requires { typename std::owner_equal::is_transparent; typename std::owner_hash::is_transparent; });
// [util.smartptr.enab]
struct Self : std::enable_shared_from_this<Self> {};
static_assert(std::is_same_v<decltype(std::declval<Self&>().weak_from_this()), std::weak_ptr<Self>>);
// [out.ptr], [inout.ptr]
void c_api(int** out);
static_assert(std::is_same_v<decltype(std::out_ptr(std::declval<std::unique_ptr<int>&>())), std::out_ptr_t<std::unique_ptr<int>, int*>>);
static_assert(std::is_same_v<decltype(std::inout_ptr(std::declval<std::unique_ptr<int>&>())), std::inout_ptr_t<std::unique_ptr<int>, int*>>);
static_assert(std::is_convertible_v<std::out_ptr_t<std::unique_ptr<int>, int*>, int**> &&
              std::is_convertible_v<std::out_ptr_t<std::unique_ptr<int>, int*>, void**>);
// [indirect], [polymorphic]
static_assert([] { std::indirect<int> i(3); std::indirect<int> j = i; *j = 4; return *i == 3 && *j == 4; }());
static_assert(std::is_same_v<std::indirect<int>::allocator_type, std::allocator<int>>);
static_assert(std::is_same_v<decltype(std::indirect(1)), std::indirect<int>>);
static_assert(std::is_same_v<decltype(std::indirect<int>() <=> std::indirect<int>()), std::strong_ordering>);
struct Base { virtual constexpr int f() const { return 1; } virtual constexpr ~Base() = default; };
struct Der : Base { constexpr int f() const override { return 2; } };
static_assert([] { std::polymorphic<Base> p(std::in_place_type<Der>); std::polymorphic<Base> q = p; return q->f() == 2; }());
static_assert(noexcept(std::declval<std::polymorphic<Base>&>().valueless_after_move()));
static_assert(std::is_default_constructible_v<std::hash<std::indirect<int>>>);
