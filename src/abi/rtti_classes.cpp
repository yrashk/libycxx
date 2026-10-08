// libycxx ABI runtime: the vtables and type_info objects of the RTTI classes (Itanium C++ ABI
// §2.9.4), and the type_info objects of the fundamental types (§2.9.2). Every type_info object the
// compilers emit points to one of these vtables by its Itanium name, which other C++ runtimes
// define too: they are hidden in every image, and in shared mode each image has its own copy
// (DECISIONS §20.6), which is why they are in a file of their own. The runtime classifies a
// type_info object whose ABI class is another image's by that class's name (rtti.cpp).
#include "internal.hpp"
#include "rtti.hpp"
#include <abi/fundamental_type_infos.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {

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

// GCC gives the fundamental type_info objects default visibility whatever -fvisibility says.
// Exported from a program or shared object, they would be the ones another
// C++ runtime in the process binds its own references to (DECISIONS §2), so they are hidden with
// assembler directives (`.hidden` on ELF, `.private_extern` on Mach-O). Which ones the compiler
// emits depends on the target (AArch64 adds __bf16, __mfp8 and the SVE types), so the list is
// the compiler's own: the build compiles a probe defining this key function and lists its
// type_info symbols (CMakeLists.txt, generated fundamental_type_infos.hpp, assembler names).
// Every compiler's list is hidden: a directive for a symbol already hidden changes nothing.
namespace {
consteval __ycxx::__abi::__asm_text hide_fundamental_type_infos() {
  __ycxx::__abi::__asm_text a;
  for (const char* const* symbol = __ycxx::__abi::fundamental_type_info_symbols; *symbol; ++symbol) {
    a.append(__ycxx::__detail::__cfg::__darwin ? ".private_extern " : ".hidden ");
    a.append(*symbol);
    a.append("\n");
  }
  return a;
}
} // namespace
asm((hide_fundamental_type_infos()));
