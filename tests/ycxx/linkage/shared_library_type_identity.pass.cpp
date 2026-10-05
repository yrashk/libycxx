// Type identity between a program and a shared library that both use the library (linked
// through the same wrapper, so with a static libycxx each image has its own hidden copy of the
// library and of its ABI runtime). The standard has no shared libraries; what is checked is
// what holds for types in one program, whose translation units here are split between two
// images. A type is the same type in every translation unit ([basic.def.odr]/15: several
// definitions "behave as if there were a single definition"), standard library types included,
// so:
//   [type.info]/3: operator== "Returns: true if the two values describe the same type";
//     /6 hash_code(): "within a single execution of the program, it shall return the same
//     value for any two type_info objects which compare equal"; [expr.typeid]/3: typeid of a
//     polymorphic glvalue refers to the type_info of its most derived type;
//   [type.index]/3, /8: type_index == and <=> (equal when the type_infos compare equal), /9,
//     /11: hash<type_index> is hash_code(): an unordered_map keyed by the program's type_index
//     finds the shared library's;
//   [expr.dynamic.cast]/7-9: down, cross and void* casts succeed exactly when the most derived
//     object has the target as an unambiguous public base, whichever image made the object
//     and whichever image evaluates the cast (user hierarchies with vague linkage or a key
//     function in the shared library; the standard exception classes);
//   [propagation]/3: "Two non-null values of type exception_ptr are equivalent and compare
//     equal if and only if they refer to the same exception";
//   [thread.thread.id]/1: the same thread has the same thread::id on both sides.
// Not checked: identity of the library's singleton OBJECTS (error categories, the time zone
// database, memory resources), which each copy of the library has on its own; the standard,
// knowing no shared libraries, says nothing about such a split.
// FLAGS: -fPIC -pthread
// SHARED: ../support/linkage/shared_types_lib.cpp
#include "../support/linkage/shared_types.hpp"
#include <compare>
#include <filesystem>
#include <functional>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "check.hpp"

static const std::type_info& here_typeid(int which) {
  switch (which) {
    case 0: return typeid(int);
    case 1: return typeid(const char*);
    case 2: return typeid(std::string);
    case 3: return typeid(std::vector<int>);
    case 4: return typeid(std::map<std::string, std::vector<double>>);
    case 5: return typeid(std::runtime_error);
    case 6: return typeid(std::error_code);
    case 7: return typeid(std::function<int(long)>);
    case 8: return typeid(VBottom);
    case 9: return typeid(VTemplate<long>);
    case 10: return typeid(Keyed);
    default: return typeid(std::filesystem::path);
  }
}

