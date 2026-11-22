// Hosted nothrow forms follow the selected ordinary allocation functions.
#pragma once

#include "try_allocate.hpp"
#include "../../runtime/new/default_identity.hpp"

#if !_YCXX_HAS_EXCEPTIONS && _YCXX_NEW_ELF_ALIAS
// A weak reference alone cannot pull an ordinary allocation member from the archive.
extern "C" [[__gnu__::__weak__, __gnu__::__visibility__("hidden")]]
void* __ycxx_hosted_new_identity(std::size_t);
extern "C" [[__gnu__::__weak__, __gnu__::__visibility__("hidden")]]
void* __ycxx_hosted_new_align_identity(std::size_t, std::align_val_t);
extern "C" [[__gnu__::__weak__, __gnu__::__visibility__("hidden")]]
void* __ycxx_hosted_new_array_identity(std::size_t);
extern "C" [[__gnu__::__weak__, __gnu__::__visibility__("hidden")]]
void* __ycxx_hosted_new_array_align_identity(std::size_t, std::align_val_t);
#endif

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

enum class __allocation_form { __scalar, __array };

#if !_YCXX_HAS_EXCEPTIONS && _YCXX_NEW_ELF_ALIAS
inline constexpr bool __has_new_default_identity = true;

inline bool __is_new_default(__allocation_form __form, void* (*__selected)(std::size_t)) noexcept {
  auto __identity = __form == __allocation_form::__array ? &__ycxx_hosted_new_array_identity
                                                        : &__ycxx_hosted_new_identity;
  return __identity && __selected == __identity;
}

inline bool __is_new_default(__allocation_form __form,
                             void* (*__selected)(std::size_t, std::align_val_t)) noexcept {
  auto __identity = __form == __allocation_form::__array ? &__ycxx_hosted_new_array_align_identity
                                                        : &__ycxx_hosted_new_align_identity;
  return __identity && __selected == __identity;
}
#else
inline constexpr bool __has_new_default_identity = false;
#endif

// [new.delete.single], [new.delete.array]: an ordinary replacement receives its
// original arguments. The default ordinary array form calls the ordinary scalar
// form, so its nullable route must also honor a scalar replacement.
template <__allocation_form _Form, class... _Args>
void* __try_ordinary_new(std::size_t __n, _Args... __args) noexcept {
  using _Fn = void* (*)(std::size_t, _Args...);
  const _Fn __selected = [] {
    if constexpr (_Form == __allocation_form::__array)
      return static_cast<_Fn>(&::operator new[]);
    else
      return static_cast<_Fn>(&::operator new);
  }();

  if constexpr (__cfg::exceptions) {
    try {
      return __selected(__n, __args...);
    } catch (...) {
      return nullptr;
    }
  } else if constexpr (__has_new_default_identity) {
    if (!__is_new_default(_Form, __selected))
      return __selected(__n, __args...);
    if constexpr (_Form == __allocation_form::__array)
      return __try_ordinary_new<__allocation_form::__scalar>(__n, __args...);
    else if constexpr (sizeof...(_Args) == 0)
      return __try_allocate(__n, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
    else
      return __try_allocate(__n, static_cast<std::size_t>(__args)...);
  } else {
    static_assert(__has_new_default_identity,
                  "libycxx: exception-disabled hosted allocation needs a supported default identity");
  }
}

}} // namespace __ycxx::__detail
