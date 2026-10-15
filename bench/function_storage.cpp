// Closure-storage boundaries and the cost of keeping callbacks in collections.
// Builds unchanged with each library; benchmark the same source before/after changing SBO.
#include "bench.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr int count = 20000;
using callback = std::function<int(int)>;

template <class F>
void construction(const char* name, const F& target) {
  bench::run(name, count, [&] {
    for (int i = 0; i < count; ++i) {
      callback f(target);
      bench::sink(f);
    }
  });
}

template <class F>
void collection(const char* create, const char* copy, const char* call, const F& target) {
  constexpr int size = 16384;
  bench::run(create, size, [&] {
    std::vector<callback> callbacks;
    callbacks.reserve(size);
    for (int i = 0; i < size; ++i) callbacks.emplace_back(target);
    bench::sink(callbacks);
  });
  std::vector<callback> callbacks(size, callback(target));
  bench::run(copy, size, [&] {
    auto other = callbacks;
    bench::sink(other);
  });
  bench::run(call, size, [&] {
    int sum = 0;
    for (auto& f : callbacks) sum += (*bench::opaque(&f))(1);
    bench::sink(sum);
  });
}

template <std::size_t Words>
auto payload() {
  std::array<std::uintptr_t, Words> data{};
  for (std::size_t i = 0; i < Words; ++i) data[i] = i + 1;
  return [data](int n) {
    for (auto value : data) n += static_cast<int>(value);
    return n;
  };
}

struct alignas(16) aligned_target {
  std::uint64_t a = 1, b = 2;
  int operator()(int n) const { return n + static_cast<int>(a + b); }
};
} // namespace

int main(int argc, char** argv) {
  bench::init(argc, argv);
  auto small = payload<1>();
  auto three = payload<3>();
  auto four = payload<4>();
  auto six = payload<6>();
  construction("function storage: construct 1 word", small);
  construction("function storage: construct 3 words", three);
  construction("function storage: construct 4 words", four);
  construction("function storage: construct 6 words", six);
  construction("function storage: construct align16", aligned_target{});

  auto p = std::make_shared<int>(1), q = std::make_shared<int>(2);
  construction("function storage: construct 2 shared_ptrs", [p, q](int n) { return n + *p + *q; });
  construction("function storage: construct short string", [s = std::string("hello")](int n) { return n + static_cast<int>(s.size()); });
  construction("function storage: construct long string", [s = std::string(100, 'x')](int n) { return n + static_cast<int>(s.size()); });

  collection("function storage: vector build 1 word", "function storage: vector copy 1 word",
             "function storage: vector call 1 word", small);
  collection("function storage: vector build 4 words", "function storage: vector copy 4 words",
             "function storage: vector call 4 words", four);

  callback source(four);
  bench::run("function storage: move 4 words (round trip)", 2 * count, [&] {
    for (int i = 0; i < count; ++i) {
      callback next(std::move(source));
      bench::sink(next);
      source = std::move(next);
      bench::sink(source);
    }
  });
}