int main() {
  // typeid, hash_code, type_index.
  std::unordered_map<std::type_index, int> index;
  for (int i = 0; i < n_types; ++i) index.emplace(here_typeid(i), i);
  CHECK(index.size() == std::size_t(n_types));
  for (int i = 0; i < n_types; ++i) {
    for (int j = 0; j < n_types; ++j) {
      CHECK((lib_typeid(i) == here_typeid(j)) == (i == j));
      CHECK((here_typeid(j) == lib_typeid(i)) == (i == j));
    }
    CHECK(lib_typeid(i).hash_code() == here_typeid(i).hash_code());
    const std::type_index a(lib_typeid(i)), b(here_typeid(i));
    CHECK(a == b && (a <=> b) == std::strong_ordering::equal);
    CHECK(std::hash<std::type_index>()(a) == std::hash<std::type_index>()(b));
    const auto it = index.find(a);
    CHECK(it != index.end() && it->second == i);
  }

  // Objects of user types made in the shared library.
  for (int w = 0; w <= 5; ++w) {
    alignas(64) unsigned char storage[storage_size];
    VBase* p = lib_make(w, storage);
    CHECK(p->which() == w);
    const std::type_info& dyn = typeid(*p);
    CHECK(dyn == lib_dynamic_typeid(p));
    const bool left = w == 1 || w == 3, right = w == 2 || w == 3;
    CHECK((dynamic_cast<VLeft*>(p) != nullptr) == left);
    CHECK((dynamic_cast<VRight*>(p) != nullptr) == right);
    CHECK((dynamic_cast<VBottom*>(p) != nullptr) == (w == 3));
    CHECK((dynamic_cast<VTemplate<long>*>(p) != nullptr) == (w == 4));
    CHECK((dynamic_cast<VTemplate<int>*>(p) != nullptr) == false);
    CHECK((dynamic_cast<Keyed*>(p) != nullptr) == (w == 5));
    CHECK((w == 3) == (dyn == typeid(VBottom)));
    CHECK((w == 5) == (dyn == typeid(Keyed)));
    if (w == 3) {
      VBottom* b = dynamic_cast<VBottom*>(p);
      CHECK(dynamic_cast<void*>(p) == static_cast<void*>(b));
      CHECK(static_cast<VBase*>(dynamic_cast<VRight*>(p)) == static_cast<VBase*>(b));
    }
    p->~VBase();
  }
  // Objects made here, cast in the shared library.
  {
    VBottom bottom;
    VBase* vb = static_cast<VLeft*>(&bottom);
    CHECK(lib_cast(vb, 0) == static_cast<VLeft*>(&bottom));
    CHECK(lib_cast(vb, 1) == static_cast<VRight*>(&bottom));
    CHECK(lib_cast(vb, 2) == &bottom);
    CHECK(lib_cast(vb, 3) == nullptr);
    CHECK(lib_cast(vb, 4) == static_cast<void*>(&bottom));
    CHECK(lib_dynamic_typeid(vb) == typeid(VBottom));
    Keyed keyed;
    CHECK(lib_cast(&keyed, 3) == &keyed && lib_cast(&keyed, 0) == nullptr);
    VTemplate<long> t;
    CHECK(lib_dynamic_typeid(&t) == typeid(VTemplate<long>));
  }

  // Standard exception objects made in the shared library, cast here; made here, cast there.
  //   0 runtime_error, 1 out_of_range, 2 system_error, 3 filesystem_error, 4 bad_alloc,
  //   5 a class derived from runtime_error and nested_exception.
  for (int w = 0; w <= 5; ++w) {
    alignas(64) unsigned char storage[storage_size];
    std::exception* e = lib_make_std(w, storage);
    CHECK((dynamic_cast<std::runtime_error*>(e) != nullptr) == (w == 0 || w == 2 || w == 3 || w == 5));
    CHECK((dynamic_cast<std::logic_error*>(e) != nullptr) == (w == 1));
    CHECK((dynamic_cast<std::out_of_range*>(e) != nullptr) == (w == 1));
    CHECK((dynamic_cast<std::system_error*>(e) != nullptr) == (w == 2 || w == 3));
    CHECK((dynamic_cast<std::filesystem::filesystem_error*>(e) != nullptr) == (w == 3));
    CHECK((dynamic_cast<std::bad_alloc*>(e) != nullptr) == (w == 4));
    CHECK((dynamic_cast<std::nested_exception*>(e) != nullptr) == (w == 5));
    CHECK((typeid(*e) == typeid(std::runtime_error)) == (w == 0));
    CHECK((typeid(*e) == typeid(std::out_of_range)) == (w == 1));
    e->~exception();
  }
  {
    const std::runtime_error re("r");
    const std::out_of_range oor("o");
    const std::system_error se(std::make_error_code(std::errc::io_error));
    const std::bad_alloc ba;
    CHECK(lib_std_cast(&re, 0) && !lib_std_cast(&re, 1) && !lib_std_cast(&re, 3));
    CHECK(lib_std_cast(&oor, 1) && lib_std_cast(&oor, 2) && !lib_std_cast(&oor, 0));
    CHECK(lib_std_cast(&se, 0) && lib_std_cast(&se, 3) && !lib_std_cast(&se, 2));
    CHECK(lib_std_cast(&ba, 4) && !lib_std_cast(&ba, 0) && !lib_std_cast(&ba, 5));
  }

  // exception_ptr copies made on the other side refer to the same exception.
  std::exception_ptr p = std::make_exception_ptr(std::length_error("len"));
  std::exception_ptr q = lib_copy(p);
  CHECK(q == p);
  CHECK(lib_copy(nullptr) == nullptr);
  CHECK(!(q == std::make_exception_ptr(std::length_error("len"))));

  // The same thread has the same id; other threads have other ids.
  CHECK(lib_thread_id() == std::this_thread::get_id());
  std::thread::id other_lib, other_here;
  std::thread t([&] {
    other_lib = lib_thread_id();
    other_here = std::this_thread::get_id();
  });
  t.join();
  CHECK(other_lib == other_here && other_lib != std::this_thread::get_id());
  return 0;
}
