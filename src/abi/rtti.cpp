// libycxx ABI runtime: the RTTI classes (Itanium C++ ABI §2.9.4), the type_info objects of the
// fundamental types (§2.9.2), and exception handler matching ([except.handle]/3).
#include "internal.hpp"
#include "rtti.hpp"

// Defining std::type_info's key function emits _ZTVSt9type_info and _ZTISt9type_info here.
std::type_info::~type_info() {}

namespace __cxxabiv1 {

// Each destructor is its class's key function: defining it emits the class's vtable (and
// type_info) here, once (§2.9.4, final note).
//
// The fundamental types: both GCC and Clang emit _ZTI<T>, _ZTIP<T> and _ZTIPK<T> for every
// fundamental type they know (§2.9.2) in the translation unit that defines
// __fundamental_type_info's key function, so defining it here provides them. GCC 16.2 emits them
// as weak COMDAT objects, including the extended floating-point types (DF16_, DF16b, DF32_,
// DF64_, DF128_, DF32x, DF64x) and the decimal types; Clang 23.1 emits strong definitions, for
// a shorter list that has __fp16 (Dh) but lacks _Float16 (DF16_). Clang-compiled code still
// references _ZTIDF16_ for typeid(_Float16), so rtti_float16.cpp supplies it.
__fundamental_type_info::~__fundamental_type_info() {}
__array_type_info::~__array_type_info() {}
__function_type_info::~__function_type_info() {}
__enum_type_info::~__enum_type_info() {}
__class_type_info::~__class_type_info() {}
__si_class_type_info::~__si_class_type_info() {}
__vmi_class_type_info::~__vmi_class_type_info() {}
__pbase_type_info::~__pbase_type_info() {}
__pointer_type_info::~__pointer_type_info() {}
__pointer_to_member_type_info::~__pointer_to_member_type_info() {}

} // namespace __cxxabiv1

