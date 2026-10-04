// [except.nested]/8: throw_with_nested(t) throws, "If is_class_v<U> && !is_final_v<U> &&
// !is_base_of_v<nested_exception, U> is true, an exception of unspecified type that is
// publicly derived from both U and nested_exception and constructed from std::forward<T>(t),
// otherwise std::forward<T>(t)." In the "otherwise" cases the exception object has exactly
// type U (observable with typeid on a polymorphic U). is_base_of_v is true for private and
// ambiguous bases too, and for nested_exception itself. In the first case the dynamic type
// differs from U.
#include <exception>
#include <typeinfo>
#include "check.hpp"

struct Poly {
  int v = 1;
  virtual ~Poly() = default;
};
struct FinalPoly final {
  int v = 2;
  virtual ~FinalPoly() = default;
};
struct PublicNested : std::nested_exception {
  int v = 3;
};
struct PrivateNested : private std::nested_exception {
  int v = 4;
  const std::nested_exception& base() const { return *this; }
};
struct NA : std::nested_exception {};
struct NB : std::nested_exception {};
struct AmbiguousNested : NA, NB {
  int v = 5;
};

int main() {
  // wrapped: dynamic type is not Poly, and it is a nested_exception holding the handled one
  try {
    try {
      throw 10;
    } catch (...) {
      std::throw_with_nested(Poly());
    }
  } catch (const Poly& p) {
    CHECK(typeid(p) != typeid(Poly));
    CHECK(p.v == 1);
    const auto* n = dynamic_cast<const std::nested_exception*>(&p);
    CHECK(n != nullptr && n->nested_ptr() != nullptr);
  }

  // final: thrown as-is
  bool hit = false;
  try {
    try {
      throw 10;
    } catch (...) {
      std::throw_with_nested(FinalPoly());
    }
  } catch (const std::nested_exception&) {
    CHECK(false);
  } catch (const FinalPoly& f) {
    hit = typeid(f) == typeid(FinalPoly) && f.v == 2;
  }
  CHECK(hit);

  // already publicly derived from nested_exception: thrown as-is, keeps its own capture
  hit = false;
  try {
    PublicNested made_outside;  // captures nothing
    try {
      throw 10;
    } catch (...) {
      std::throw_with_nested(made_outside);
    }
  } catch (const PublicNested& p) {
    hit = typeid(p) == typeid(PublicNested) && p.v == 3 && p.nested_ptr() == nullptr;
  }
  CHECK(hit);

  // privately derived from nested_exception: is_base_of_v is true, so thrown as-is
  hit = false;
  try {
    try {
      throw 10;
    } catch (...) {
      std::throw_with_nested(PrivateNested());  // the temporary captures 10 itself
    }
  } catch (const std::nested_exception&) {
    CHECK(false);  // a private base does not match
  } catch (const PrivateNested& p) {
    hit = typeid(p) == typeid(PrivateNested) && p.v == 4 && p.base().nested_ptr() != nullptr;
  }
  CHECK(hit);

  // ambiguous nested_exception bases: still is_base_of_v, thrown as-is
  hit = false;
  try {
    std::throw_with_nested(AmbiguousNested());
  } catch (const AmbiguousNested& a) {
    hit = typeid(a) == typeid(AmbiguousNested) && a.v == 5;
  }
  CHECK(hit);

  // nested_exception itself
  hit = false;
  try {
    try {
      throw 10;
    } catch (...) {
      std::nested_exception ne;  // captures 10
      std::throw_with_nested(ne);
    }
  } catch (const std::nested_exception& ne) {
    hit = typeid(ne) == typeid(std::nested_exception);
    int inner = 0;
    try {
      ne.rethrow_nested();
    } catch (int i) {
      inner = i;
    }
    hit = hit && inner == 10;
  }
  CHECK(hit);
  return 0;
}
