// REQUIRES: rtti
// The mangled names of the standard library's types (DECISIONS §20.4-20.5): the entities the
// compilers or the platform name in plain std keep their ::std names, so the replaceable
// allocation functions keep the platform's mangled names (_ZnwmSt11align_val_t,
// _ZnwmRKSt9nothrow_t); every other standard type is in the inline ABI namespace std::__y1, whose
// names no other C++ library uses.
#include <cstddef>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>
#include "check.hpp"

int main() {
  CHECK(std::strcmp(typeid(std::align_val_t).name(), "St11align_val_t") == 0);
  CHECK(std::strcmp(typeid(std::nothrow_t).name(), "St9nothrow_t") == 0);
  CHECK(std::strcmp(typeid(std::destroying_delete_t).name(), "St19destroying_delete_t") == 0);
  CHECK(std::strcmp(typeid(std::byte).name(), "St4byte") == 0);
  CHECK(std::strcmp(typeid(std::initializer_list<int>).name(), "St16initializer_listIiE") == 0);
  CHECK(std::strcmp(typeid(std::exception).name(), "NSt4__y19exceptionE") == 0);
  CHECK(std::strcmp(typeid(std::runtime_error).name(), "NSt4__y113runtime_errorE") == 0);
  CHECK(std::strcmp(typeid(std::type_info).name(), "NSt4__y19type_infoE") == 0);
  CHECK(std::strcmp(typeid(std::vector<int>).name(), "NSt4__y16vectorIiNS_9allocatorIiEEEE") == 0);
  CHECK(std::strcmp(typeid(std::string).name(),
                    "NSt4__y112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE") == 0);
  return 0;
}