namespace ycxx::abi {

using namespace __cxxabiv1;

rtti_kind kind_of(const std::type_info& t) noexcept {
  // The dynamic type of a type_info object is one of the ABI classes; their type_info objects
  // live in this runtime, so comparing addresses is enough.
  const std::type_info* d = &typeid(t);
  if (d == &typeid(__si_class_type_info))
    return rtti_kind::class_si;
  if (d == &typeid(__vmi_class_type_info))
    return rtti_kind::class_vmi;
  if (d == &typeid(__class_type_info))
    return rtti_kind::class_plain;
  if (d == &typeid(__pointer_type_info))
    return rtti_kind::pointer;
  if (d == &typeid(__fundamental_type_info))
    return rtti_kind::fundamental;
  if (d == &typeid(__pointer_to_member_type_info))
    return rtti_kind::member_pointer;
  if (d == &typeid(__enum_type_info))
    return rtti_kind::enumeration;
  if (d == &typeid(__function_type_info))
    return rtti_kind::function;
  if (d == &typeid(__array_type_info))
    return rtti_kind::array;
  return rtti_kind::unknown;
}

base_search find_bases(const subobject& root, const __class_type_info& target) {
  base_search r;
  auto visit = [&](const subobject& s) {
    if (!(*s.type == target))
      return false;
    if (r.count == 0) {
      r.count = 1;
      r.first = s;
      r.is_public = s.is_public;
    } else if (same_subobject(r.first, s)) {
      r.is_public = r.is_public || s.is_public;
    } else {
      r.count = 2; // ambiguous; nothing more to learn
      return true;
    }
    return false;
  };
  walk_bases(root, visit);
  return r;
}

namespace {

// Null member pointer values, for std::nullptr_t caught by a pointer-to-member handler. The
// Itanium representation (§2.3): a null data member pointer is -1; a member function pointer
// is {ptr, adj} and is null when ptr is 0.
constexpr std::ptrdiff_t null_data_member_pointer = -1;
struct member_function_pointer {
  std::ptrdiff_t ptr;
  std::ptrdiff_t adj;
};
constexpr member_function_pointer null_member_function_pointer{0, 0};

constexpr unsigned qualifier_mask =
    __pbase_type_info::__const_mask | __pbase_type_info::__volatile_mask | __pbase_type_info::__restrict_mask;
// "Qualifiers" of a function type that a function pointer conversion may drop ([conv.fctptr]);
// transaction_safe (the TM TS) is treated like noexcept.
constexpr unsigned function_qualifier_mask =
    __pbase_type_info::__noexcept_mask | __pbase_type_info::__transaction_safe_mask;

bool is_pointer_like(rtti_kind k) noexcept { return k == rtti_kind::pointer || k == rtti_kind::member_pointer; }

// The function "qualifiers" of a pointer or pointer to member. GCC 16.2 leaves __noexcept_mask
// clear for pointers to noexcept member functions (_ZTIM1SDoFvvE has __flags 0 and __pointee
// _ZTIFvvE; Clang 23.1 sets the bit), so for those they are read from the mangled name as well:
// M <class type> <member type> (§5.1.5.8), where <class type> is spelled exactly as the
// context class's own name (it is the first component, so no substitution can abbreviate it)
// and the member function type is [<CV-qualifiers>] [Dx] [Do] F ... E.
unsigned function_qualifiers(const __pbase_type_info* p, rtti_kind kind) {
  unsigned f = p->__flags & function_qualifier_mask;
  if (kind != rtti_kind::member_pointer || kind_of(*p->__pointee) != rtti_kind::function)
    return f;
  const char* name = p->name();
  const char* context = static_cast<const __pointer_to_member_type_info*>(p)->__context->name();
  std::size_t n = __builtin_strlen(context);
  if (name[0] != 'M' || __builtin_strncmp(name + 1, context, n) != 0)
    return f;
  const char* s = name + 1 + n;
  while (*s == 'r' || *s == 'V' || *s == 'K')
    ++s;
  if (s[0] == 'D' && s[1] == 'x') {
    f |= __pbase_type_info::__transaction_safe_mask;
    s += 2;
  }
  if (s[0] == 'D' && s[1] == 'o')
    f |= __pbase_type_info::__noexcept_mask;
  return f;
}

// Whether a prvalue of the thrown pointer or pointer-to-member type converts to the handler's
// type by a qualification conversion ([conv.qual]/3), optionally preceded by a function pointer
// conversion ([conv.fctptr]). Both types are walked level by level along their qualification-
// decompositions; level i's qualifiers are the __flags of the i-th __pbase_type_info:
//   - the handler may not drop a qualifier at any level;
//   - where it adds one at level i, every level 0 < k < i of the handler must be const
//     (int** -> const int* const* is valid, int** -> const int** is not);
//   - "pointer to member of class C" levels must name the same C;
//   - noexcept may be dropped only from the function type a single pointer (or pointer to
//     member) designates: deeper down, the types are not similar ([conv.qual]/2);
//   - the remaining types U must be the same.
// Arrays: the type_info of an array type does not describe its element, so the array-of-
// unknown-bound and element-qualifier rules for pointers to arrays are not applied; such
// pointers match only exactly.
bool qualification_convertible(const __pbase_type_info* t, const __pbase_type_info* h, rtti_kind kind) {
  bool outer_const = true; // whether the handler is const at every level so far
  bool top = true;
  for (;;) {
    unsigned tf = t->__flags;
    unsigned hf = h->__flags;
    if ((tf & ~hf & qualifier_mask) != 0)
      return false;
    if ((hf & ~tf & qualifier_mask) != 0 && !outer_const)
      return false;
    unsigned tfq = function_qualifiers(t, kind);
    unsigned hfq = function_qualifiers(h, kind);
    if (top ? (hfq & ~tfq) != 0 : hfq != tfq)
      return false;
    if (kind == rtti_kind::member_pointer &&
        !(*static_cast<const __pointer_to_member_type_info*>(t)->__context ==
          *static_cast<const __pointer_to_member_type_info*>(h)->__context))
      return false;
    outer_const = outer_const && (hf & __pbase_type_info::__const_mask) != 0;
    top = false;

    rtti_kind tk = kind_of(*t->__pointee);
    rtti_kind hk = kind_of(*h->__pointee);
    if (tk != hk || !is_pointer_like(tk))
      return *t->__pointee == *h->__pointee;
    t = static_cast<const __pbase_type_info*>(t->__pointee);
    h = static_cast<const __pbase_type_info*>(h->__pointee);
    kind = tk;
  }
}

// [except.handle]/3.3 for a pointer thrown to a pointer handler. *obj is the address of the
// thrown pointer; on a match it becomes the converted pointer value.
bool pointer_matches(const __pointer_type_info* h, const __pointer_type_info* t, void** obj) {
  void* value = *static_cast<void* const*>(*obj);
  rtti_kind hk = kind_of(*h->__pointee);
  rtti_kind tk = kind_of(*t->__pointee);
  bool keeps_qualifiers = (t->__flags & ~h->__flags & qualifier_mask) == 0;

  // Pointer conversion to cv void* ([conv.ptr]/2), from any pointer to an object type. A
  // qualification conversion may follow, but only at the first level.
  if (hk == rtti_kind::fundamental && *h->__pointee == typeid(void) && tk != rtti_kind::function) {
    if (!keeps_qualifiers)
      return false;
    *obj = value;
    return true;
  }

  // Derived-to-base pointer conversion ([conv.ptr]/3) to an unambiguous public base, again
  // with qualifiers added at the first level only. A null pointer stays null; the base is
  // still checked, by type alone.
  if (is_class(hk) && is_class(tk) && !(*h->__pointee == *t->__pointee)) {
    if (!keeps_qualifiers)
      return false;
    auto* derived = static_cast<const __class_type_info*>(t->__pointee);
    auto* base = static_cast<const __class_type_info*>(h->__pointee);
    base_search r = find_bases(subobject{derived, static_cast<const char*>(value), nullptr, 0, true}, *base);
    if (r.count != 1 || !r.is_public)
      return false;
    *obj = const_cast<char*>(r.first.addr); // nullptr for a null pointer
    return true;
  }

  if (!qualification_convertible(t, h, rtti_kind::pointer))
    return false;
  *obj = value;
  return true;
}

} // namespace

// What __cxa_begin_catch must return, found by inspecting the code GCC 16.2 and Clang 23.1
// generate for each kind of handler (x86-64):
//   - catch (T* p): the return value is p itself (both compilers). For catch (T* const& p) GCC
//     copies the return value to a temporary and binds p to it; Clang binds p to the thrown
//     object itself (the _Unwind_Exception address + 32), so with Clang a converted pointer is
//     seen only by by-value handlers.
//   - catch (int S::* pm), catch (void (S::*pmf)()) and their const& forms: the return value
//     is the address of a member pointer object, which the handler copies (or binds to).
//   - catch (std::nullptr_t n): GCC ignores the return value; Clang copies the return value
//     itself into n, as for a pointer. catch (const std::nullptr_t&) binds to the returned
//     address in both. The address of the exception object serves all three: the value of a
//     std::nullptr_t object is never read ([conv.lval]/3.1).
//   - class and other non-pointer handlers: the address of the (base) object.
// The handler's type_info does not tell `T&` from `const T&` (both record T), so pointer
// conversions also apply to `catch (T*&)`, which [except.handle]/3.3 excludes.
bool catch_matches(const std::type_info* handler, const std::type_info* thrown, void** obj) noexcept {
  rtti_kind tk = kind_of(*thrown);
  if (*handler == *thrown) {
    if (tk == rtti_kind::pointer)
      *obj = *static_cast<void* const*>(*obj);
    return true;
  }
  rtti_kind hk = kind_of(*handler);

  // [except.handle]/3.2: an unambiguous public base class of the exception's class.
  if (is_class(hk) && is_class(tk)) {
    auto* derived = static_cast<const __class_type_info*>(thrown);
    base_search r = find_bases(subobject{derived, static_cast<const char*>(*obj), nullptr, 0, true},
                               *static_cast<const __class_type_info*>(handler));
    if (r.count != 1 || !r.is_public)
      return false;
    *obj = const_cast<char*>(r.first.addr);
    return true;
  }

  // [except.handle]/3.4: std::nullptr_t caught as any pointer or pointer to member.
  if (*thrown == typeid(std::nullptr_t)) {
    if (hk == rtti_kind::pointer) {
      *obj = nullptr;
      return true;
    }
    if (hk == rtti_kind::member_pointer) {
      auto* h = static_cast<const __pointer_to_member_type_info*>(handler);
      const void* null_value = &null_data_member_pointer;
      if (kind_of(*h->__pointee) == rtti_kind::function)
        null_value = &null_member_function_pointer;
      *obj = const_cast<void*>(null_value);
      return true;
    }
    return false;
  }

  // [except.handle]/3.3: pointer and qualification conversions, and function pointer conversions.
  if (hk == rtti_kind::pointer && tk == rtti_kind::pointer)
    return pointer_matches(static_cast<const __pointer_type_info*>(handler),
                           static_cast<const __pointer_type_info*>(thrown), obj);
  // Pointers to members: [conv.mem]/2 is a pointer-to-member conversion, not one of the
  // conversions [except.handle]/3.3 lists, so only qualification and function pointer
  // conversions apply, and they never change the representation.
  if (hk == rtti_kind::member_pointer && tk == rtti_kind::member_pointer)
    return qualification_convertible(static_cast<const __pbase_type_info*>(thrown),
                                     static_cast<const __pbase_type_info*>(handler), rtti_kind::member_pointer);
  return false;
}

} // namespace ycxx::abi
