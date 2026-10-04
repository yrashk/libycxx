// Exception-injection sweep over the algorithms that may use a temporary buffer (stable_sort,
// stable_partition, inplace_merge, std and ranges forms) and over the uninitialized memory
// algorithms: the comparison/predicate, the element type's constructors and assignments,
// operator new, and source iterators throw at their k-th call, for every k.
//   [res.on.exception.handling]/1 and [algorithms.requirements]: the algorithms impose no
//     guarantee on the values left in the range, but every element object of the range is
//     still alive afterwards (none destroyed, none leaked: the temporary buffer's objects are
//     destroyed and its storage freed), and the function completes normally when nothing throws.
//   [alg.sort]/[stable.sort], [alg.partitions], [alg.merge]: "If enough extra memory is
//     available" bounds the complexity only: a failed buffer allocation may be handled by
//     working without one, so an injected operator new failure may or may not propagate.
//   [specialized.algorithms.general]/2: "if an exception is thrown in the following algorithms,
//     objects constructed by a placement new-expression are destroyed in an unspecified order
//     before allowing the exception to propagate": nothing remains in the destination.
#include <algorithm>
#include <memory>
#include <new>
#include <ranges>
#include "exc_new.hpp"

using namespace exh;

constexpr int N = 24;
static const int init_vals[N] = {5, 3, 9, 1, 7, 3, 8, 2, 6, 4, 0, 9, 11, 13, 12, 10, 15, 14, 3, 1, 17, 16, 19, 18};

template <class E>
struct Arr {
  alignas(E) unsigned char raw[N * sizeof(E)];
  E* p() { return reinterpret_cast<E*>(raw); }
  Arr() {
    for (int i = 0; i < N; ++i) ::new (p() + i) E(init_vals[i]);
  }
  ~Arr() {
    for (int i = 0; i < N; ++i) p()[i].~E();
  }
};

template <class E, class Op>
void algo(const char* name, std::initializer_list<Kind> ks, Op op) {
  for (Kind k : ks) {
    auto scenario = [&] {
      Arr<E> a;
      long live0 = st.live;
      bool threw = attempt([&] { op(a.p(), a.p() + N); });
      EXH_EXPECT(st.live == live0, "elements of the range destroyed or temporaries leaked");
      for (int i = 0; i < N; ++i) a.p()[i].use("element after the algorithm:");
      return threw;
    };
    if (k == gnew)
      sweep_new(name, scenario, options{.may_swallow = true});
    else
      sweep(name, k, new_balanced(scenario));
  }
}

template <class E>
void buffered(const char* ename) {
  static char labels[16][96];
  int li = 0;
  auto L = [&](const char* s) {
    __builtin_snprintf(labels[li], sizeof labels[li], "%s [%s]", s, ename);
    return labels[li++];
  };
  const auto KC = {compare, copy_ctor, move_ctor, copy_assign, move_assign, gnew};
  const auto KP = {pred, copy_ctor, move_ctor, copy_assign, move_assign, gnew};
  algo<E>(L("stable_sort"), KC, [](E* b, E* e) { std::stable_sort(b, e, exh::less{}); });
  algo<E>(L("ranges::stable_sort"), KC, [](E* b, E* e) { std::ranges::stable_sort(b, e, exh::less{}); });
  algo<E>(L("stable_partition"), KP, [](E* b, E* e) { std::stable_partition(b, e, pred_odd{}); });
  algo<E>(L("ranges::stable_partition"), KP, [](E* b, E* e) { std::ranges::stable_partition(b, e, pred_odd{}); });
  // inplace_merge needs two sorted halves: sort them with nothing armed first.
  auto prep = [](E* b, E* e) {
    disarm();
    std::sort(b, b + N / 2);
    std::sort(b + N / 2, e);
  };
  algo<E>(L("inplace_merge"), KC, [prep](E* b, E* e) {
    long saved = st.left[st.kind];
    prep(b, e);
    st.left[st.kind] = saved;
    std::inplace_merge(b, b + N / 2, e, exh::less{});
  });
  algo<E>(L("ranges::inplace_merge"), KC, [prep](E* b, E* e) {
    long saved = st.left[st.kind];
    prep(b, e);
    st.left[st.kind] = saved;
    std::ranges::inplace_merge(b, b + N / 2, e, exh::less{});
  });
  // Non-buffered neighbours, for comparison.
  algo<E>(L("sort"), {compare, move_ctor, move_assign}, [](E* b, E* e) { std::sort(b, e, exh::less{}); });
  algo<E>(L("partial_sort"), {compare, move_ctor, move_assign}, [](E* b, E* e) { std::partial_sort(b, b + 7, e, exh::less{}); });
  algo<E>(L("nth_element"), {compare, move_ctor, move_assign}, [](E* b, E* e) { std::nth_element(b, b + 7, e, exh::less{}); });
  algo<E>(L("rotate"), {move_ctor, move_assign}, [](E* b, E* e) { std::rotate(b, b + 5, e); });
  algo<E>(L("shift_right"), {move_ctor, move_assign}, [](E* b, E* e) { std::shift_right(b, e, 5); });
}

