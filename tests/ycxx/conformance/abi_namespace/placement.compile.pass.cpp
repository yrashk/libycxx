// FLAGS: -freflection
// REQUIRES: gcc
// Which namespace each standard entity is declared in (DECISIONS §20.4-20.5), asked of the compiler
// with reflection: the entities the compilers or the platform name in plain std are members of
// ::std itself, every other one is a member of std's inline ABI namespace.
#include <cstddef>
#include <exception>
#include <initializer_list>
#include <meta>
#include <new>
#include <string>
#include <typeinfo>
#include <vector>

consteval bool in_plain_std(std::meta::info r) { return std::meta::parent_of(r) == ^^std; }
consteval bool in_abi_namespace(std::meta::info r) {
  std::meta::info p = std::meta::parent_of(r);
  return p != ^^std && std::meta::parent_of(p) == ^^std && std::meta::identifier_of(p) == "__y1";
}

static_assert(in_plain_std(^^std::align_val_t));
static_assert(in_plain_std(^^std::destroying_delete_t));
static_assert(in_plain_std(^^std::nothrow_t));
static_assert(in_plain_std(^^std::nothrow));
static_assert(in_plain_std(^^std::initializer_list));
static_assert(in_plain_std(^^std::byte));
static_assert(in_plain_std(^^std::to_integer));
static_assert(in_plain_std(^^std::terminate));
static_assert(in_plain_std(^^std::meta));

static_assert(in_abi_namespace(^^std::exception));
static_assert(in_abi_namespace(^^std::type_info));
static_assert(in_abi_namespace(^^std::bad_alloc));
static_assert(in_abi_namespace(^^std::set_terminate));
static_assert(in_abi_namespace(^^std::vector));
static_assert(in_abi_namespace(^^std::basic_string));
static_assert(in_abi_namespace(^^std::is_same));
