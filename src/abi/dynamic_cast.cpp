// libycxx ABI runtime: __dynamic_cast (Itanium C++ ABI §2.9.7, [expr.dynamic.cast]/9).
#include "rtti.hpp"

using namespace __cxxabiv1;
using __ycxx::__abi::__rtti_kind;

namespace {

// One depth-first walk over the most derived object's base-class subobjects gathers everything
// [expr.dynamic.cast]/9 needs:
//   - the downcast (9.1): the distinct dst subobjects that contain the source subobject (type
//     src at address sub), and whether one of them reaches it along a public path;
//   - the cross cast (9.2): whether the source is a public base of the most derived object, and
//     the distinct dst subobjects, with whether one of them is reachable publicly.
// A class is never its own base, so a path holds at most one dst and at most one source: below
// a dst (or the source) the walk stops comparing against that type. When the class has no
// repeated base class at all (its __vmi_class_type_info __flags are clear; both compilers set
// them for a repetition anywhere in the hierarchy), every subobject lies on exactly one path,
// and the walk stops once it has seen the source and a dst.
struct cast_walk {
  const __class_type_info* __src;
  const char* __sub;
  const __class_type_info* __dst;
  bool unique_bases;

  bool src_seen = false;
  bool src_public = false;  // the source is a public base of the most derived object
  const char* down = nullptr;
  int down_count = 0;       // distinct dst subobjects containing the source (saturating at 2)
  bool down_public = false; // ... one of which reaches it along a public path
  const char* across = nullptr;
  int across_count = 0;     // distinct dst subobjects (saturating at 2)
  bool across_public = false;

  // Virtual-base subtrees already walked in a given state: walking one again in the same state
  // adds nothing (the results only collect distinct addresses and or-ed flags), and stacked
  // diamonds would otherwise be walked along exponentially many paths. Past its capacity the
  // walk simply enters again.
  struct seen_state {
    const char* __addr;
    const char* in_dst;
    unsigned flags;
  };
  static constexpr int capacity = 64;
  seen_state __seen[capacity];
  int nseen = 0;
  bool __enter(const char* __addr, const char* in_dst, unsigned flags) noexcept {
    for (int i = 0; i < nseen; ++i)
      if (__seen[i].__addr == __addr && __seen[i].in_dst == in_dst && __seen[i].flags == flags)
        return false;
    if (nseen < capacity)
      __seen[nseen++] = {__addr, in_dst, flags};
    return true;
  }

  cast_walk(const __class_type_info* s, const char* at, const __class_type_info* d, bool unique) noexcept
      : __src(s), __sub(at), __dst(d), unique_bases(unique) {}

  // The subobject of type t at addr. pub: the path from the most derived object is public;
  // in_dst: the dst subobject on this path, or null; dst_pub: the path from in_dst is public;
  // below_src: the source is on this path. Returns true to end the walk.
  bool visit(const __class_type_info* t, const char* __addr, bool __pub, const char* in_dst, bool dst_pub,
             bool below_src) {
    if (in_dst == nullptr && __ycxx::__abi::__same_type(*t, *__dst)) {
      if (across_count == 0) {
        across = __addr;
        across_count = 1;
        across_public = __pub;
      } else if (across == __addr) {
        across_public = across_public || __pub;
      } else {
        across_count = 2;
      }
      in_dst = __addr;
      dst_pub = true;
    } else if (!below_src && __addr == __sub && __ycxx::__abi::__same_type(*t, *__src)) {
      src_seen = true;
      src_public = src_public || __pub;
      below_src = true;
      if (in_dst != nullptr) {
        if (down_count == 0) {
          down = in_dst;
          down_count = 1;
          down_public = dst_pub;
        } else if (down == in_dst) {
          down_public = down_public || dst_pub;
        } else {
          down_count = 2;
        }
      }
    }
    if (unique_bases && src_seen && across_count != 0)
      return true;
    switch (__ycxx::__abi::__kind_of(*t)) {
    case __rtti_kind::__class_si:
      // §2.9.4: a single public non-virtual base at offset zero.
      return visit(static_cast<const __si_class_type_info*>(t)->__base_type, __addr, __pub, in_dst, dst_pub, below_src);
    case __rtti_kind::__class_vmi: {
      auto* __vmi = static_cast<const __vmi_class_type_info*>(t);
      for (unsigned i = 0; i < __vmi->__base_count; ++i) {
        const __base_class_type_info& b = __vmi->__y_bases()[i];
        // As in walk_bases (rtti.hpp): a virtual base's offset is stored in this subobject's
        // vtable, at b.offset() from its virtual pointer.
        const char* __child = __addr + b.offset();
        const bool p = b.is_public();
        if (b.is_virtual()) {
          __child = __addr + *reinterpret_cast<const std::ptrdiff_t*>(*reinterpret_cast<const char* const*>(__addr) +
                                                                  b.offset());
          const unsigned flags = (__pub && p ? 1u : 0u) | (dst_pub && p ? 2u : 0u) | (below_src ? 4u : 0u);
          if (!__enter(__child, in_dst, flags))
            continue;
        }
        if (visit(b.__base_type, __child, __pub && p, in_dst, dst_pub && p, below_src))
          return true;
      }
      return false;
    }
    default: return false;
    }
  }
};

} // namespace

