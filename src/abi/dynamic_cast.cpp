// libycxx ABI runtime: __dynamic_cast (Itanium C++ ABI §2.9.7, [expr.dynamic.cast]/9).
#include "rtti.hpp"

using namespace __cxxabiv1;
using ycxx::abi::base_search;
using ycxx::abi::subobject;

namespace {

// [expr.dynamic.cast]/9.1: among the dst objects within the most derived object, those that
// contain the source subobject (type src at address sub) as a base, counting distinct ones.
// The cast succeeds when there is exactly one and the source is a public base of it.
base_search containing_objects(const subobject& mdo, const __class_type_info& src, const char* sub,
                               const __class_type_info& dst) {
  base_search r;
  auto visit_candidate = [&](const subobject& c) {
    if (!(*c.type == dst))
      return false;
    // Search c's own bases, with path accessibility measured from c.
    bool contains = false;
    bool via_public = false;
    auto visit_source = [&](const subobject& s) {
      if (s.addr == sub && *s.type == src) {
        contains = true;
        via_public = via_public || s.is_public;
        return via_public; // a public path settles it
      }
      return false;
    };
    ycxx::abi::walk_bases(subobject{c.type, c.addr, c.anchor, c.offset, true}, visit_source);
    if (!contains)
      return false;
    if (r.count == 0) {
      r.count = 1;
      r.first = c;
      r.is_public = via_public;
    } else if (r.first.addr == c.addr) {
      r.is_public = r.is_public || via_public;
    } else {
      r.count = 2;
      return true;
    }
    return false;
  };
  ycxx::abi::walk_bases(mdo, visit_candidate);
  return r;
}

// Whether the source subobject is a public base class subobject of the most derived object.
bool is_public_base(const subobject& mdo, const __class_type_info& src, const char* sub) {
  auto visit = [&](const subobject& s) { return s.is_public && s.addr == sub && *s.type == src; };
  return ycxx::abi::walk_bases(mdo, visit);
}

} // namespace

// sub:  the address of a polymorphic subobject of static type src (non-null).
// dst:  the class T of dynamic_cast<T*> / dynamic_cast<T&>.
// src2dst_offset: the compiler's static hint (§2.9.7): >= 0 when src is a unique public
//       non-virtual base of dst at that offset; -1 no hint; -2 src is not a public base of
//       dst; -3 src is a public base of dst several times, never virtually. Only the first
//       form is used, for a fast path; every result is otherwise computed from the RTTI.
// Returns the dst object, or nullptr when the run-time check fails.
extern "C" void* __dynamic_cast(const void* sub, const __class_type_info* src, const __class_type_info* dst,
                                std::ptrdiff_t src2dst_offset) {
  // §2.9.4: vtable entry -2 is the offset from this virtual pointer to the top of the object,
  // entry -1 the type_info of that object. During construction or destruction the virtual
  // pointer names a construction vtable, whose entries describe the class under construction
  // ([class.cdtor]/6 treats it as the most derived object).
  const char* vptr = *static_cast<const char* const*>(sub);
  std::ptrdiff_t to_top = reinterpret_cast<const std::ptrdiff_t*>(vptr)[-2];
  auto* mdo_info = reinterpret_cast<const std::type_info* const*>(vptr)[-1];
  if (mdo_info == nullptr) // a vtable emitted without RTTI (-fno-rtti)
    return nullptr;
  auto* mdo_type = static_cast<const __class_type_info*>(mdo_info);
  const char* mdo = static_cast<const char*>(sub) + to_top;
  const char* source = static_cast<const char*>(sub);

  // Fast path, the common downcast to the most derived type: the hint says the src subobject
  // at that offset is the only src base of dst and is public, and a class has no subobject of
  // its own type, so the most derived object is the one dst object containing it.
  if (src2dst_offset >= 0 && mdo + src2dst_offset == source && *mdo_type == *dst)
    return const_cast<char*>(mdo);

  subobject root{mdo_type, mdo, nullptr, 0, true};
  base_search down = containing_objects(root, *src, source, *dst);
  if (down.count == 1 && down.is_public)
    return const_cast<char*>(down.first.addr);

  // [expr.dynamic.cast]/9.2, the cross cast: the source must be a public base of the most
  // derived object, and dst an unambiguous public base of it.
  if (!is_public_base(root, *src, source))
    return nullptr;
  base_search across = ycxx::abi::find_bases(root, *dst);
  if (across.count == 1 && across.is_public)
    return const_cast<char*>(across.first.addr);
  return nullptr;
}
