// libycxx ABI runtime: the RTTI classes of the Itanium C++ ABI (§2.9.4) and helpers shared by
// handler matching and __dynamic_cast. Not installed.
//
// The compilers emit type_info objects whose vtable pointers name the vtables of these classes
// (_ZTVN10__cxxabiv1...E); the vtables, and the type_info objects of the fundamental types, are
// emitted by rtti.cpp, which defines every class's key function (its destructor). The data
// members are laid out exactly as §2.9.4 specifies; the member functions are ours alone.
#pragma once

#include <cstddef>
#include <typeinfo>

namespace [[gnu::visibility("hidden")]] __cxxabiv1 {

class __fundamental_type_info : public std::type_info {
public:
  explicit __fundamental_type_info(const char* n) noexcept : std::type_info(n) {}
  ~__fundamental_type_info() override;
};

class __array_type_info : public std::type_info {
public:
  explicit __array_type_info(const char* n) noexcept : std::type_info(n) {}
  ~__array_type_info() override;
};

class __function_type_info : public std::type_info {
public:
  explicit __function_type_info(const char* n) noexcept : std::type_info(n) {}
  ~__function_type_info() override;
};

class __enum_type_info : public std::type_info {
public:
  explicit __enum_type_info(const char* n) noexcept : std::type_info(n) {}
  ~__enum_type_info() override;
};

// A class with no bases (and the base of the other two class representations). Also used,
// as a local object, for incomplete classes named by pointer RTTI.
class __class_type_info : public std::type_info {
public:
  explicit __class_type_info(const char* n) noexcept : std::type_info(n) {}
  ~__class_type_info() override;
};

// A class whose only base is public, non-virtual and at offset zero.
class __si_class_type_info : public __class_type_info {
public:
  const __class_type_info* __base_type;

  __si_class_type_info(const char* n, const __class_type_info* base) noexcept
      : __class_type_info(n), __base_type(base) {}
  ~__si_class_type_info() override;
};

struct __base_class_type_info {
  const __class_type_info* __base_type;
  // Bits 8 and up: for a non-virtual base, its offset in the derived object; for a virtual
  // base, the (negative) offset in the derived object's vtable of the virtual-base offset.
  long __offset_flags;

  enum __offset_flags_masks { __virtual_mask = 0x1, __public_mask = 0x2, __offset_shift = 8 };

  bool is_virtual() const noexcept { return (__offset_flags & __virtual_mask) != 0; }
  bool is_public() const noexcept { return (__offset_flags & __public_mask) != 0; }
  // Arithmetic shift: the offset is signed.
  long offset() const noexcept { return __offset_flags >> __offset_shift; }
};

// Any other class: __base_count direct base descriptions follow (a variable-length object;
// the compiler emits as many elements as there are bases).
class __vmi_class_type_info : public __class_type_info {
public:
  unsigned int __flags;
  unsigned int __base_count;
  __base_class_type_info __base_info[1];

  enum __flags_masks { __non_diamond_repeat_mask = 0x1, __diamond_shaped_mask = 0x2 };

  ~__vmi_class_type_info() override;
  const __base_class_type_info* bases() const noexcept { return __base_info; }
};

class __pbase_type_info : public std::type_info {
public:
  // Qualifiers of the pointee, plus the "qualifiers" of a pointed-to function type, whose
  // __pointee then names the function type without them (§2.9.4).
  unsigned int __flags;
  const std::type_info* __pointee;

  enum __masks {
    __const_mask = 0x1,
    __volatile_mask = 0x2,
    __restrict_mask = 0x4,
    __incomplete_mask = 0x8,
    __incomplete_class_mask = 0x10,
    __transaction_safe_mask = 0x20,
    __noexcept_mask = 0x40
  };

  __pbase_type_info(const char* n, unsigned int flags, const std::type_info* pointee) noexcept
      : std::type_info(n), __flags(flags), __pointee(pointee) {}
  ~__pbase_type_info() override;
};

class __pointer_type_info : public __pbase_type_info {
public:
  using __pbase_type_info::__pbase_type_info;
  ~__pointer_type_info() override;
};

class __pointer_to_member_type_info : public __pbase_type_info {
public:
  const __class_type_info* __context;

  __pointer_to_member_type_info(const char* n, unsigned int flags, const std::type_info* pointee,
                                const __class_type_info* context) noexcept
      : __pbase_type_info(n, flags, pointee), __context(context) {}
  ~__pointer_to_member_type_info() override;
};

} // namespace __cxxabiv1

