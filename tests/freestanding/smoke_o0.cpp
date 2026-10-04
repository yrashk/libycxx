// Freestanding smoke test, compiled at -O0 so that nothing is inlined or folded away: every
// symbol these uses need must be defined without libsupc++ or libc.
#include <exception>
#include <new>
#include <optional>
#include <variant>

namespace {
struct my_error : std::exception {
  const char* what() const noexcept override { return "my_error"; }
};
} // namespace

extern "C" int ycxx_freestanding_o0() {
  const std::nothrow_t* nt = &std::nothrow; // the object must be defined
  my_error e;                               // ~exception() must be defined
  std::bad_optional_access boa;
  std::bad_variant_access bva;
  const std::exception& base = e;
  return (nt != nullptr) + (base.what()[0] == 'm') + (boa.what() != nullptr) + (bva.what() != nullptr);
}
