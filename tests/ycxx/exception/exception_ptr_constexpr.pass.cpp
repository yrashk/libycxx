// [propagation]/8: "All member functions [of exception_ptr] are marked constexpr."
// [exception.syn]: rethrow_exception, make_exception_ptr and exception_ptr_cast are
// constexpr, so they are usable during constant evaluation (P3068).
// XFAIL-COMPILER: clang  no constexpr exception support (P3068) in clang yet
// REQUIRES: exceptions
#include <exception>
#include <optional>
#include "check.hpp"

struct Err {
  int code;
};

constexpr bool test() {
  std::exception_ptr null;
  if (null) return false;
  if (!(null == nullptr)) return false;
  std::exception_ptr p = std::make_exception_ptr(Err{5});
  if (!p) return false;
  std::exception_ptr q = p;
  if (q != p) return false;
  auto o = std::exception_ptr_cast<Err>(p);
  if (!o || o->code != 5) return false;
  if (std::exception_ptr_cast<int>(p)) return false;
  int got = 0;
  try {
    std::rethrow_exception(q);
  } catch (const Err& e) {
    got = e.code;
  }
  if (got != 5) return false;
  q = nullptr;
  if (q) return false;
  std::exception_ptr m = std::move(p);
  return static_cast<bool>(m);
}
static_assert(test());

constexpr std::exception_ptr empty{};
static_assert(empty == nullptr);

int main() {
  CHECK(test());
  return 0;
}
