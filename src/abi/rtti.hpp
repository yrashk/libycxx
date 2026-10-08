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

namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {

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
  const __base_class_type_info* __y_bases() const noexcept { return __base_info; }
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

  __pbase_type_info(const char* n, unsigned int flags, const std::type_info* __pointee) noexcept
      : std::type_info(n), __flags(flags), __pointee(__pointee) {}
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

  __pointer_to_member_type_info(const char* n, unsigned int flags, const std::type_info* __pointee,
                                const __class_type_info* __context) noexcept
      : __pbase_type_info(n, flags, __pointee), __context(__context) {}
  ~__pointer_to_member_type_info() override;
};

} // namespace __cxxabiv1

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __abi {

// Which ABI class a type_info object is (its dynamic type, read through typeid).
enum class __rtti_kind : unsigned char {
  __fundamental,
  array,
  function,
  __enumeration,
  __class_plain, // __class_type_info: no bases, or an incomplete class
  __class_si,
  __class_vmi,
  pointer,
  __member_pointer,
  unknown,
};

__rtti_kind __kind_of_any(const std::type_info& t) noexcept;
// The class kinds a hierarchy walk meets on every step are tested inline, by the address of the
// type_info object's own type_info (this runtime's ABI classes); the rest, and type_info objects
// of another image's runtime copy, go to __kind_of_any.
inline __rtti_kind __kind_of(const std::type_info& t) noexcept {
  const std::type_info* d = &typeid(t);
  if (d == &typeid(__cxxabiv1::__si_class_type_info))
    return __rtti_kind::__class_si;
  if (d == &typeid(__cxxabiv1::__vmi_class_type_info))
    return __rtti_kind::__class_vmi;
  if (d == &typeid(__cxxabiv1::__class_type_info))
    return __rtti_kind::__class_plain;
  return __kind_of_any(t);
}

// std::type_info::operator== with the name comparison written out: the runtime compares types on
// every step of a dynamic_cast or handler search, mostly types that differ within their first
// few characters, where a call to strcmp costs more than the comparison itself.
struct __type_info_name : std::type_info {
  static constexpr const char* std::type_info::* name = &__type_info_name::__name_;
};
inline bool __same_type(const std::type_info& a, const std::type_info& b) noexcept {
  if (&a == &b)
    return true;
  const char* __x = __ycxx::__detail::__rtti_name(a.*__type_info_name::name);
  const char* y = __ycxx::__detail::__rtti_name(b.*__type_info_name::name);
  // A name starting with '*' belongs to one type_info object only (see operator==).
  if (*__x == '*' || *y == '*')
    return false;
  if (__x == y)
    return true;
  // Most names differ within a few characters; a long common prefix (a nested name) goes to the
  // C library's strcmp.
  const char* const __x0 = __x;
  for (int i = 0; i < 8; ++i, ++__x, ++y) {
    if (*__x != *y)
      return false;
    if (*__x == '\0')
      return true;
  }
  // A name that begins in an unnamed namespace ([namespace.unnamed]) is a type of one
  // translation unit, with internal linkage ([basic.link]/4), so it has one type_info object: two
  // objects are two types. GCC marks such names with '*'; Clang does not, and their common
  // prefix "N12_GLOBAL__N_1" would otherwise cost a strcmp per comparison.
  // Inline: x0 holds at least 9 characters, so its first 8 are one word; the other 7 are compared
  // until the first difference, at the latest x0's terminator.
  unsigned long long __head, __want;
  __builtin_memcpy(&__head, __x0, 8);
  __builtin_memcpy(&__want, "N12_GLOB", 8);
  if (__head == __want) {
    const char* const __tail = "AL__N_1";
    int i = 0;
    while (i < 7 && __x0[8 + i] == __tail[i])
      ++i;
    if (i == 7)
      return false;
  }
  return __builtin_strcmp(__x, y) == 0;
}

// Whether a type_info object is the only one of its type, so that any other object is another
// type: its name is marked '*' (GCC: internal linkage), or begins in an unnamed namespace
// (Clang does not mark those; see same_type).
inline bool __sole_type_info(const std::type_info& a) noexcept {
  const char* __x = __ycxx::__detail::__rtti_name(a.*__type_info_name::name);
  if (*__x == '*')
    return true;
  const char* const __anon = "N12_GLOBAL__N_1";
  int i = 0;
  while (i < 15 && __x[i] == __anon[i])
    ++i;
  return i == 15;
}

// same_type(t, type) against one type many times (a hierarchy walk): when that type_info is the
// sole one of its type, addresses decide.
struct __type_matcher {
  const std::type_info* type;
  bool __by_address;
  explicit __type_matcher(const std::type_info& t) noexcept : type(&t), __by_address(__sole_type_info(t)) {}
  bool __matches(const std::type_info& t) const noexcept {
    return &t == type || (!__by_address && __same_type(t, *type));
  }
};

inline bool is_class(__rtti_kind k) noexcept {
  return k == __rtti_kind::__class_plain || k == __rtti_kind::__class_si || k == __rtti_kind::__class_vmi;
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
struct __subobject {
  const __cxxabiv1::__class_type_info* type;
  const char* __addr;
  const __cxxabiv1::__class_type_info* __anchor;
  std::ptrdiff_t offset;
  bool is_public;
};

inline bool __same_subobject(const __subobject& a, const __subobject& b) noexcept {
  if (a.__addr != nullptr && b.__addr != nullptr)
    return a.__addr == b.__addr;
  if (a.offset != b.offset)
    return false;
  if (a.__anchor == nullptr || b.__anchor == nullptr)
    return a.__anchor == b.__anchor;
  return __same_type(*a.__anchor, *b.__anchor);
}

// The virtual-base subobjects a walk has already entered, with whether along a public path. A
// virtual base reached again along another path holds the same subobjects, so its subtree needs
// another walk only if the new path is public and the earlier ones were not: without this, a
// hierarchy of stacked diamonds would be walked along every one of its exponentially many paths.
// Past its capacity the walk simply enters again.
struct __vbase_memo {
  static constexpr int capacity = 64;
  const void* key[capacity];
  bool is_public[capacity];
  int n = 0;

  // Whether the subtree must be walked; records the visit.
  bool __enter(const void* k, bool __pub) noexcept {
    for (int i = 0; i < n; ++i)
      if (key[i] == k) {
        if (is_public[i] || !__pub)
          return false;
        is_public[i] = true;
        return true;
      }
    if (n < capacity) {
      key[n] = k;
      is_public[n] = __pub;
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
template <class _Visit>
bool __walk_bases(const __subobject& s, _Visit& visit, __vbase_memo& __memo) {
  if (visit(s))
    return true;
  switch (__kind_of(*s.type)) {
  case __rtti_kind::__class_si: {
    // §2.9.4: a single public non-virtual base at offset zero.
    auto* __si = static_cast<const __cxxabiv1::__si_class_type_info*>(s.type);
    return __walk_bases(__subobject{__si->__base_type, s.__addr, s.__anchor, s.offset, s.is_public}, visit, __memo);
  }
  case __rtti_kind::__class_vmi: {
    auto* __vmi = static_cast<const __cxxabiv1::__vmi_class_type_info*>(s.type);
    for (unsigned i = 0; i < __vmi->__base_count; ++i) {
      const __cxxabiv1::__base_class_type_info& b = __vmi->__y_bases()[i];
      __subobject __child{b.__base_type, nullptr, s.__anchor, s.offset + b.offset(),
                      s.is_public && b.is_public()};
      if (b.is_virtual()) {
        // §2.9.4: offset() is where, relative to the derived subobject's virtual pointer,
        // its vtable stores the offset of the virtual base from the derived subobject. A class
        // with virtual bases is dynamic, so its virtual pointer is at offset zero. This also
        // holds for construction vtables, which give the layout of the object being built.
        __child.__anchor = b.__base_type;
        __child.offset = 0;
        if (s.__addr != nullptr) {
          const char* __vptr = *reinterpret_cast<const char* const*>(s.__addr);
          __child.__addr = s.__addr + *reinterpret_cast<const std::ptrdiff_t*>(__vptr + b.offset());
        }
        // The subobject's identity: its address, or without an object its type (every virtual
        // base of one type is one subobject).
        const void* k = __child.__addr != nullptr ? static_cast<const void*>(__child.__addr) : b.__base_type;
        if (!__memo.__enter(k, __child.is_public))
          continue;
      } else if (s.__addr != nullptr) {
        __child.__addr = s.__addr + b.offset();
      }
      if (__walk_bases(__child, visit, __memo))
        return true;
    }
    return false;
  }
  default: return false;
  }
}
template <class _Visit>
bool __walk_bases(const __subobject& s, _Visit& visit) {
  __vbase_memo __memo;
  return __walk_bases(s, visit, __memo);
}

// The subobjects of type `target` within the subobject `__root` (root itself included).
struct __base_search {
  int count = 0;        // distinct subobjects found, saturating at 2
  __subobject first{};    // the first one found
  bool is_public = false; // whether `first` is reachable along some public path
};

__base_search __find_bases(const __subobject& __root, const __cxxabiv1::__class_type_info& target);

}} // namespace __ycxx::__abi
