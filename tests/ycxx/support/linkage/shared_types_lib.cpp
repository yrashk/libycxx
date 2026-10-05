// The shared library of linkage/shared_library_type_identity.pass.cpp.
#include "shared_types.hpp"
#include <filesystem>
#include <functional>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

Keyed::Keyed() = default;
Keyed::~Keyed() = default;
int Keyed::which() const { return 5; }

namespace {
struct Nested : std::runtime_error, std::nested_exception {
  Nested() : std::runtime_error("nested") {}
};
}  // namespace

const std::type_info& lib_typeid(int which) {
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

const std::type_info& lib_dynamic_typeid(const VBase* p) { return typeid(*p); }

VBase* lib_make(int which, void* s) {
  switch (which) {
    case 0: return ::new (s) VBase;
    case 1: return ::new (s) VLeft;
    case 2: return ::new (s) VRight;
    case 3: return static_cast<VLeft*>(::new (s) VBottom);
    case 4: return ::new (s) VTemplate<long>;
    default: return ::new (s) Keyed;
  }
}

std::exception* lib_make_std(int which, void* s) {
  switch (which) {
    case 0: return ::new (s) std::runtime_error("runtime");
    case 1: return ::new (s) std::out_of_range("range");
    case 2: return ::new (s) std::system_error(std::make_error_code(std::errc::invalid_argument));
    case 3: return ::new (s) std::filesystem::filesystem_error("fs", std::make_error_code(std::errc::io_error));
    case 4: return ::new (s) std::bad_alloc;
    default: return ::new (s) Nested;
  }
}

bool lib_std_cast(const std::exception* p, int to) {
  switch (to) {
    case 0: return dynamic_cast<const std::runtime_error*>(p) != nullptr;
    case 1: return dynamic_cast<const std::logic_error*>(p) != nullptr;
    case 2: return dynamic_cast<const std::out_of_range*>(p) != nullptr;
    case 3: return dynamic_cast<const std::system_error*>(p) != nullptr;
    case 4: return dynamic_cast<const std::bad_alloc*>(p) != nullptr;
    default: return dynamic_cast<const std::nested_exception*>(p) != nullptr;
  }
}

const void* lib_cast(const VBase* p, int to) {
  switch (to) {
    case 0: return dynamic_cast<const VLeft*>(p);
    case 1: return dynamic_cast<const VRight*>(p);
    case 2: return dynamic_cast<const VBottom*>(p);
    case 3: return dynamic_cast<const Keyed*>(p);
    default: return dynamic_cast<const void*>(p);
  }
}

std::exception_ptr lib_copy(const std::exception_ptr& p) {
  std::exception_ptr q = p;
  return q;
}

std::thread::id lib_thread_id() { return std::this_thread::get_id(); }
