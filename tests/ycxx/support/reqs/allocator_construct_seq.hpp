// Generic run-time check, instantiated per allocator-aware sequence container: every object of
// the element type is constructed through allocator_traits<A>::construct and destroyed
// through allocator_traits<A>::destroy, with an allocator equal to get_allocator().
// [container.alloc.reqmts]/2: the element requirements are stated only in terms of
// allocator_traits<A>::construct(m, p, args) (Cpp17DefaultInsertable, Cpp17MoveInsertable,
// Cpp17CopyInsertable, Cpp17EmplaceConstructible) and allocator_traits<A>::destroy(m, p)
// (Cpp17Erasable); Note 2: "A container calls allocator_traits<A>::construct(m, p, args) to
// construct an element at p using args, with m == get_allocator()". So a container may not
// build a T (not even a temporary, e.g. for emplace in the middle) or destroy one other than
// through its allocator: T need not be constructible from args other than through
// construct. [container.reqmts]/64: "A copy of this allocator is used for any memory
// allocation and element construction performed".
//
// Tracked counts constructions and destructions that happen outside ConstructAlloc's
// construct / destroy.
#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <ranges>
#include <type_traits>
#include <utility>

namespace reqs::allocator_construct_seq {

struct Counters {
  int construct_depth = 0, destroy_depth = 0;
  int outside_constructs = 0, outside_destroys = 0;
  int constructs = 0, destroys = 0;
  long live = 0;
  int expected_outside = 0;  // Tracked objects the test itself creates and destroys
};
inline Counters counters;

// The test passes Val (not Tracked) wherever it can, so that every Tracked object is made by
// the container; the few Tracked objects the test makes itself (initializer lists) are
// accounted for in Counters::expected_outside.
struct Val {
  int v;
};

struct Tracked {
  int v = 0;
  static void born() {
    if (counters.construct_depth == 0) ++counters.outside_constructs;
    ++counters.live;
  }
  Tracked() { born(); }
  Tracked(int x) : v(x) { born(); }
  Tracked(Val x) : v(x.v) { born(); }
  Tracked(const Tracked& o) : v(o.v) { born(); }
  Tracked(Tracked&& o) noexcept : v(o.v) { born(); }
  Tracked& operator=(const Tracked&) = default;
  Tracked& operator=(Tracked&&) noexcept = default;
  Tracked& operator=(Val x) {  // so that assigning from a Val range makes no temporary
    v = x.v;
    return *this;
  }
  ~Tracked() {
    if (counters.destroy_depth == 0) ++counters.outside_destroys;
    --counters.live;
  }
  friend bool operator==(const Tracked& a, const Tracked& b) { return a.v == b.v; }
  friend auto operator<=>(const Tracked& a, const Tracked& b) { return a.v <=> b.v; }
};

template <class T>
struct ConstructAlloc {
  using value_type = T;
  using is_always_equal = std::false_type;
  int id = 0;
  ConstructAlloc() = default;
  explicit ConstructAlloc(int i) : id(i) {}
  template <class U>
  ConstructAlloc(const ConstructAlloc<U>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    struct Guard {
      ~Guard() { --counters.construct_depth; }
    } g;
    ++counters.construct_depth;
    ++counters.constructs;
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }
  template <class U>
  void destroy(U* p) {
    struct Guard {
      ~Guard() { --counters.destroy_depth; }
    } g;
    ++counters.destroy_depth;
    ++counters.destroys;
    p->~U();
  }
  template <class U>
  friend bool operator==(const ConstructAlloc& a, const ConstructAlloc<U>& b) noexcept {
    return a.id == b.id;
  }
};

template <class X>
constexpr bool is_forward_list = requires(X& x) { x.before_begin(); };

template <class X>
std::ptrdiff_t length(const X& x) {
  std::ptrdiff_t n = 0;
  for (auto it = x.begin(); it != x.end(); ++it) ++n;
  return n;
}

template <class X>
auto at(X& x, int k) {  // const_iterator before which insert puts element k
  if constexpr (is_forward_list<X>) {
    auto p = x.cbefore_begin();
    for (int i = 0; i < k; ++i) ++p;
    return p;
  } else {
    return std::next(x.cbegin(), k);
  }
}

// Runs the operations; returns false if a result is wrong. The caller checks the counters.
template <class X>
bool operations() {
  using A = typename X::allocator_type;
  using T = Tracked;
  Val arr[5] = {{1}, {2}, {3}, {4}, {5}};
  X src(arr, arr + 5, A(1));  // lvalue elements to pass as const T& / T&&
  auto elem = [&](int k) -> T& { return *std::next(src.begin(), k); };
  X a{A(1)};
  X n(4, A(1));
  X nt(3, elem(0), A(1));
  X fr(std::from_range, arr, A(1));
  counters.expected_outside += 2;
  X il({T(1), T(2)}, A(1));  // the initializer_list's own two elements
  X cp(src);
  X cpa(src, A(2));
  X mv(std::move(cp));
  X mva(std::move(cpa), A(3));  // unequal: element-wise
  if (length(mva) != 5 || length(n) != 4 || length(nt) != 3 || length(fr) != 5 || length(il) != 2) return false;
  a.assign(3, elem(1));
  a.assign(arr, arr + 4);
  a.assign_range(arr);
  counters.expected_outside += 1;
  a.assign({T(6)});
  counters.expected_outside += 3;
  a = {T(1), T(2), T(3)};
  a = src;
  a = il;
  a = std::move(mva);  // unequal, not propagating: element-wise
  if (length(a) != 5) return false;
  counters.expected_outside += 2;
  if constexpr (is_forward_list<X>) {
    a.emplace_after(at(a, 2), 10);
    a.emplace_after(at(a, 1), *std::next(a.begin(), 3));  // argument aliases an element
    a.insert_after(at(a, 1), elem(0));
    a.insert_after(at(a, 1), std::move(elem(1)));
    a.insert_after(at(a, 3), 2, elem(2));
    a.insert_after(at(a, 1), arr, arr + 3);
    a.insert_after(at(a, 4), {T(13), T(14)});
    a.insert_range_after(at(a, 2), arr);
    a.erase_after(at(a, 1));
    a.erase_after(at(a, 0), std::next(a.cbegin(), 3));
  } else {
    a.emplace(at(a, 2), 10);
    a.emplace(at(a, 1), *std::next(a.begin(), 3));  // argument aliases an element
    a.insert(at(a, 1), elem(0));
    a.insert(at(a, 1), std::move(elem(1)));
    a.insert(at(a, 3), 2, elem(2));
    a.insert(at(a, 1), arr, arr + 3);
    a.insert(at(a, 4), {T(13), T(14)});
    a.insert_range(at(a, 2), arr);
    a.erase(at(a, 1));
    a.erase(at(a, 0), at(a, 2));
  }
  if constexpr (requires { a.push_back(elem(0)); }) {
    a.push_back(elem(3));
    a.push_back(std::move(elem(4)));
    a.emplace_back(16);
    a.emplace_back(a.front());
    a.append_range(arr);
    a.pop_back();
  }
  if constexpr (requires { a.push_front(elem(0)); }) {
    a.push_front(elem(3));
    a.push_front(std::move(elem(2)));
    a.emplace_front(18);
    a.emplace_front(*std::next(a.begin(), 2));
    a.prepend_range(arr);
    a.pop_front();
  }
  if constexpr (requires { a.reserve(1); }) {
    a.reserve(200);  // relocation
    a.shrink_to_fit();
  }
  a.resize(60);
  a.resize(70, elem(0));
  a.resize(20);
  std::erase_if(a, [](const T& t) { return t.v == 5; });
  std::erase(a, elem(0));
  if constexpr (requires { a.sort(); }) {
    a.sort();
    a.unique();
    a.remove(elem(1));
    a.remove_if([](const T& t) { return t.v == 2; });
    X b(arr, arr + 5, A(1));
    a.merge(b);
    a.reverse();
    X c(arr, arr + 2, A(1));
    if constexpr (is_forward_list<X>)
      a.splice_after(a.cbefore_begin(), c);
    else
      a.splice(a.cbegin(), c);
  }
  X s(arr, arr + 2, A(1));
  a.swap(s);  // equal allocators
  a.clear();
  return a.begin() == a.end();
}

template <class X>
bool test() {
  counters = Counters();
  bool ok = operations<X>();
  return ok && counters.outside_constructs == counters.expected_outside &&
         counters.outside_destroys == counters.expected_outside && counters.live == 0 &&
         counters.constructs > 100 && counters.constructs == counters.destroys;
}

}  // namespace reqs::allocator_construct_seq