namespace [[gnu::visibility("hidden")]] ycxx { namespace abi {

// Which ABI class a type_info object is (its dynamic type, read through typeid).
enum class rtti_kind : unsigned char {
  fundamental,
  array,
  function,
  enumeration,
  class_plain, // __class_type_info: no bases, or an incomplete class
  class_si,
  class_vmi,
  pointer,
  member_pointer,
  unknown,
};

rtti_kind kind_of(const std::type_info& t) noexcept;

// std::type_info::operator== with the name comparison written out: the runtime compares types on
// every step of a dynamic_cast or handler search, mostly types that differ within their first
// few characters, where a call to strcmp costs more than the comparison itself.
struct type_info_name : std::type_info {
  static constexpr const char* std::type_info::* name = &type_info_name::name_;
};
inline bool same_type(const std::type_info& a, const std::type_info& b) noexcept {
  if (&a == &b)
    return true;
  const char* x = ycxx::detail::rtti_name(a.*type_info_name::name);
  const char* y = ycxx::detail::rtti_name(b.*type_info_name::name);
  // A name starting with '*' belongs to one type_info object only (see operator==).
  if (*x == '*' || *y == '*')
    return false;
  if (x == y)
    return true;
  // Most names differ within a few characters; a long common prefix (a nested or
  // anonymous-namespace name, which Clang does not mark with '*') goes to the C library's strcmp.
  for (int i = 0; i < 8; ++i, ++x, ++y) {
    if (*x != *y)
      return false;
    if (*x == '\0')
      return true;
  }
  return __builtin_strcmp(x, y) == 0;
}

inline bool is_class(rtti_kind k) noexcept {
  return k == rtti_kind::class_plain || k == rtti_kind::class_si || k == rtti_kind::class_vmi;
}

// One base-class subobject reached along one inheritance path.
//   addr:   its address, or nullptr when walking a type without an object (a null pointer
//           thrown and caught as a pointer to a base); virtual-base offsets need an object.
//   anchor/offset: an object-independent identity: the virtual base it lies in (nullptr for
//           the non-virtual part of the root) and its offset from that virtual base. Every
//           virtual base of a given type is one subobject, and two subobjects of one type are
//           never at the same address ([intro.object]), so two paths reach the same subobject
//           exactly when both components agree.
//   is_public: whether every step of this path is a public base.
struct subobject {
  const __cxxabiv1::__class_type_info* type;
  const char* addr;
  const __cxxabiv1::__class_type_info* anchor;
  std::ptrdiff_t offset;
  bool is_public;
};

inline bool same_subobject(const subobject& a, const subobject& b) noexcept {
  if (a.addr != nullptr && b.addr != nullptr)
    return a.addr == b.addr;
  if (a.offset != b.offset)
    return false;
  if (a.anchor == nullptr || b.anchor == nullptr)
    return a.anchor == b.anchor;
  return same_type(*a.anchor, *b.anchor);
}

// The virtual-base subobjects a walk has already entered, with whether along a public path. A
// virtual base reached again along another path holds the same subobjects, so its subtree needs
// another walk only if the new path is public and the earlier ones were not: without this, a
// hierarchy of stacked diamonds would be walked along every one of its exponentially many paths.
// Past its capacity the walk simply enters again.
struct vbase_memo {
  static constexpr int capacity = 64;
  const void* key[capacity];
  bool is_public[capacity];
  int n = 0;

  // Whether the subtree must be walked; records the visit.
  bool enter(const void* k, bool pub) noexcept {
    for (int i = 0; i < n; ++i)
      if (key[i] == k) {
        if (is_public[i] || !pub)
          return false;
        is_public[i] = true;
        return true;
      }
    if (n < capacity) {
      key[n] = k;
      is_public[n] = pub;
      ++n;
    }
    return true;
  }
};

// Calls visit(s) for s and then, depth first, for every base-class subobject along every
// inheritance path, except that a virtual base already entered along a path at least as public
// is not entered again (vbase_memo). Stops as soon as visit returns true; returns whether it
// did. The visitors only gather distinct subobjects and whether each is publicly reachable, which
// the skipped paths cannot change.
template <class Visit>
bool walk_bases(const subobject& s, Visit& visit, vbase_memo& memo) {
  if (visit(s))
    return true;
  switch (kind_of(*s.type)) {
  case rtti_kind::class_si: {
    // §2.9.4: a single public non-virtual base at offset zero.
    auto* si = static_cast<const __cxxabiv1::__si_class_type_info*>(s.type);
    return walk_bases(subobject{si->__base_type, s.addr, s.anchor, s.offset, s.is_public}, visit, memo);
  }
  case rtti_kind::class_vmi: {
    auto* vmi = static_cast<const __cxxabiv1::__vmi_class_type_info*>(s.type);
    for (unsigned i = 0; i < vmi->__base_count; ++i) {
      const __cxxabiv1::__base_class_type_info& b = vmi->bases()[i];
      subobject child{b.__base_type, nullptr, s.anchor, s.offset + b.offset(),
                      s.is_public && b.is_public()};
      if (b.is_virtual()) {
        // §2.9.4: offset() is where, relative to the derived subobject's virtual pointer,
        // its vtable stores the offset of the virtual base from the derived subobject. A class
        // with virtual bases is dynamic, so its virtual pointer is at offset zero. This also
        // holds for construction vtables, which give the layout of the object being built.
        child.anchor = b.__base_type;
        child.offset = 0;
        if (s.addr != nullptr) {
          const char* vptr = *reinterpret_cast<const char* const*>(s.addr);
          child.addr = s.addr + *reinterpret_cast<const std::ptrdiff_t*>(vptr + b.offset());
        }
        // The subobject's identity: its address, or without an object its type (every virtual
        // base of one type is one subobject).
        const void* k = child.addr != nullptr ? static_cast<const void*>(child.addr) : b.__base_type;
        if (!memo.enter(k, child.is_public))
          continue;
      } else if (s.addr != nullptr) {
        child.addr = s.addr + b.offset();
      }
      if (walk_bases(child, visit, memo))
        return true;
    }
    return false;
  }
  default: return false;
  }
}
template <class Visit>
bool walk_bases(const subobject& s, Visit& visit) {
  vbase_memo memo;
  return walk_bases(s, visit, memo);
}

// The subobjects of type `target` within the subobject `root` (root itself included).
struct base_search {
  int count = 0;        // distinct subobjects found, saturating at 2
  subobject first{};    // the first one found
  bool is_public = false; // whether `first` is reachable along some public path
};

base_search find_bases(const subobject& root, const __cxxabiv1::__class_type_info& target);

}} // namespace ycxx::abi
