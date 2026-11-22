// libycxx ABI runtime: a typed allocation error for the exception-disabled scheduler,
// including when its translation unit also lacks RTTI. This TU is built with both enabled.
#include <exception>
#include <new>
#include <ycxx/core/error.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __abi {

std::exception_ptr __parallel_scheduler_bad_alloc() noexcept {
  try {
    ::__ycxx::__detail::__raise_with(ycxx_error_bad_alloc, "std::bad_alloc", [] { return std::bad_alloc(); });
  } catch (...) {
    return std::current_exception();
  }
}

}} // namespace __ycxx::__abi
