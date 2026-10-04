// libycxx ABI runtime: type_info objects for _Float16 (mangled DF16_) and pointers to it.
//
// §2.9.2 places the type_info objects of the fundamental types in the runtime. rtti.cpp gets
// them from the compiler (see __fundamental_type_info's destructor there), but Clang 23.1 omits
// _Float16 from that list although its code references _ZTIDF16_, _ZTIPDF16_ and _ZTIPKDF16_.
// They are defined here by hand. GCC 16.2 emits the same three objects as weak COMDAT
// definitions in rtti.cpp; when both end up linked, these strong definitions take precedence,
// which is harmless since they are identical.
//
// std::type_info's constructor is not constexpr, so objects of the ABI classes could not be
// constant-initialised here. These definitions spell out the §2.9.4 layout instead: a virtual
// pointer to the address point of the class's vtable (its virtual functions, preceded by the
// offset-to-top and type_info entries) and the members.

#include <cstddef>

namespace ycxx::abi {

// The start of a vtable whose virtual functions are the complete and deleting destructors.
struct vtable_image {
  std::ptrdiff_t offset_to_top;
  const void* type;
  const void* virtuals[2];
};

struct fundamental_image {
  const void* const* vptr;
  const char* name;
};

struct pointer_image {
  const void* const* vptr;
  const char* name;
  unsigned int flags;
  const fundamental_image* pointee;
};

extern const vtable_image fundamental_vtable asm("_ZTVN10__cxxabiv123__fundamental_type_infoE");
extern const vtable_image pointer_vtable asm("_ZTVN10__cxxabiv119__pointer_type_infoE");

extern const fundamental_image float16_info asm("_ZTIDF16_");
extern const pointer_image float16_pointer_info asm("_ZTIPDF16_");
extern const pointer_image float16_const_pointer_info asm("_ZTIPKDF16_");

constinit const fundamental_image float16_info{fundamental_vtable.virtuals, "DF16_"};
constinit const pointer_image float16_pointer_info{pointer_vtable.virtuals, "PDF16_", 0, &float16_info};
constinit const pointer_image float16_const_pointer_info{pointer_vtable.virtuals, "PKDF16_", 0x1, &float16_info};

} // namespace ycxx::abi
