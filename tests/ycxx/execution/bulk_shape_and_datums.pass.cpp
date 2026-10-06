// [exec.bulk]/2: Shape is decltype(auto(shape)) (an lvalue or const shape is decayed);
// /9.1.1: bulk and bulk_unchunked call f(i, args...) with i of type Shape for every i from 0 to
// shape, bulk_chunked calls f(b, e, args...) with b and e of type Shape covering [0, shape)
// once; /9.1: args are lvalues referring to the child's value datums, or decayed copies of them
// when they are copy_constructible, so a move-only datum is passed by lvalue reference to the
// object that is then sent on.
#include <execution>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

template <class Shape>
void check_shape(const Shape& n) {
  // bulk
  {
    std::vector<int> hits(n, 0);
    record<int> rec;
    run(ex::just() | ex::bulk(ex::seq, n, [&](auto i) {
          static_assert(std::is_same_v<decltype(i), Shape>);
          hits[static_cast<std::size_t>(i)] += 1;
        }),
        receiver_for(rec));
    CHECK(rec.how == done::value);
    for (int h : hits)
      CHECK(h == 1);
  }
  // bulk_unchunked
  {
    std::vector<int> hits(n, 0);
    record<int> rec;
    run(ex::just() | ex::bulk_unchunked(ex::seq, n, [&](auto i) {
          static_assert(std::is_same_v<decltype(i), Shape>);
          hits[static_cast<std::size_t>(i)] += 1;
        }),
        receiver_for(rec));
    CHECK(rec.how == done::value);
    for (int h : hits)
      CHECK(h == 1);
  }
  // bulk_chunked
  {
    std::vector<int> hits(n, 0);
    record<int> rec;
    run(ex::just() | ex::bulk_chunked(ex::seq, n, [&](auto b, auto e) {
          static_assert(std::is_same_v<decltype(b), Shape> && std::is_same_v<decltype(e), Shape>);
          CHECK(Shape(0) <= b && b < e && e <= n);
          for (auto i = b; i != e; ++i)
            hits[static_cast<std::size_t>(i)] += 1;
        }),
        receiver_for(rec));
    CHECK(rec.how == done::value);
    for (int h : hits)
      CHECK(h == 1);
  }
}

int main() {
  const short s = 5;
  check_shape(s);
  check_shape(static_cast<unsigned char>(3));
  check_shape(7ull);
  check_shape(1L);

  // A move-only datum: f gets an lvalue referring to it, and that object is sent on.
  {
    int* seen = nullptr;
    int calls = 0;
    auto p = std::make_unique<int>(4);
    int* const orig = p.get();
    record<int, std::unique_ptr<int>> rec;
    run(ex::just(std::move(p)) | ex::bulk(ex::seq, 3, [&](int, std::unique_ptr<int>& q) {
          ++calls;
          seen = q.get();
        }),
        receiver_for(rec));
    CHECK(rec.how == done::value && calls == 3 && seen == orig);
    CHECK(std::get<0>(*rec.values).get() == orig && *std::get<0>(*rec.values) == 4);
  }
  {
    int calls = 0;
    auto p = std::make_unique<int>(5);
    int* const orig = p.get();
    record<int, std::unique_ptr<int>> rec;
    run(ex::just(std::move(p)) | ex::bulk_chunked(ex::seq, 3, [&](int b, int e, std::unique_ptr<int>& q) {
          calls += e - b;
          CHECK(q.get() == orig);
        }),
        receiver_for(rec));
    CHECK(rec.how == done::value && calls == 3 && std::get<0>(*rec.values).get() == orig);
  }
  {
    int calls = 0;
    auto p = std::make_unique<int>(6);
    int* const orig = p.get();
    record<int, std::unique_ptr<int>> rec;
    run(ex::just(std::move(p)) | ex::bulk_unchunked(ex::seq, 2, [&](int, std::unique_ptr<int>& q) {
          ++calls;
          CHECK(q.get() == orig);
        }),
        receiver_for(rec));
    CHECK(rec.how == done::value && calls == 2 && std::get<0>(*rec.values).get() == orig);
  }
  return 0;
}
