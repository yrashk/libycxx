// dynamic_cast ([expr.dynamic.cast]/8-/10) and handler matching ([except.handle]/3.2: T an
// unambiguous public base of E) over diamonds stacked on top of each other:
//  - virtual: VT<k> derives from VL<k> and VR<k>, both deriving virtually from VT<k-1>, 18
//    levels (2^18 inheritance paths to VT<0>, but each VT<j> occurs once): every cast from the
//    root to any VT<j>, VL<j>, VR<j> succeeds and every handler for them matches; one level
//    inherits privately on one side (still public through the other, [class.paths]/1);
//  - non-virtual: NT<k> derives from NL<k> and NR<k>, both deriving from NT<k-1>, 10 levels
//    (1024 NT<0> subobjects): from a given NT<0> subobject a downcast to NT<j> or to the NL/NR
//    on its path finds the one object derived from it (9.1), a cast to the NL/NR not on its path
//    fails (no such object derived from it, and the type is ambiguous in the most derived
//    object, 9.2), and handlers for the repeated types do not match (ambiguous).
// REQUIRES: exceptions
#include <cstdint>
#include <initializer_list>
#include <typeinfo>
#include <type_traits>
#include "check.hpp"

// ---- virtual ----
template <int K>
struct VT;
template <>
struct VT<0> {
  int v0 = 0;
  virtual ~VT() = default;
};
template <int K>
struct VL : virtual VT<K - 1> {
  int l = K;
};
template <int K>
struct VRB : virtual VT<K - 1> {
  int r = K;
};
template <int K>
struct VRP : private virtual VT<K - 1> {  // private on this side at level 5
  int r = K;
};
template <int K>
using VRight = std::conditional_t<K == 5, VRP<K>, VRB<K>>;
template <int K>
struct VT : VL<K>, VRight<K> {
  int t = K;
};
constexpr int VK = 18;

template <int J>
void virtual_casts(VT<0>* root, VT<VK>* top) {
  if constexpr (J >= 1) {
    CHECK(dynamic_cast<VT<J>*>(root) == static_cast<VT<J>*>(top));
    CHECK(dynamic_cast<VL<J>*>(root) == static_cast<VL<J>*>(top));
    CHECK(dynamic_cast<VRight<J>*>(root) == static_cast<VRight<J>*>(static_cast<VT<J>*>(top)));
    // cross cast between the sides of a diamond
    VL<J>* l = top;
    CHECK(dynamic_cast<VRight<J>*>(l) == static_cast<VRight<J>*>(static_cast<VT<J>*>(top)));
    virtual_casts<J - 1>(root, top);
  }
}

template <int J>
void virtual_handlers(VT<VK>* top) {
  if constexpr (J >= 1) {
    bool ok = false;
    try {
      throw top;
    } catch (VL<J>* p) {
      ok = p == static_cast<VL<J>*>(top);
    }
    CHECK(ok);
    ok = false;
    try {
      throw VT<VK>();
    } catch (const VT<J>& t) {
      ok = t.t == J;
    }
    CHECK(ok);
    virtual_handlers<J - 1>(top);
  }
}

// ---- non-virtual ----
template <int K>
struct NT;
template <>
struct NT<0> {
  int n0 = 0;
  virtual ~NT() = default;
};
template <int K>
struct NL : NT<K - 1> {};
template <int K>
struct NR : NT<K - 1> {};
template <int K>
struct NT : NL<K>, NR<K> {};
constexpr int NK = 10;

// the NT<J> subobject of *p reached by the side choices in mask (bit k-1: level k, 1 = right)
template <int J, int K>
NT<J>* sub(NT<K>* p, unsigned mask) {
  if constexpr (K == J) {
    return p;
  } else {
    NT<K - 1>* next = (mask >> (K - 1)) & 1 ? static_cast<NT<K - 1>*>(static_cast<NR<K>*>(p))
                                            : static_cast<NT<K - 1>*>(static_cast<NL<K>*>(p));
    return sub<J>(next, mask);
  }
}

template <int J>
void nonvirtual_casts(NT<0>* root, NT<NK>* top, unsigned mask) {
  if constexpr (J >= 1) {
    NT<J>* owner = sub<J>(top, mask);
    CHECK(dynamic_cast<NT<J>*>(root) == owner);  // 9.1
    const bool right = (mask >> (J - 1)) & 1;
    NL<J>* l = dynamic_cast<NL<J>*>(root);
    NR<J>* r = dynamic_cast<NR<J>*>(root);
    if (right) {
      CHECK(r == static_cast<NR<J>*>(owner));
      CHECK(l == (J == NK ? static_cast<NL<J>*>(owner) : nullptr));  // unambiguous only at the top
    } else {
      CHECK(l == static_cast<NL<J>*>(owner));
      CHECK(r == (J == NK ? static_cast<NR<J>*>(owner) : nullptr));  // unambiguous only at the top
    }
    nonvirtual_casts<J - 1>(root, top, mask);
  }
}

int main() {
  // virtual
  auto* vt = new VT<VK>;
  VT<0>* vroot = static_cast<VL<1>*>(static_cast<VT<1>*>(vt));
  CHECK(dynamic_cast<void*>(vroot) == static_cast<void*>(vt));
  for (int i = 0; i < 3; ++i) {
    virtual_casts<VK>(vroot, vt);
    virtual_handlers<VK>(vt);
  }
  bool caught = false;
  try {
    throw VT<VK>();
  } catch (const VT<0>& r) {
    caught = typeid(r) == typeid(VT<VK>);
  }
  CHECK(caught);
  delete vt;

  // non-virtual
  auto* nt = new NT<NK>;
  for (unsigned mask : {0u, 1023u, 1u, 512u, 341u, 682u, 777u}) {
    NT<0>* root = sub<0>(nt, mask);
    CHECK(dynamic_cast<NT<NK>*>(root) == nt);
    CHECK(dynamic_cast<void*>(root) == static_cast<void*>(nt));
    nonvirtual_casts<NK>(root, nt, mask);
  }
  int which = 0;
  try {
    throw NT<NK>();
  } catch (const NT<0>&) {  // 1024 such bases: ambiguous
    which = 1;
  } catch (const NT<NK - 1>&) {  // two: ambiguous
    which = 2;
  } catch (const NL<NK>&) {
    which = 3;
  }
  CHECK(which == 3);
  delete nt;
}
