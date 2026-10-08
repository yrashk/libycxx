// The per-image part of the RTTI runtime (shared mode): the vtables and type_info objects of the
// __cxxabiv1 classes, which every type_info object the compiler emits points to, and the
// fundamental types' type_info objects, all hidden. The runtime in libycxx.so classifies another
// image's type_info objects by the names of their ABI classes (DECISIONS §2), so these copies
// never need to be libycxx.so's. Compiled with -I <tree>/src/abi and the tree's build/generated.
#include "internal.hpp"
#include "rtti.hpp"
#include <abi/fundamental_type_infos.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {
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

// std::nothrow (plain std, hidden): an empty object whose address nobody compares.
namespace [[__gnu__::__visibility__("hidden")]] std {
const nothrow_t nothrow{};
} // namespace std