// sub:  the address of a polymorphic subobject of static type src (non-null).
// dst:  the class T of dynamic_cast<T*> / dynamic_cast<T&>.
// src2dst_offset: the compiler's static hint (§2.9.7): >= 0 when src is a unique public
//       non-virtual base of dst at that offset; -1 no hint; -2 src is not a public base of
//       dst; -3 src is a public base of dst several times, never virtually. Only the first
//       form is used, for a fast path; every result is otherwise computed from the RTTI.
// Returns the dst object, or nullptr when the run-time check fails.
extern "C" [[__gnu__::__visibility__("hidden")]] void* __dynamic_cast(const void* __sub, const __class_type_info* __src, const __class_type_info* __dst,
                                std::ptrdiff_t src2dst_offset) {
  // §2.9.4: vtable entry -2 is the offset from this virtual pointer to the top of the object,
  // entry -1 the type_info of that object. During construction or destruction the virtual
  // pointer names a construction vtable, whose entries describe the class under construction
  // ([class.cdtor]/6 treats it as the most derived object).
  const char* __vptr = *static_cast<const char* const*>(__sub);
  std::ptrdiff_t to_top = reinterpret_cast<const std::ptrdiff_t*>(__vptr)[-2];
  auto* mdo_info = reinterpret_cast<const std::type_info* const*>(__vptr)[-1];
  if (mdo_info == nullptr) // a vtable emitted without RTTI (-fno-rtti)
    return nullptr;
  auto* mdo_type = static_cast<const __class_type_info*>(mdo_info);
  const char* mdo = static_cast<const char*>(__sub) + to_top;
  const char* __source = static_cast<const char*>(__sub);

  // Fast path, the common downcast to the most derived type: the hint says the src subobject
  // at that offset is the only src base of dst and is public, and a class has no subobject of
  // its own type, so the most derived object is the one dst object containing it.
  if (src2dst_offset >= 0 && mdo + src2dst_offset == __source && __ycxx::__abi::__same_type(*mdo_type, *__dst))
    return const_cast<char*>(mdo);

  // Single inheritance from the top (the usual case): those classes are all at the most derived
  // object's address and public bases of it. If both src (at that address) and dst are among
  // them, the cast succeeds there: a downcast when dst contains src, else dst is a public base of
  // src. A source below the chain (say, behind a private base of its last class) needs the full
  // walk. If the chain is the whole hierarchy and dst is not in it, dst is no base.
  const __class_type_info* t = mdo_type;
  __rtti_kind k = __ycxx::__abi::__kind_of(*t);
  bool dst_in_chain = false;
  bool src_in_chain = false;
  for (;;) {
    dst_in_chain = dst_in_chain || __ycxx::__abi::__same_type(*t, *__dst);
    src_in_chain = src_in_chain || (__source == mdo && __ycxx::__abi::__same_type(*t, *__src));
    if (dst_in_chain && src_in_chain)
      return const_cast<char*>(mdo);
    if (k != __rtti_kind::__class_si)
      break;
    t = static_cast<const __si_class_type_info*>(t)->__base_type;
    k = __ycxx::__abi::__kind_of(*t);
  }
  if (k != __rtti_kind::__class_vmi)
    return nullptr;

  // The classes above t occur once each (none can be a base of t), so t's flags tell whether
  // any base class repeats.
  cast_walk __w(__src, __source, __dst, static_cast<const __vmi_class_type_info*>(t)->__flags == 0);
  __w.visit(mdo_type, mdo, true, nullptr, true, false);
  if (__w.down_count == 1 && __w.down_public)
    return const_cast<char*>(__w.down);
  // [expr.dynamic.cast]/9.2, the cross cast: the source must be a public base of the most
  // derived object, and dst an unambiguous public base of it.
  if (__w.src_public && __w.across_count == 1 && __w.across_public)
    return const_cast<char*>(__w.across);
  return nullptr;
}
