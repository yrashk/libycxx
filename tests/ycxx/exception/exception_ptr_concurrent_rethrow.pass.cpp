// Several threads rethrow the same exception_ptr objects concurrently and throw their own
// exceptions of different types in between, all through the same handler lists.
// [propagation]/7: exception_ptr copies may be used concurrently ("Changes in the number of
// exception_ptr objects that refer to a particular exception object do not introduce a data
// race"); /10: rethrow_exception(p) throws the exception object p refers to, or a copy (Note:
// unspecified which); [except.handle]/3: the handler that matches depends only on the type of
// the exception object, so each thread must see the same match results for each type however
// the threads interleave.
// FLAGS: -pthread
#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include "check.hpp"

struct A {
  int a = 0;
  virtual ~A() = default;
};
struct B {
  int b = 0;
  virtual ~B() = default;
};
struct D1 : A, B {
  D1() { a = 1; b = 11; }
};
struct D2 : B, A {
  long pad[4] = {};
  D2() { a = 2; b = 12; }
};
struct Amb : D1, D2 {};
struct Err : std::logic_error {
  int code;
  explicit Err(int c) : std::logic_error("err " + std::to_string(c)), code(c) {}
};

// result encodes handler and payload
static int classify(void (*f)(int), int arg) {
  try {
    f(arg);
  } catch (const Err& e) {
    return 1000 + e.code;
  } catch (const std::exception& e) {
    return 2000 + static_cast<int>(std::string(e.what()).size());
  } catch (A& x) {
    return 3000 + x.a;
  } catch (const B& x) {
    return 4000 + x.b;
  } catch (int i) {
    return 5000 + i;
  } catch (...) {
    return 6000;
  }
  return 0;
}

static std::vector<std::exception_ptr> shared;
static void rethrow_shared(int i) { std::rethrow_exception(shared[static_cast<unsigned>(i)]); }
static void throw_own(int i) {
  switch (i % 6) {
    case 0: throw D1();
    case 1: throw D2();
    case 2: throw Amb();
    case 3: throw Err(i);
    case 4: throw i;
    default: throw std::runtime_error(std::string(static_cast<unsigned>(i % 7), 'x'));
  }
}
static int expected_own(int i) {
  switch (i % 6) {
    case 0: return 3001;
    case 1: return 3002;
    case 2: return 6000;
    case 3: return 1000 + i;
    case 4: return 5000 + i;
    default: return 2000 + i % 7;
  }
}

int main() {
  std::vector<int> expected_shared;
  for (int i = 0; i < 24; ++i) {
    try {
      throw_own(i);
    } catch (...) {
      shared.push_back(std::current_exception());
    }
    expected_shared.push_back(expected_own(i));
  }
  // exception objects of type B, one of them copied from a D2 subobject
  shared.push_back(std::make_exception_ptr(B()));
  expected_shared.push_back(4000);
  shared.push_back(std::make_exception_ptr(static_cast<const B&>(D2())));  // sliced copy: a B
  expected_shared.push_back(4012);

  std::atomic<int> failures{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < 8; ++t) {
    threads.emplace_back([t, &failures, &expected_shared] {
      unsigned x = 17u + static_cast<unsigned>(t);
      for (int k = 0; k < 4000; ++k) {
        x = x * 1103515245u + 12345u;
        int i = static_cast<int>((x >> 16) % shared.size());
        if (classify(rethrow_shared, i) != expected_shared[static_cast<unsigned>(i)]) ++failures;
        std::exception_ptr copy = shared[static_cast<unsigned>(i)];  // copies race with rethrows
        int j = static_cast<int>((x >> 8) % 97);
        if (classify(throw_own, j) != expected_own(j)) ++failures;
        if (copy != shared[static_cast<unsigned>(i)]) ++failures;
      }
    });
  }
  for (auto& th : threads) th.join();
  CHECK(failures == 0);
  return 0;
}
