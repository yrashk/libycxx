// [util.smartptr.atomic.shared], [util.smartptr.atomic.weak]: the class synopses: value_type;
// "static constexpr bool is_always_lock_free"; is_lock_free() const noexcept; the constexpr
// noexcept default constructor (/1) and atomic(nullptr_t) (so constant initialization is
// possible, [basic.start.static]); no copy construction or copy assignment; every operation
// noexcept. [util.smartptr.atomic.general]/1: "The template parameter T of these partial
// specializations may be an incomplete type." [memory.syn] declares the partial
// specializations in <memory> (Note 1 of [util.smartptr.atomic.general]).
#include <memory>
#include <atomic>
#include <type_traits>
#include <utility>

using SA = std::atomic<std::shared_ptr<int>>;
using WA = std::atomic<std::weak_ptr<int>>;

static_assert(std::is_same_v<SA::value_type, std::shared_ptr<int>>);
static_assert(std::is_same_v<WA::value_type, std::weak_ptr<int>>);
static_assert(std::is_same_v<decltype(SA::is_always_lock_free), const bool>);
static_assert(std::is_same_v<decltype(WA::is_always_lock_free), const bool>);
constexpr bool use_s = SA::is_always_lock_free, use_w = WA::is_always_lock_free;  // constant
static_assert(use_s || !use_s);
static_assert(use_w || !use_w);

static_assert(std::is_nothrow_default_constructible_v<SA>);
static_assert(std::is_nothrow_default_constructible_v<WA>);
static_assert(std::is_nothrow_constructible_v<SA, std::nullptr_t>);
static_assert(std::is_nothrow_constructible_v<SA, std::shared_ptr<int>>);
static_assert(std::is_nothrow_constructible_v<WA, std::weak_ptr<int>>);
static_assert(!std::is_copy_constructible_v<SA> && !std::is_copy_assignable_v<SA>);
static_assert(!std::is_copy_constructible_v<WA> && !std::is_copy_assignable_v<WA>);
static_assert(!std::is_move_constructible_v<SA> && !std::is_move_assignable_v<SA>);
static_assert(!std::is_move_constructible_v<WA> && !std::is_move_assignable_v<WA>);

constinit SA global_s;
constinit SA global_n{nullptr};
constinit WA global_w;

template<class A, class P>
void check_noexcept(A& a, const A& ca, P& p) {
  static_assert(noexcept(ca.is_lock_free()));
  static_assert(std::is_same_v<decltype(ca.is_lock_free()), bool>);
  static_assert(noexcept(ca.load()) && noexcept(ca.load(std::memory_order::acquire)));
  static_assert(std::is_same_v<decltype(ca.load()), P>);
  static_assert(noexcept(static_cast<P>(ca)));
  static_assert(noexcept(a.store(p)) && noexcept(a.store(p, std::memory_order::release)));
  static_assert(std::is_same_v<decltype(a.store(p)), void>);
  static_assert(noexcept(a = p));
  static_assert(std::is_same_v<decltype(a = p), void>);
  static_assert(noexcept(a.exchange(p)) && std::is_same_v<decltype(a.exchange(p)), P>);
  static_assert(noexcept(a.compare_exchange_weak(p, p)));
  static_assert(noexcept(a.compare_exchange_strong(p, p)));
  static_assert(noexcept(a.compare_exchange_weak(p, p, std::memory_order::acq_rel)));
  static_assert(noexcept(a.compare_exchange_strong(p, p, std::memory_order::acq_rel, std::memory_order::acquire)));
  static_assert(std::is_same_v<decltype(a.compare_exchange_strong(p, p)), bool>);
  static_assert(noexcept(ca.wait(p)) && noexcept(ca.wait(p, std::memory_order::relaxed)));
  static_assert(noexcept(a.notify_one()) && noexcept(a.notify_all()));
}
template void check_noexcept<SA, std::shared_ptr<int>>(SA&, const SA&, std::shared_ptr<int>&);
template void check_noexcept<WA, std::weak_ptr<int>>(WA&, const WA&, std::weak_ptr<int>&);

static_assert(noexcept(std::declval<SA&>() = nullptr));
static_assert(std::is_same_v<decltype(std::declval<SA&>() = nullptr), void>);

// incomplete T
struct Incomplete;
struct Holder {
  std::atomic<std::shared_ptr<Incomplete>> s;
  std::atomic<std::weak_ptr<Incomplete>> w;
  std::shared_ptr<Incomplete> get() const { return s.load(); }
  void put(std::shared_ptr<Incomplete> p) { w = p; s.store(std::move(p)); }
};

// the draft's own example, [util.smartptr.atomic.general]/3
template<typename T> class atomic_list {
  struct node {
    T t;
    std::shared_ptr<node> next;
  };
  std::atomic<std::shared_ptr<node>> head;
public:
  std::shared_ptr<node> find(T t) const {
    auto p = head.load();
    while (p && p->t != t)
      p = p->next;
    return p;
  }
  void push_front(T t) {
    auto p = std::make_shared<node>();
    p->t = t;
    p->next = head;
    while (!head.compare_exchange_weak(p->next, p)) {}
  }
};
template class atomic_list<int>;
