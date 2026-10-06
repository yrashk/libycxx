// [linalg.helpers.concepts]/1: the algorithms are constrained by the exposition-only concepts:
// in-vector / in-matrix are rank-1 / rank-2 mdspans; out-* and inout-* also need an assignable
// reference and is_always_unique(); possibly-packed-out-matrix accepts a layout_blas_packed
// output (not unique) too; scalar is semiregular, not an mdspan and not an execution policy.
// [linalg.algs.blas1.copy]/2, [linalg.algs.blas1.add]/2, [linalg.algs.blas1.swap]/2: the ranks
// must agree (Constraints). [linalg.algs.reqs]/1.1: an ExecutionPolicy parameter must satisfy
// is_execution_policy. Each failure is a substitution failure, observable with requires.
#include <linalg>
#include <array>
#include <execution>
#include <mdspan>
#include <utility>

namespace la = std::linalg;
using std::extents;

using V = std::mdspan<double, extents<int, 3>>;
using CV = std::mdspan<const double, extents<int, 3>>;
using M = std::mdspan<double, extents<int, 3, 3>>;
using CM = std::mdspan<const double, extents<int, 3, 3>>;
using SM = std::mdspan<double, extents<int, 3, 3>, std::layout_stride>;  // unique, not exhaustive
using PM = std::mdspan<double, extents<int, 3, 3>, la::layout_blas_packed<la::upper_triangle_t, la::row_major_t>>;
using V3 = std::mdspan<double, extents<int, 3, 3, 3>>;
using SV = decltype(la::scaled(2.0, std::declval<V>()));  // read-only
using std::execution::parallel_policy, std::execution::sequenced_policy;
using std::execution::parallel_unsequenced_policy;

template <class X, class Y>
concept can_copy = requires(X x, Y y) { la::copy(x, y); };
template <class P, class X, class Y>
concept can_copy_policy = requires(P&& p, X x, Y y) { la::copy(std::forward<P>(p), x, y); };
template <class X, class Y, class Z>
concept can_add = requires(X x, Y y, Z z) { la::add(x, y, z); };
template <class X, class Y>
concept can_swap = requires(X x, Y y) { la::swap_elements(x, y); };
template <class S, class X>
concept can_scale = requires(S s, X x) { la::scale(s, x); };
template <class X, class Y, class I>
concept can_dot = requires(X x, Y y, I i) { la::dot(x, y, i); };
template <class X, class Y>
concept can_dot2 = requires(X x, Y y) { la::dot(x, y); };
template <class P, class X, class Y>
concept can_dot_policy = requires(P&& p, X x, Y y) { la::dot(std::forward<P>(p), x, y); };
template <class A, class B, class C>
concept can_gemm = requires(A a, B b, C c) { la::matrix_product(a, b, c); };
template <class X, class A>
concept can_syr = requires(X x, A a) { la::symmetric_matrix_rank_1_update(1.0, x, a, la::upper_triangle); };
template <class X, class Y, class A>
concept can_ger = requires(X x, Y y, A a) { la::matrix_rank_1_update(x, y, a); };
template <class A, class X, class Y>
concept can_gemv = requires(A a, X x, Y y) { la::matrix_vector_product(a, x, y); };

// copy: in-object -> out-object of the same rank
static_assert(can_copy<V, V> && can_copy<CV, V> && can_copy<SV, V>);
static_assert(!can_copy<V, CV> && !can_copy<V, SV>);
static_assert(!can_copy<V, M> && !can_copy<M, V>);  // ranks differ
static_assert(!can_copy<V3, V3>);                   // rank 3 is not an object
static_assert(can_copy<M, SM> && can_copy<SM, M>);  // layout_stride is always unique
static_assert(!can_copy<M, PM>);                    // packed is not unique
static_assert(can_copy<PM, M>);                     // but may be read
static_assert(!can_copy<std::array<double, 3>, V>);

// add, swap_elements, scale
static_assert(can_add<V, CV, V> && !can_add<V, M, V> && !can_add<V, V, CV> && can_add<M, CM, M>);
static_assert(can_swap<V, V> && !can_swap<V, CV> && !can_swap<M, V> && !can_swap<PM, M>);
static_assert(can_scale<double, V> && can_scale<int, M> && !can_scale<double, CV>);
static_assert(!can_scale<V, V>);  // an mdspan is not a scalar

// dot: the init argument must be a scalar
static_assert(can_dot<V, CV, double> && !can_dot<V, CV, V> && !can_dot<V, CV, parallel_policy>);
static_assert(!can_dot2<M, V> && can_dot2<SV, CV>);

// matrix_product and the rank-1 updates: out-matrix vs possibly-packed-out-matrix
static_assert(can_gemm<CM, CM, M> && can_gemm<CM, CM, SM>);
static_assert(!can_gemm<CM, CM, CM> && !can_gemm<CM, CM, PM>);
static_assert(can_syr<CV, PM> && can_syr<CV, M> && !can_syr<CV, CM> && !can_syr<CM, M>);
static_assert(can_ger<CV, CV, M> && !can_ger<CV, CV, PM> && !can_ger<CV, CV, CM>);

// matrix_vector_product: rank checks on every operand
static_assert(can_gemv<CM, CV, V> && !can_gemv<CV, CV, V> && !can_gemv<CM, CM, V> && !can_gemv<CM, CV, M>);

// the execution-policy overloads (with rvalue policies: /1.1 names
// is_execution_policy<ExecutionPolicy>, which a deduced lvalue reference type would not satisfy)
static_assert(can_copy_policy<parallel_policy, V, V> && can_copy_policy<sequenced_policy, CV, V>);
static_assert(!can_copy_policy<parallel_policy, V, CV>);
static_assert(can_dot_policy<parallel_unsequenced_policy, CV, CV>);
static_assert(!can_copy_policy<V, V, V>);  // an mdspan is not a policy
static_assert(!can_copy_policy<int, V, V>);

int main() {}
