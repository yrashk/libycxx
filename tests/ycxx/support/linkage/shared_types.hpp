// Interface between linkage/shared_library_type_identity.pass.cpp and the shared library built
// from shared_types_lib.cpp.
#pragma once
#include <exception>
#include <thread>
#include <typeindex>
#include <typeinfo>

// A polymorphic hierarchy defined in this header only (no key function: its vtables and
// type_info objects have vague linkage in both images).
struct VBase {
  virtual ~VBase() = default;
  virtual int which() const { return 0; }
};
struct VLeft : virtual VBase {
  int which() const override { return 1; }
};
struct VRight : virtual VBase {
  int which() const override { return 2; }
};
struct VBottom : VLeft, VRight {
  int which() const override { return 3; }
};
template <class T>
struct VTemplate : VBase {
  T value{};
  int which() const override { return 4; }
};

// A class whose key function is defined in the shared library.
struct __attribute__((visibility("default"))) Keyed : VBase {
  Keyed();
  ~Keyed() override;
  int which() const override;
};

// The types compared below (same order on both sides).
constexpr int n_types = 12;

// typeid of the which-th type, evaluated in the shared library.
__attribute__((visibility("default"))) const std::type_info& lib_typeid(int which);
// typeid of the dynamic type of *p, evaluated in the shared library.
__attribute__((visibility("default"))) const std::type_info& lib_dynamic_typeid(const VBase* p);
// An object of a VBase-derived class made in the shared library: 0 VBase, 1 VLeft, 2 VRight,
// 3 VBottom, 4 VTemplate<long>, 5 Keyed. The caller deletes it.
__attribute__((visibility("default"))) VBase* lib_make(int which);
// A standard exception made in the shared library: 0 runtime_error, 1 out_of_range,
// 2 system_error, 3 filesystem::filesystem_error, 4 bad_alloc, 5 nested_exception holder.
// The caller deletes it.
__attribute__((visibility("default"))) std::exception* lib_make_std(int which);
// dynamic_cast<To*>(p) != nullptr in the shared library for To: 0 runtime_error,
// 1 logic_error, 2 out_of_range, 3 system_error, 4 bad_alloc, 5 nested_exception.
__attribute__((visibility("default"))) bool lib_std_cast(const std::exception* p, int to);
// dynamic_cast in the shared library: To 0 VLeft, 1 VRight, 2 VBottom, 3 Keyed, 4 void.
__attribute__((visibility("default"))) const void* lib_cast(const VBase* p, int to);
// A copy, made in the shared library, of an exception_ptr.
__attribute__((visibility("default"))) std::exception_ptr lib_copy(const std::exception_ptr& p);
// this_thread::get_id() in the shared library.
__attribute__((visibility("default"))) std::thread::id lib_thread_id();