// Uninitialized algorithms writing into raw storage.
template <class Op>
void uninit(const char* name, std::initializer_list<Kind> ks, Op op) {
  static T src[8] = {T(1), T(2), T(3), T(4), T(5), T(6), T(7), T(8)};
  for (Kind k : ks)
    sweep(name, k, new_balanced([&] {
      alignas(T) unsigned char raw[8 * sizeof(T)];
      T* d = reinterpret_cast<T*>(raw);
      long live0 = st.live;
      bool threw = attempt([&] { op(src, d); });
      if (threw)
        EXH_EXPECT(st.live == live0, "[specialized.algorithms.general]/2: constructed objects not destroyed");
      else {
        long made = st.live - live0;
        std::destroy(d, d + made);
      }
      return threw;
    }));
}

int main() {
  buffered<T>("T");
  buffered<NT>("NT");

  const auto KI = {copy_ctor, iter_inc, iter_deref, iter_cmp};
  uninit("uninitialized_copy(input)", KI, [](T* s, T* d) {
    range<in_tag> r{s, s + 8};
    std::uninitialized_copy(r.begin(), r.end(), d);
  });
  uninit("uninitialized_copy_n(input)", KI, [](T* s, T* d) { std::uninitialized_copy_n(iter<in_tag>(s), 8, d); });
  uninit("uninitialized_move", {move_ctor}, [](T* s, T* d) {
    T tmp[8] = {T(1), T(2), T(3), T(4), T(5), T(6), T(7), T(8)};
    (void)s;
    std::uninitialized_move(std::begin(tmp), std::end(tmp), d);
  });
  uninit("uninitialized_fill_n", {copy_ctor}, [](T* s, T* d) { std::uninitialized_fill_n(d, 8, s[0]); });
  uninit("uninitialized_fill", {copy_ctor}, [](T* s, T* d) { std::uninitialized_fill(d, d + 8, s[0]); });
  uninit("uninitialized_value_construct_n", {default_ctor}, [](T*, T* d) { std::uninitialized_value_construct_n(d, 8); });
  uninit("uninitialized_default_construct", {default_ctor}, [](T*, T* d) { std::uninitialized_default_construct(d, d + 8); });
  uninit("ranges::uninitialized_copy(input -> raw)", KI, [](T* s, T* d) {
    range<in_tag> r{s, s + 8};
    std::ranges::uninitialized_copy(r.begin(), r.end(), d, d + 8);
  });
  uninit("ranges::uninitialized_copy_n(input)", KI,
         [](T* s, T* d) { std::ranges::uninitialized_copy_n(iter<in_tag>(s), 8, d, d + 8); });
  uninit("ranges::uninitialized_fill", {copy_ctor}, [](T* s, T* d) { std::ranges::uninitialized_fill(d, d + 8, s[1]); });
  uninit("ranges::uninitialized_value_construct", {default_ctor},
         [](T*, T* d) { std::ranges::uninitialized_value_construct(d, d + 8); });
  uninit("ranges::uninitialized_default_construct_n", {default_ctor},
         [](T*, T* d) { std::ranges::uninitialized_default_construct_n(d, 8); });
  return finish();
}
