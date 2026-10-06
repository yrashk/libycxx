// libycxx core: the formatting library ([format]) except the formatters of ranges and tuples
// (format_ranges.hpp) and the locale-specific parts (ycxx/hosted/format_locale.hpp). The primary
// formatter, formattable and the formatters of [format.formatter.spec] are in format_decl.hpp
// (every header declaring a formatter specialization provides them); the functions their members
// call are defined here.
//
// Output. Every basic_format_context the library creates writes through
// __ycxx::__adl_free::__fmt_iter<charT>, an output iterator appending to a type-erased buffer
// (fmt_buf: an array plus a function pointer that makes room by flushing it to the destination
// or by growing it), so format_context and wformat_context are one type each
// ([format.context]/5) and formatting needs no allocation unless the result does.
//
// Checking. basic_format_string's consteval constructor scans the format string with the code
// vformat uses and calls formatter<remove_cvref_t<Args_i>, charT>::parse for each replacement
// field in constant evaluation; the parse context then knows the number and the kinds of the
// arguments, so next_arg_id/check_arg_id/check_dynamic_spec reject a bad index or kind there
// ([format.parse.ctx]/10, /13, /15). An invalid string is not a constant expression (a
// format_error thrown in constant evaluation, or a call of a non-constexpr function named after
// the problem); at run time the same code throws format_error.
//
// Locale. basic_format_context keeps a pointer to the locale passed to the formatting function
// (null: std::locale()). Its locale() member and the numpunct lookups of the L option are only
// declared here; <format> defines them (ycxx/hosted/format_locale.hpp).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/charconv.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/format_decl.hpp>
#include <ycxx/core/format_unicode.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {
class locale;

// [format.error]
class format_error : public runtime_error {
public:
  constexpr explicit format_error(const string& __what_arg) : runtime_error(__what_arg) {}
  constexpr explicit format_error(const char* __what_arg) : runtime_error(__what_arg) {}
};

template <class _Context>
class basic_format_arg;
template <class _Context>
class basic_format_args;
template <class __charT, class... _Args>
struct basic_format_string;

template <class _Out>
struct format_to_n_result {
  _Out out;
  iter_difference_t<_Out> size;
};
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// Not constexpr: a call during the compile-time check of a format string makes the string
// ill-formed; the name (or the argument, which the diagnostic shows) says why.
inline void __format_error_in_constant_evaluation(const char*) noexcept {}
inline void __format_string_argument_index_out_of_range() noexcept {}
inline void __format_string_dynamic_argument_has_wrong_type() noexcept {}

[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_format_error(const char* what) {
  if consteval {
    // Clang cannot throw during constant evaluation; this shows the message instead.
    if constexpr (__cfg::__y_clang)
      ::__ycxx::__detail::__format_error_in_constant_evaluation(what);
  }
  ::__ycxx::__detail::__raise_with(ycxx_error_format_error, what, [what] { return std::format_error(what); });
}

// The alternatives of basic_format_arg ([format.arg]/1), in the order of the exposition-only
// variant.
enum class __fmt_kind : unsigned char {
  none,
  __boolean,
  character,
  __int_,
  __uint_,
  __llong,
  __ullong,
  __float_,
  __double_,
  __ldouble,
  __cstring,
  string,
  pointer,
  handle
};

template <class _Tp>
inline constexpr bool __fmt_is_string_view = false;
template <class _Cp, class _Tr>
inline constexpr bool __fmt_is_string_view<std::basic_string_view<_Cp, _Tr>> = true;
template <class _Tp>
inline constexpr bool __fmt_is_string = false;
template <class _Cp, class _Tr, class _Ap>
inline constexpr bool __fmt_is_string<std::basic_string<_Cp, _Tr, _Ap>> = true;

// The alternative a (cv-unqualified) TD takes in basic_format_arg<Context> with char-type charT
// ([format.arg]/6).
template <class _TD, class __charT>
consteval __fmt_kind __fmt_kind_of() {
  using _Dp = std::decay_t<_TD>;
  if constexpr (__is_same(_TD, bool))
    return __fmt_kind::__boolean;
  else if constexpr (__is_same(_TD, __charT) || (__is_same(_TD, char) && __is_same(__charT, wchar_t)))
    return __fmt_kind::character;
  else if constexpr (::__ycxx::__detail::__is_standard_signed_integer<_TD> && sizeof(_TD) <= sizeof(int))
    return __fmt_kind::__int_;
  else if constexpr (::__ycxx::__detail::__is_standard_unsigned_integer<_TD> && sizeof(_TD) <= sizeof(unsigned))
    return __fmt_kind::__uint_;
  else if constexpr (::__ycxx::__detail::__is_standard_signed_integer<_TD> && sizeof(_TD) <= sizeof(long long))
    return __fmt_kind::__llong;
  else if constexpr (::__ycxx::__detail::__is_standard_unsigned_integer<_TD> && sizeof(_TD) <= sizeof(unsigned long long))
    return __fmt_kind::__ullong;
  else if constexpr (__is_same(_TD, float))
    return __fmt_kind::__float_;
  else if constexpr (__is_same(_TD, double))
    return __fmt_kind::__double_;
  else if constexpr (__is_same(_TD, long double))
    return __fmt_kind::__ldouble;
  else if constexpr ((__fmt_is_string_view<_TD> || __fmt_is_string<_TD>) && requires {
                       requires __is_same(typename _TD::value_type, __charT);
                     })
    return __fmt_kind::string;
  else if constexpr (__is_same(_Dp, __charT*) || __is_same(_Dp, const __charT*))
    return __fmt_kind::__cstring;
  else if constexpr (std::is_void_v<std::remove_pointer_t<_TD>> || __is_same(_TD, decltype(nullptr)))
    return __fmt_kind::pointer;
  else
    return __fmt_kind::handle;
}

struct __fmt_access;

template <std::size_t _Np>
consteval bool __fmt_unique(const __fmt_kind (&k)[_Np]) {
  for (std::size_t i = 0; i != _Np; ++i)
    for (std::size_t __j = i + 1; __j != _Np; ++__j)
      if (k[i] == k[__j])
        return false;
  return true;
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// The type-erased output buffer behind fmt_iter: [data_, data_ + size_) holds pending output;
// make_room_ is called when size_ == cap_ and leaves size_ < cap_ (by flushing the contents to
// the destination, or by growing the storage). A destination that no longer needs the characters
// (a counter past its limit) sets discard_; fill then only counts them in discarded_, so a huge
// width or precision costs no time there.
template <class __charT>
class __fmt_buf {
public:
  using __make_room_fn = void (*)(__fmt_buf&);

  constexpr __fmt_buf(__charT* data, std::size_t __cap, __make_room_fn __f) noexcept : __data_(data), __cap_(__cap), __make_room_(__f) {}
  __fmt_buf(const __fmt_buf&) = delete;
  __fmt_buf& operator=(const __fmt_buf&) = delete;

  constexpr void push_back(__charT c) {
    if (__size_ == __cap_)
      __make_room_(*this);
    __data_[__size_++] = c;
  }
  constexpr void append(const __charT* p, std::size_t n) {
    while (n != 0) {
      if (__size_ == __cap_)
        __make_room_(*this);
      std::size_t k = __cap_ - __size_;
      if (k > n)
        k = n;
      if consteval {
        for (std::size_t i = 0; i != k; ++i)
          __data_[__size_ + i] = p[i];
      } else {
        __builtin_memcpy(static_cast<void*>(__data_ + __size_), static_cast<const void*>(p), k * sizeof(__charT));
      }
      __size_ += k;
      p += k;
      n -= k;
    }
  }
  constexpr void fill(std::size_t n, __charT c) {
    if (__discard_)
      return discard(n);
    while (n != 0) {
      if (__size_ == __cap_)
        __make_room_(*this);
      std::size_t k = __cap_ - __size_;
      if (k > n)
        k = n;
      for (std::size_t i = 0; i != k; ++i)
        __data_[__size_ + i] = c;
      __size_ += k;
      n -= k;
    }
  }

  // Counts n characters that are not stored (saturating).
  constexpr void discard(std::size_t n) noexcept {
    __discarded_ = n > static_cast<std::size_t>(-1) - __discarded_ ? static_cast<std::size_t>(-1) : __discarded_ + n;
  }

  __charT* __data_;
  std::size_t __size_ = 0;
  std::size_t __cap_;
  __make_room_fn __make_room_;
  bool __discard_ = false;
  std::size_t __discarded_ = 0;
};

// format_context::iterator ([format.context]/4): appends to a fmt_buf.
template <class __charT>
class __fmt_iter {
  __fmt_buf<__charT>* __buf_ = nullptr;
  friend __ycxx::__detail::__fmt_access;

public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;

  constexpr __fmt_iter() noexcept = default;
  constexpr explicit __fmt_iter(__fmt_buf<__charT>& b) noexcept : __buf_(__builtin_addressof(b)) {}
  constexpr __fmt_iter& operator=(const __charT& c) {
    __buf_->push_back(c);
    return *this;
  }
  constexpr __fmt_iter& operator*() noexcept { return *this; }
  constexpr __fmt_iter& operator++() noexcept { return *this; }
  constexpr __fmt_iter operator++(int) noexcept { return *this; }
};

// dynamic-format-string ([format.syn]): the result of dynamic_format / runtime_format.
template <class __charT>
struct __dynamic_format_string {
private:
  std::basic_string_view<__charT> __str_;
  template <class _Cp, class... _Args>
  friend struct std::basic_format_string;

public:
  constexpr __dynamic_format_string(std::basic_string_view<__charT> s) noexcept : __str_(s) {}
  __dynamic_format_string(const __dynamic_format_string&) = delete;
  __dynamic_format_string& operator=(const __dynamic_format_string&) = delete;
};

// format-arg-store ([format.arg.store]).
template <class _Context, class... _Args>
class __fmt_arg_store {
  friend std::basic_format_args<_Context>;
  std::basic_format_arg<_Context> __args_[sizeof...(_Args) == 0 ? 1 : sizeof...(_Args)];

public:
  constexpr explicit __fmt_arg_store(_Args&... a) noexcept;
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

// [format.parse.ctx]
template <class __charT>
class basic_format_parse_context {
public:
  using char_type = __charT;
  using const_iterator = typename basic_string_view<__charT>::const_iterator;
  using iterator = const_iterator;

private:
  enum __indexing : unsigned char { unknown, __manual, __automatic };
  iterator __begin_;
  iterator __end_;
  __indexing __indexing_ = unknown;
  size_t __next_arg_id_ = 0;
  size_t __num_args_ = 0;
  // While a format string is checked at compile time: the kinds of the arguments.
  const __ycxx::__detail::__fmt_kind* __kinds_ = nullptr;
  friend __ycxx::__detail::__fmt_access;

  constexpr basic_format_parse_context(basic_string_view<__charT> __fmt, size_t __num_args,
                                       const __ycxx::__detail::__fmt_kind* __kinds) noexcept
      : __begin_(__fmt.begin()), __end_(__fmt.end()), __num_args_(__num_args), __kinds_(__kinds) {}

  template <class _Tp>
  static consteval __ycxx::__detail::__fmt_kind __kind_of() {
    static_assert(__ycxx::__detail::__is_any_of<_Tp, bool, __charT, int, unsigned, long long, unsigned long long, float, double,
                                          long double, const __charT*, basic_string_view<__charT>, const void*>,
                  "std::basic_format_parse_context::check_dynamic_spec: Ts must be bool, char_type, int, unsigned "
                  "int, long long int, unsigned long long int, float, double, long double, const char_type*, "
                  "basic_string_view<char_type> or const void*");
    return __ycxx::__detail::__fmt_kind_of<_Tp, __charT>();
  }

public:
  constexpr explicit basic_format_parse_context(basic_string_view<__charT> __fmt) noexcept
      : __begin_(__fmt.begin()), __end_(__fmt.end()) {}
  basic_format_parse_context(const basic_format_parse_context&) = delete;
  basic_format_parse_context& operator=(const basic_format_parse_context&) = delete;

  constexpr const_iterator begin() const noexcept { return __begin_; }
  constexpr const_iterator end() const noexcept { return __end_; }
  constexpr void advance_to(const_iterator __it) { __begin_ = __it; }

  constexpr size_t next_arg_id() {
    if (__indexing_ == __manual)
      __ycxx::__detail::__throw_format_error("std::format: automatic and manual argument indexing are mixed");
    __indexing_ = __automatic;
    if consteval {
      if (__next_arg_id_ >= __num_args_)
        __ycxx::__detail::__format_string_argument_index_out_of_range();
    }
    return __next_arg_id_++;
  }
  constexpr void check_arg_id(size_t id) {
    if (__indexing_ == __automatic)
      __ycxx::__detail::__throw_format_error("std::format: automatic and manual argument indexing are mixed");
    __indexing_ = __manual;
    if consteval {
      if (id >= __num_args_)
        __ycxx::__detail::__format_string_argument_index_out_of_range();
    }
  }
  template <class... _Ts>
  constexpr void check_dynamic_spec(size_t id) noexcept {
    static_assert(sizeof...(_Ts) >= 1, "std::basic_format_parse_context::check_dynamic_spec: Ts must not be empty");
    constexpr __ycxx::__detail::__fmt_kind __kinds[] = {__kind_of<_Ts>()...};
    static_assert(__ycxx::__detail::__fmt_unique(__kinds), "std::basic_format_parse_context::check_dynamic_spec: the types in Ts must be unique");
    if consteval {
      if (id >= __num_args_)
        __ycxx::__detail::__format_string_argument_index_out_of_range();
      else if (__kinds_ != nullptr) {
        bool found = false;
        for (__ycxx::__detail::__fmt_kind k : __kinds)
          found = found || k == __kinds_[id];
        if (!found)
          __ycxx::__detail::__format_string_dynamic_argument_has_wrong_type();
      }
    }
  }
  constexpr void check_dynamic_spec_integral(size_t id) noexcept {
    check_dynamic_spec<int, unsigned int, long long int, unsigned long long int>(id);
  }
  constexpr void check_dynamic_spec_string(size_t id) noexcept {
    check_dynamic_spec<const char_type*, basic_string_view<char_type>>(id);
  }
};
using format_parse_context = basic_format_parse_context<char>;
using wformat_parse_context = basic_format_parse_context<wchar_t>;
// The parse members of format_decl.hpp's formatters return const charT*.
static_assert(__is_same(format_parse_context::iterator, const char*) &&
              __is_same(wformat_parse_context::iterator, const wchar_t*));

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] std {

// [format.arg]
template <class _Context>
class basic_format_arg {
  using char_type = typename _Context::char_type;

public:
  class handle {
    const void* __ptr_;
    void (*__format_)(basic_format_parse_context<char_type>&, _Context&, const void*);
    friend class basic_format_arg;

    template <class _Tp>
      requires(!__is_same(remove_cv_t<_Tp>, handle))
    constexpr explicit handle(_Tp& __val) noexcept : __ptr_(__builtin_addressof(__val)) {
      using _TD = remove_const_t<_Tp>;
      using _TQ = conditional_t<__ycxx::__detail::__fmt_formattable_with<const _TD, _Context>, const _TD, _TD>;
      static_assert(__ycxx::__detail::__fmt_formattable_with<_TQ, _Context>,
                    "std::basic_format_arg::handle: the argument type is not formattable");
      __format_ = [](basic_format_parse_context<char_type>& __parse_ctx, _Context& __format_ctx, const void* ptr) {
        typename _Context::template formatter_type<_TD> __f;
        __parse_ctx.advance_to(__f.parse(__parse_ctx));
        __format_ctx.advance_to(__f.format(*const_cast<_TQ*>(static_cast<const _TD*>(ptr)), __format_ctx));
      };
    }

  public:
    constexpr void format(basic_format_parse_context<char_type>& __parse_ctx, _Context& __format_ctx) const {
      __format_(__parse_ctx, __format_ctx, __ptr_);
    }
  };

private:
  friend __ycxx::__detail::__fmt_access;
  template <class _Cp, class... _Args>
  friend class __ycxx::__adl_free::__fmt_arg_store;

  union value {
    monostate none;
    bool b;
    char_type c;
    int i;
    unsigned __u;
    long long __ll;
    unsigned long long __ull;
    float __f;
    double d;
    long double __ld;
    const char_type* s;
    basic_string_view<char_type> sv;
    const void* p;
    handle h;
    constexpr value() noexcept : none() {}
    constexpr value(bool __v) noexcept : b(__v) {}
    constexpr value(char_type __v) noexcept : c(__v) {}
    constexpr value(int __v) noexcept : i(__v) {}
    constexpr value(unsigned __v) noexcept : __u(__v) {}
    constexpr value(long long __v) noexcept : __ll(__v) {}
    constexpr value(unsigned long long __v) noexcept : __ull(__v) {}
    constexpr value(float __v) noexcept : __f(__v) {}
    constexpr value(double __v) noexcept : d(__v) {}
    constexpr value(long double __v) noexcept : __ld(__v) {}
    constexpr value(const char_type* __v) noexcept : s(__v) {}
    constexpr value(basic_string_view<char_type> __v) noexcept : sv(__v) {}
    constexpr value(const void* __v) noexcept : p(__v) {}
    constexpr value(handle __v) noexcept : h(__v) {}
  };

  __ycxx::__detail::__fmt_kind __kind_ = __ycxx::__detail::__fmt_kind::none;
  value __v_;

  // [format.arg]/4-6.
  template <class _Tp>
  constexpr explicit basic_format_arg(_Tp& __v) noexcept;

public:
  constexpr basic_format_arg() noexcept {}
  constexpr explicit operator bool() const noexcept { return __kind_ != __ycxx::__detail::__fmt_kind::none; }

  template <class _Visitor>
  constexpr decltype(auto) visit(this basic_format_arg arg, _Visitor&& __vis) {
    using _Kp = __ycxx::__detail::__fmt_kind;
    switch (arg.__kind_) {
    case _Kp::__boolean: return static_cast<_Visitor&&>(__vis)(arg.__v_.b);
    case _Kp::character: return static_cast<_Visitor&&>(__vis)(arg.__v_.c);
    case _Kp::__int_: return static_cast<_Visitor&&>(__vis)(arg.__v_.i);
    case _Kp::__uint_: return static_cast<_Visitor&&>(__vis)(arg.__v_.__u);
    case _Kp::__llong: return static_cast<_Visitor&&>(__vis)(arg.__v_.__ll);
    case _Kp::__ullong: return static_cast<_Visitor&&>(__vis)(arg.__v_.__ull);
    case _Kp::__float_: return static_cast<_Visitor&&>(__vis)(arg.__v_.__f);
    case _Kp::__double_: return static_cast<_Visitor&&>(__vis)(arg.__v_.d);
    case _Kp::__ldouble: return static_cast<_Visitor&&>(__vis)(arg.__v_.__ld);
    case _Kp::__cstring: return static_cast<_Visitor&&>(__vis)(arg.__v_.s);
    case _Kp::string: return static_cast<_Visitor&&>(__vis)(arg.__v_.sv);
    case _Kp::pointer: return static_cast<_Visitor&&>(__vis)(arg.__v_.p);
    case _Kp::handle: return static_cast<_Visitor&&>(__vis)(arg.__v_.h);
    case _Kp::none: break;
    }
    return static_cast<_Visitor&&>(__vis)(arg.__v_.none);
  }
  template <class _Rp, class _Visitor>
  constexpr _Rp visit(this basic_format_arg arg, _Visitor&& __vis) {
    return arg.visit([&__vis](auto& __x) -> _Rp {
      if constexpr (is_void_v<_Rp>)
        static_cast<void>(static_cast<_Visitor&&>(__vis)(__x));
      else
        return static_cast<_Visitor&&>(__vis)(__x);
    });
  }
};

// [depr.format.arg] (Annex D)
template <class _Visitor, class _Context>
[[deprecated("visit_format_arg is deprecated ([depr.format.arg]); use basic_format_arg::visit")]]
decltype(auto) visit_format_arg(_Visitor&& __vis, basic_format_arg<_Context> arg) {
  return static_cast<basic_format_arg<_Context>&&>(arg).visit(static_cast<_Visitor&&>(__vis));
}

// [format.args]
template <class _Context>
class basic_format_args {
  size_t __size_;
  const basic_format_arg<_Context>* __data_;
  friend __ycxx::__detail::__fmt_access;

public:
  template <class... _Args>
  constexpr basic_format_args(const __ycxx::__adl_free::__fmt_arg_store<_Context, _Args...>& store) noexcept
      : __size_(sizeof...(_Args)), __data_(store.__args_) {}
  constexpr basic_format_arg<_Context> get(size_t i) const noexcept {
    return i < __size_ ? __data_[i] : basic_format_arg<_Context>();
  }
};
template <class _Context, class... _Args>
basic_format_args(__ycxx::__adl_free::__fmt_arg_store<_Context, _Args...>) -> basic_format_args<_Context>;

// [format.context]
template <class _Out, class __charT>
class basic_format_context {
  basic_format_args<basic_format_context> __args_;
  _Out __out_;
  const std::locale* __loc_; // null: std::locale()
  friend __ycxx::__detail::__fmt_access;

  constexpr basic_format_context(_Out out, basic_format_args<basic_format_context> __args, const std::locale* __loc)
      : __args_(__args), __out_(static_cast<_Out&&>(out)), __loc_(__loc) {}

public:
  using iterator = _Out;
  using char_type = __charT;
  template <class _Tp>
  using formatter_type = formatter<_Tp, __charT>;

  basic_format_context(const basic_format_context&) = delete;
  basic_format_context& operator=(const basic_format_context&) = delete;

  constexpr basic_format_arg<basic_format_context> arg(size_t id) const noexcept { return __args_.get(id); }
  std::locale locale(); // defined in ycxx/hosted/format_locale.hpp
  constexpr iterator out() { return static_cast<_Out&&>(__out_); }
  constexpr void advance_to(iterator __it) { __out_ = static_cast<_Out&&>(__it); }
};

using format_context = basic_format_context<__ycxx::__adl_free::__fmt_iter<char>, char>;
using wformat_context = basic_format_context<__ycxx::__adl_free::__fmt_iter<wchar_t>, wchar_t>;
using format_args = basic_format_args<format_context>;
using wformat_args = basic_format_args<wformat_context>;

// [format.formattable]
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class __charT>
using __fmt_context = std::basic_format_context<__ycxx::__adl_free::__fmt_iter<__charT>, __charT>;
template <class __charT>
using __fmt_args = std::basic_format_args<__fmt_context<__charT>>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Context>
template <class _Tp>
constexpr basic_format_arg<_Context>::basic_format_arg(_Tp& __v) noexcept {
  static_assert(__ycxx::__detail::__fmt_formattable_with<_Tp, _Context>,
                "std::make_format_args: an argument type has no enabled formatter");
  using _TD = remove_const_t<_Tp>;
  using _Kp = __ycxx::__detail::__fmt_kind;
  constexpr _Kp k = __ycxx::__detail::__fmt_kind_of<_TD, char_type>();
  __kind_ = k;
  if constexpr (k == _Kp::__boolean)
    __v_ = value(static_cast<bool>(__v));
  else if constexpr (k == _Kp::character) {
    if constexpr (__is_same(_TD, char) && __is_same(char_type, wchar_t))
      __v_ = value(static_cast<wchar_t>(static_cast<unsigned char>(__v)));
    else
      __v_ = value(static_cast<char_type>(__v));
  } else if constexpr (k == _Kp::__int_)
    __v_ = value(static_cast<int>(__v));
  else if constexpr (k == _Kp::__uint_)
    __v_ = value(static_cast<unsigned>(__v));
  else if constexpr (k == _Kp::__llong)
    __v_ = value(static_cast<long long>(__v));
  else if constexpr (k == _Kp::__ullong)
    __v_ = value(static_cast<unsigned long long>(__v));
  else if constexpr (k == _Kp::__float_ || k == _Kp::__double_ || k == _Kp::__ldouble)
    __v_ = value(__v);
  else if constexpr (k == _Kp::string)
    __v_ = value(basic_string_view<char_type>(__v.data(), __v.size()));
  else if constexpr (k == _Kp::__cstring)
    __v_ = value(static_cast<const char_type*>(__v));
  else if constexpr (k == _Kp::pointer)
    __v_ = value(static_cast<const void*>(__v));
  else
    __v_ = value(handle(__v));
}

// [format.arg.store]
template <class _Context = format_context, class... _Args>
constexpr __ycxx::__adl_free::__fmt_arg_store<_Context, _Args...> make_format_args(_Args&... __fmt_args) {
  return __ycxx::__adl_free::__fmt_arg_store<_Context, _Args...>(__fmt_args...);
}
template <class... _Args>
constexpr __ycxx::__adl_free::__fmt_arg_store<wformat_context, _Args...> make_wformat_args(_Args&... __args) {
  return __ycxx::__adl_free::__fmt_arg_store<wformat_context, _Args...>(__args...);
}
} // namespace std

template <class _Context, class... _Args>
constexpr __ycxx::__adl_free::__fmt_arg_store<_Context, _Args...>::__fmt_arg_store(_Args&... a) noexcept
    : __args_{std::basic_format_arg<_Context>(a)...} {}

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// Access to the private members of the formatting classes.
struct __fmt_access {
  template <class __charT>
  static constexpr std::basic_format_parse_context<__charT> __parse_context(std::basic_string_view<__charT> __fmt,
                                                                        std::size_t __num_args, const __fmt_kind* __kinds) {
    return std::basic_format_parse_context<__charT>(__fmt, __num_args, __kinds);
  }
  template <class __charT>
  static constexpr __fmt_context<__charT> __context(__ycxx::__adl_free::__fmt_buf<__charT>& __buf, __fmt_args<__charT> __args,
                                              const std::locale* __loc) {
    return __fmt_context<__charT>(__ycxx::__adl_free::__fmt_iter<__charT>(__buf), __args, __loc);
  }
  // A context like ctx (same arguments and locale) writing to buf.
  template <class __charT>
  static constexpr __fmt_context<__charT> __context_like(const __fmt_context<__charT>& __ctx, __ycxx::__adl_free::__fmt_buf<__charT>& __buf) {
    return __fmt_context<__charT>(__ycxx::__adl_free::__fmt_iter<__charT>(__buf), __ctx.__args_, __ctx.__loc_);
  }
  template <class _Out, class __charT>
  static constexpr const std::locale* __locale_ptr(const std::basic_format_context<_Out, __charT>& __ctx) {
    return __ctx.__loc_;
  }
  template <class __charT>
  static constexpr __ycxx::__adl_free::__fmt_buf<__charT>& __buffer(__ycxx::__adl_free::__fmt_iter<__charT> __it) {
    return *__it.__buf_;
  }
  template <class _Context>
  static constexpr std::size_t size(const std::basic_format_args<_Context>& a) {
    return a.__size_;
  }
  template <class _Context>
  static constexpr __fmt_kind kind(const std::basic_format_arg<_Context>& a) {
    return a.__kind_;
  }
  template <class _Context>
  static constexpr const auto& value(const std::basic_format_arg<_Context>& a) {
    return a.__v_;
  }
};

// ---- output helpers ---------------------------------------------------------------------------

template <class __charT, class _Out>
constexpr _Out __fmt_put(_Out out, const __charT* p, std::size_t n) {
  if constexpr (__is_same(_Out, __ycxx::__adl_free::__fmt_iter<__charT>)) {
    __fmt_access::__buffer(out).append(p, n);
  } else {
    for (std::size_t i = 0; i != n; ++i) {
      *out = p[i];
      ++out;
    }
  }
  return out;
}
template <class __charT, class _Out>
constexpr _Out __fmt_put(_Out out, __charT c) {
  if constexpr (__is_same(_Out, __ycxx::__adl_free::__fmt_iter<__charT>)) {
    __fmt_access::__buffer(out).push_back(c);
  } else {
    *out = c;
    ++out;
  }
  return out;
}
// [p, p + n) of ASCII characters, widened to charT.
template <class __charT, class _Out>
constexpr _Out __fmt_put_ascii(_Out out, const char* p, std::size_t n) {
  if constexpr (__is_same(__charT, char)) {
    return ::__ycxx::__detail::__fmt_put<char>(static_cast<_Out&&>(out), p, n);
  } else {
    for (std::size_t i = 0; i != n; ++i)
      out = ::__ycxx::__detail::__fmt_put<__charT>(static_cast<_Out&&>(out), static_cast<__charT>(p[i]));
    return out;
  }
}
template <class __charT, class _Out>
constexpr _Out __fmt_put_n(_Out out, std::size_t n, const __charT* __unit, std::size_t __len) {
  if constexpr (__is_same(_Out, __ycxx::__adl_free::__fmt_iter<__charT>)) {
    __ycxx::__adl_free::__fmt_buf<__charT>& b = __fmt_access::__buffer(out);
    if (__len == 1) {
      b.fill(n, *__unit);
      return out;
    }
    if (b.__discard_) {
      b.discard(n > static_cast<std::size_t>(-1) / __len ? static_cast<std::size_t>(-1) : n * __len);
      return out;
    }
  }
  for (std::size_t i = 0; i != n; ++i)
    out = ::__ycxx::__detail::__fmt_put<__charT>(static_cast<_Out&&>(out), __unit, __len);
  return out;
}

// STATICALLY-WIDEN: the string literal for charT.
template <class __charT>
constexpr std::basic_string_view<__charT> __fmt_lit(const char* s, const wchar_t* ws) noexcept {
  if constexpr (__is_same(__charT, char))
    return s;
  else
    return ws;
}

// ---- buffers ----------------------------------------------------------------------------------

inline constexpr std::size_t __fmt_local_size = 256;

// Growing storage: a local array first, then the heap. vformat and the formatters that need the
// width of their output before writing it use this one.
template <class __charT>
class __fmt_dynbuf : public __ycxx::__adl_free::__fmt_buf<__charT> {
  __charT __local_[__fmt_local_size];
  bool __heap_ = false;

  static constexpr void __grow(__ycxx::__adl_free::__fmt_buf<__charT>& b) {
    __fmt_dynbuf& __self = static_cast<__fmt_dynbuf&>(b);
    const std::size_t __cap = __self.__cap_ * 2;
    __charT* p = std::allocator<__charT>().allocate(__cap);
    for (std::size_t i = 0; i != __self.__size_; ++i)
      p[i] = __self.__data_[i];
    if (__self.__heap_)
      std::allocator<__charT>().deallocate(__self.__data_, __self.__cap_);
    __self.__data_ = p;
    __self.__cap_ = __cap;
    __self.__heap_ = true;
  }

public:
  constexpr __fmt_dynbuf() noexcept : __ycxx::__adl_free::__fmt_buf<__charT>(__local_, __fmt_local_size, &__grow) {}
  constexpr ~__fmt_dynbuf() {
    if (__heap_)
      std::allocator<__charT>().deallocate(this->__data_, this->__cap_);
  }
  constexpr const __charT* data() const noexcept { return this->__data_; }
  constexpr std::size_t size() const noexcept { return this->__size_; }
  constexpr std::basic_string_view<__charT> view() const noexcept { return {this->__data_, this->__size_}; }
};

// Output to an output iterator, through a local array.
template <class __charT, class _Out>
class __fmt_iter_sink : public __ycxx::__adl_free::__fmt_buf<__charT> {
  __charT __local_[__fmt_local_size];
  _Out __out_;

  static constexpr void flush(__ycxx::__adl_free::__fmt_buf<__charT>& b) {
    __fmt_iter_sink& __self = static_cast<__fmt_iter_sink&>(b);
    for (std::size_t i = 0; i != __self.__size_; ++i) {
      *__self.__out_ = __self.__data_[i];
      ++__self.__out_;
    }
    __self.__size_ = 0;
  }

public:
  constexpr explicit __fmt_iter_sink(_Out out) : __ycxx::__adl_free::__fmt_buf<__charT>(__local_, __fmt_local_size, &flush), __out_(static_cast<_Out&&>(out)) {}
  constexpr _Out finish() {
    flush(*this);
    return static_cast<_Out&&>(__out_);
  }
};

// Output to [p, ...): written in place, never flushed ([format.functions]/16: the caller
// provides room for the whole result).
template <class __charT>
class __fmt_ptr_sink : public __ycxx::__adl_free::__fmt_buf<__charT> {
  static constexpr void __never(__ycxx::__adl_free::__fmt_buf<__charT>&) {}

public:
  constexpr explicit __fmt_ptr_sink(__charT* p) noexcept
      : __ycxx::__adl_free::__fmt_buf<__charT>(p, static_cast<std::size_t>(-1) / sizeof(__charT) / 2, &__never) {}
  constexpr __charT* finish() noexcept { return this->__data_ + this->__size_; }
};

// Counts the output (formatted_size), and writes its first `__limit` characters to an output
// iterator (format_to_n).
template <class __charT, class _Out>
class __fmt_count_sink : public __ycxx::__adl_free::__fmt_buf<__charT> {
  __charT __local_[__fmt_local_size];
  std::size_t __count_ = 0;
  std::size_t __limit_;
  _Out __out_;

  static constexpr void flush(__ycxx::__adl_free::__fmt_buf<__charT>& b) {
    __fmt_count_sink& __self = static_cast<__fmt_count_sink&>(b);
    if constexpr (!__is_same(_Out, decltype(nullptr))) {
      for (std::size_t i = 0; i != __self.__size_ && __self.__count_ + i < __self.__limit_; ++i) {
        *__self.__out_ = __self.__data_[i];
        ++__self.__out_;
      }
    }
    __self.__count_ += __self.__size_;
    __self.__size_ = 0;
    // Past the limit only the count matters.
    __self.__discard_ = __self.__count_ >= __self.__limit_;
  }

public:
  constexpr __fmt_count_sink(_Out out, std::size_t __limit)
      : __ycxx::__adl_free::__fmt_buf<__charT>(__local_, __fmt_local_size, &flush), __limit_(__limit), __out_(static_cast<_Out&&>(out)) {
    this->__discard_ = __limit == 0;
  }
  constexpr std::size_t finish() {
    flush(*this);
    const std::size_t d = this->__discarded_;
    return d > static_cast<std::size_t>(-1) - __count_ ? static_cast<std::size_t>(-1) : __count_ + d;
  }
  constexpr _Out& out() noexcept { return __out_; }
};

// ---- the std-format-spec ([format.string.std]) ------------------------------------------------

// fmt_spec and its enumerations are in format_decl.hpp.

template <class __charT>
constexpr bool __fmt_is_digit(__charT c) noexcept {
  return c >= __charT('0') && c <= __charT('9');
}

// A nonnegative-integer at p (p != e, *p is a digit). The grammar sets no upper bound; a value
// beyond size_t saturates (no output can be that wide, and no argument has that index).
template <class __charT>
constexpr const __charT* __fmt_parse_number(const __charT* p, const __charT* e, std::size_t& value) {
  constexpr std::size_t max = static_cast<std::size_t>(-1);
  std::size_t __v = 0;
  for (; p != e && ::__ycxx::__detail::__fmt_is_digit(*p); ++p) {
    const std::size_t d = static_cast<std::size_t>(*p - __charT('0'));
    __v = __v > (max - d) / 10 ? max : __v * 10 + d;
  }
  value = __v;
  return p;
}

// arg-id ([format.string.general]): 0 or a positive-integer.
template <class __charT>
constexpr const __charT* __fmt_parse_arg_id(const __charT* p, const __charT* e, std::size_t& id) {
  if (*p == __charT('0')) {
    id = 0;
    return p + 1;
  }
  if (!::__ycxx::__detail::__fmt_is_digit(*p))
    ::__ycxx::__detail::__throw_format_error("std::format: invalid argument index in the format string");
  return ::__ycxx::__detail::__fmt_parse_number(p, e, id);
}

// { arg-id(opt) } in a width or precision: p points after '{'. Records the argument index.
template <class __charT>
constexpr const __charT* __fmt_parse_dynamic(std::basic_format_parse_context<__charT>& __pc, const __charT* p, const __charT* e,
                                         std::size_t& id) {
  if (p == e)
    ::__ycxx::__detail::__throw_format_error("std::format: unterminated dynamic width or precision");
  if (*p == __charT('}')) {
    id = __pc.next_arg_id();
  } else {
    p = ::__ycxx::__detail::__fmt_parse_arg_id(p, e, id);
    if (p == e || *p != __charT('}'))
      ::__ycxx::__detail::__throw_format_error("std::format: invalid dynamic width or precision");
    __pc.check_arg_id(id);
  }
  __pc.check_dynamic_spec_integral(id);
  return p + 1;
}

constexpr bool __fmt_is_align(char32_t c) noexcept {
  return c == U'<' || c == U'>' || c == U'^';
}
constexpr __fmt_align __fmt_align_of(char32_t c) noexcept {
  return c == U'<' ? __fmt_align::left : c == U'>' ? __fmt_align::right : __fmt_align::__center;
}

// fill-and-align (opt) at p; `__colon_ok` is false for range-fill and tuple-fill.
template <class __charT>
constexpr const __charT* __fmt_parse_fill_align(const __charT* p, const __charT* e, __fmt_spec<__charT>& s, bool __colon_ok = true) {
  if (p == e)
    return p;
  const __uni::__decoded d = ::__ycxx::__detail::__uni::__decode(p, e);
  if (*p == __charT('}') || (!__colon_ok && *p == __charT(':')))
    return p; // '}' ends the spec; a range-fill or tuple-fill is never ':', which starts the underlying spec
  if (static_cast<std::size_t>(e - p) > d.__len && ::__ycxx::__detail::__fmt_is_align(static_cast<char32_t>(p[d.__len]))) {
    if (!d.ok || *p == __charT('{'))
      ::__ycxx::__detail::__throw_format_error("std::format: invalid fill character");
    // A decoded scalar value is at most 4 code units; the bound also tells GCC -O3 that the
    // copy stays inside fill (-Wstringop-overflow).
    const unsigned n = d.__len < 4 ? d.__len : 4;
    for (unsigned i = 0; i != n; ++i)
      s.fill[i] = p[i];
    s.__fill_len = static_cast<unsigned char>(n);
    s.align = ::__ycxx::__detail::__fmt_align_of(static_cast<char32_t>(p[d.__len]));
    return p + d.__len + 1;
  }
  if (::__ycxx::__detail::__fmt_is_align(static_cast<char32_t>(*p))) {
    s.align = ::__ycxx::__detail::__fmt_align_of(static_cast<char32_t>(*p));
    return p + 1;
  }
  return p;
}

// width (opt) at p.
template <class __charT>
constexpr const __charT* __fmt_parse_width(std::basic_format_parse_context<__charT>& __pc, const __charT* p, const __charT* e,
                                       __fmt_spec<__charT>& s) {
  if (p != e && *p >= __charT('1') && *p <= __charT('9')) {
    p = ::__ycxx::__detail::__fmt_parse_number(p, e, s.width);
    s.__width_kind = __fmt_dyn::value;
  } else if (p != e && *p == __charT('{')) {
    p = ::__ycxx::__detail::__fmt_parse_dynamic(__pc, p + 1, e, s.width);
    s.__width_kind = __fmt_dyn::arg;
  }
  return p;
}

template <class __charT>
constexpr bool __fmt_spec_end(const __charT* p, const __charT* e) noexcept {
  return p == e || *p == __charT('}');
}

// Parses a std-format-spec and checks it against the category ([format.string.std]/5-24).
template <class __charT>
constexpr const __charT* __fmt_parse_spec(std::basic_format_parse_context<__charT>& __pc, __fmt_spec<__charT>& s, __fmt_cat cat) {
  const __charT* p = __pc.begin();
  const __charT* const e = __pc.end();
  if (::__ycxx::__detail::__fmt_spec_end(p, e))
    return p;
  p = ::__ycxx::__detail::__fmt_parse_fill_align(p, e, s);
  if (p != e) {
    if (*p == __charT('+'))
      s.sign = __fmt_sign::plus, ++p;
    else if (*p == __charT('-'))
      s.sign = __fmt_sign::minus, ++p;
    else if (*p == __charT(' '))
      s.sign = __fmt_sign::space, ++p;
  }
  if (p != e && *p == __charT('#'))
    s.__alt = true, ++p;
  if (p != e && *p == __charT('0'))
    s.zero = true, ++p;
  p = ::__ycxx::__detail::__fmt_parse_width(__pc, p, e, s);
  if (p != e && *p == __charT('.')) {
    ++p;
    if (p != e && ::__ycxx::__detail::__fmt_is_digit(*p)) {
      p = ::__ycxx::__detail::__fmt_parse_number(p, e, s.precision);
      s.__prec_kind = __fmt_dyn::value;
    } else if (p != e && *p == __charT('{')) {
      p = ::__ycxx::__detail::__fmt_parse_dynamic(__pc, p + 1, e, s.precision);
      s.__prec_kind = __fmt_dyn::arg;
    } else {
      ::__ycxx::__detail::__throw_format_error("std::format: missing precision after '.'");
    }
  }
  if (p != e && *p == __charT('L'))
    s.__localized = true, ++p;
  if (p != e && *p != __charT('}')) {
    switch (static_cast<char32_t>(*p)) {
    case U'a': case U'A': case U'b': case U'B': case U'c': case U'd': case U'e': case U'E': case U'f':
    case U'F': case U'g': case U'G': case U'o': case U'p': case U'P': case U's': case U'x': case U'X':
    case U'?':
      s.type = static_cast<char>(*p);
      ++p;
      break;
    default:
      break;
    }
  }
  if (!::__ycxx::__detail::__fmt_spec_end(p, e))
    ::__ycxx::__detail::__throw_format_error("std::format: invalid format specification");

  // The type, and which options it admits.
  const char t = s.type;
  const auto __is_one_of = [t](const char* set) {
    for (; *set != 0; ++set)
      if (*set == t)
        return true;
    return false;
  };
  bool __type_ok = t == 0;
  bool __int_pres = t != 0 && __is_one_of("bBdoxX");
  switch (cat) {
  case __fmt_cat::__integer:
    __type_ok = __type_ok || __is_one_of("bBcdoxX");
    __int_pres = true; // sign, # and 0 apply to every integer presentation
    break;
  case __fmt_cat::character: __type_ok = __type_ok || __is_one_of("cbBdoxX?"); break;
  case __fmt_cat::__boolean: __type_ok = __type_ok || __is_one_of("sbBdoxX"); break;
  case __fmt_cat::__floating:
    __type_ok = __type_ok || __is_one_of("aAeEfFgG");
    __int_pres = true;
    break;
  case __fmt_cat::string: __type_ok = __type_ok || __is_one_of("s?"); break;
  case __fmt_cat::pointer: __type_ok = __type_ok || __is_one_of("pP"); break;
  }
  if (!__type_ok)
    ::__ycxx::__detail::__throw_format_error("std::format: invalid presentation type for the argument");
  if ((s.sign != __fmt_sign::none || s.__alt) && !__int_pres)
    ::__ycxx::__detail::__throw_format_error("std::format: the sign and # options need an arithmetic presentation");
  if (s.zero && !__int_pres && cat != __fmt_cat::pointer)
    ::__ycxx::__detail::__throw_format_error("std::format: the 0 option needs an arithmetic or pointer presentation");
  if (s.__prec_kind != __fmt_dyn::none && cat != __fmt_cat::__floating && cat != __fmt_cat::string)
    ::__ycxx::__detail::__throw_format_error("std::format: precision is valid only for floating-point and string types");
  if (s.__localized && (cat == __fmt_cat::string || cat == __fmt_cat::pointer))
    ::__ycxx::__detail::__throw_format_error("std::format: the L option is valid only for arithmetic types");
  return p;
}

// The value of a dynamic width or precision ([format.string.std]/10): used as is, whatever its
// size (only a negative value is an error); one beyond size_t saturates, like a number written in
// the format string. A width larger than the output can be is honoured: format then fails to
// allocate (bad_alloc), while formatted_size and format_to_n only count the padding.
template <class _Context>
constexpr std::size_t __fmt_dynamic_value(const _Context& __ctx, std::size_t id) {
  return __ctx.arg(id).visit([](auto __v) -> std::size_t {
    using _Vp = decltype(__v);
    if constexpr (__is_same(_Vp, int) || __is_same(_Vp, long long) || __is_same(_Vp, unsigned) ||
                  __is_same(_Vp, unsigned long long)) {
      if constexpr (__is_same(_Vp, int) || __is_same(_Vp, long long)) {
        if (__v < 0)
          ::__ycxx::__detail::__throw_format_error("std::format: negative dynamic width or precision");
      }
      if (static_cast<unsigned long long>(__v) > static_cast<unsigned long long>(static_cast<std::size_t>(-1)))
        return static_cast<std::size_t>(-1);
      return static_cast<std::size_t>(__v);
    } else {
      ::__ycxx::__detail::__throw_format_error("std::format: dynamic width or precision is not an integer");
    }
  });
}
template <class __charT, class _Context>
constexpr std::size_t __fmt_width(const __fmt_spec<__charT>& s, const _Context& __ctx) {
  return s.__width_kind == __fmt_dyn::arg ? ::__ycxx::__detail::__fmt_dynamic_value(__ctx, s.width) : s.width;
}
// -1: no precision; a precision beyond LLONG_MAX is LLONG_MAX.
template <class __charT, class _Context>
constexpr long long __fmt_precision(const __fmt_spec<__charT>& s, const _Context& __ctx) {
  if (s.__prec_kind == __fmt_dyn::none)
    return -1;
  const std::size_t p =
      s.__prec_kind == __fmt_dyn::arg ? ::__ycxx::__detail::__fmt_dynamic_value(__ctx, s.precision) : s.precision;
  return p > static_cast<std::size_t>(__LONG_LONG_MAX__) ? __LONG_LONG_MAX__ : static_cast<long long>(p);
}

// Writes [p, p + n), of estimated width `__est`, padded to `width` ([format.string.std]/4).
template <class __charT, class _Out>
constexpr _Out __fmt_write_padded(_Out out, const __fmt_spec<__charT>& s, __fmt_align __def, std::size_t width, std::size_t __est,
                               const __charT* p, std::size_t n) {
  if (width <= __est)
    return ::__ycxx::__detail::__fmt_put<__charT>(static_cast<_Out&&>(out), p, n);
  const std::size_t __pad = width - __est;
  const __fmt_align a = s.align == __fmt_align::none ? __def : s.align;
  const std::size_t before = a == __fmt_align::right ? __pad : a == __fmt_align::__center ? __pad / 2 : 0;
  out = ::__ycxx::__detail::__fmt_put_n<__charT>(static_cast<_Out&&>(out), before, s.fill, s.__fill_len);
  out = ::__ycxx::__detail::__fmt_put<__charT>(static_cast<_Out&&>(out), p, n);
  return ::__ycxx::__detail::__fmt_put_n<__charT>(static_cast<_Out&&>(out), __pad - before, s.fill, s.__fill_len);
}

// ---- numbers -------------------------------------------------------------------------------

// The numpunct values the L option uses ([format.string.std]/17), from the context's locale; the
// two functions are defined with <format> (ycxx/hosted/format_locale.hpp), as is
// basic_format_context::locale(), and instantiated for format_context and wformat_context in the
// hosted runtime (for the headers that include only this one).
template <class __charT>
struct __fmt_numpunct {
  std::string grouping;
  __charT thousands_sep;
  __charT decimal_point;
};
template <class __charT, class _Context>
__fmt_numpunct<__charT> __fmt_get_numpunct(_Context& __ctx);
template <class __charT, class _Context>
std::basic_string<__charT> __fmt_get_boolname(_Context& __ctx, bool value);

// The size of digit group t (0: the rightmost) of numpunct grouping g, 0 for unlimited.
constexpr std::size_t __fmt_group_size(const std::string& __g, std::size_t t) noexcept {
  if (__g.empty())
    return 0;
  const char c = t < __g.size() ? __g[t] : __g[__g.size() - 1];
  return c <= 0 || c == __SCHAR_MAX__ ? 0 : static_cast<std::size_t>(c);
}

// The parts of a formatted number: sign, prefix, the integer digits (grouped with L) and the
// rest (fraction and exponent; its '.' becomes the locale's decimal point with L).
struct __fmt_number {
  char sign = 0;
  const char* prefix = "";
  std::size_t __prefix_len = 0;
  const char* digits = nullptr;
  std::size_t __ndigits = 0;
  const char* __rest = nullptr;
  std::size_t __nrest = 0;
  bool __zero_ok = true; // false for infinities and NaNs ([format.string.std]/8)
  // '0's inserted at rest + zeros_at: the digits of a precision beyond fmt_float_prec_cap
  std::size_t __zeros = 0, __zeros_at = 0;
};

template <class __charT, class _Out>
constexpr _Out __fmt_write_number(_Out out, const __fmt_spec<__charT>& s, std::size_t width, const __fmt_number& n,
                               const __fmt_numpunct<__charT>* __np) {
  // Digit groups: group t from the right has size fmt_group_size(t); the leftmost one takes
  // what remains.
  std::size_t __groups = 1, __grouped = 0;
  if (__np != nullptr) {
    for (std::size_t t = 0;; ++t) {
      const std::size_t __sz = ::__ycxx::__detail::__fmt_group_size(__np->grouping, t);
      if (__sz == 0 || __grouped + __sz >= n.__ndigits)
        break;
      __grouped += __sz;
      ++__groups;
    }
  }
  const std::size_t __total = (n.sign != 0) + n.__prefix_len + n.__ndigits + (__groups - 1) + n.__nrest + n.__zeros;
  std::size_t __zeros = 0, before = 0, __after = 0;
  if (width > __total) {
    if (s.zero && s.align == __fmt_align::none && n.__zero_ok) {
      __zeros = width - __total;
    } else {
      const std::size_t __pad = width - __total;
      const __fmt_align a = s.align == __fmt_align::none ? __fmt_align::right : s.align;
      before = a == __fmt_align::right ? __pad : a == __fmt_align::__center ? __pad / 2 : 0;
      __after = __pad - before;
    }
  }
  out = ::__ycxx::__detail::__fmt_put_n<__charT>(static_cast<_Out&&>(out), before, s.fill, s.__fill_len);
  if (n.sign != 0)
    out = ::__ycxx::__detail::__fmt_put<__charT>(static_cast<_Out&&>(out), static_cast<__charT>(n.sign));
  out = ::__ycxx::__detail::__fmt_put_ascii<__charT>(static_cast<_Out&&>(out), n.prefix, n.__prefix_len);
  if (__zeros != 0) {
    const __charT __z = __charT('0');
    out = ::__ycxx::__detail::__fmt_put_n<__charT>(static_cast<_Out&&>(out), __zeros, &__z, 1);
  }
  if (__groups == 1) {
    out = ::__ycxx::__detail::__fmt_put_ascii<__charT>(static_cast<_Out&&>(out), n.digits, n.__ndigits);
  } else {
    const char* d = n.digits;
    const std::size_t first = n.__ndigits - __grouped;
    out = ::__ycxx::__detail::__fmt_put_ascii<__charT>(static_cast<_Out&&>(out), d, first);
    d += first;
    for (std::size_t t = __groups - 1; t-- > 0;) {
      const std::size_t __sz = ::__ycxx::__detail::__fmt_group_size(__np->grouping, t);
      out = ::__ycxx::__detail::__fmt_put<__charT>(static_cast<_Out&&>(out), __np->thousands_sep);
      out = ::__ycxx::__detail::__fmt_put_ascii<__charT>(static_cast<_Out&&>(out), d, __sz);
      d += __sz;
    }
  }
  auto __put_rest = [&](std::size_t from, std::size_t to) {
    if (__np == nullptr)
      return ::__ycxx::__detail::__fmt_put_ascii<__charT>(static_cast<_Out&&>(out), n.__rest + from, to - from);
    for (std::size_t i = from; i != to; ++i)
      out = ::__ycxx::__detail::__fmt_put<__charT>(static_cast<_Out&&>(out),
                                           n.__rest[i] == '.' ? __np->decimal_point : static_cast<__charT>(n.__rest[i]));
    return static_cast<_Out&&>(out);
  };
  const std::size_t at = n.__zeros_at < n.__nrest ? n.__zeros_at : n.__nrest;
  out = __put_rest(0, at);
  if (n.__zeros != 0) {
    const __charT __z = __charT('0');
    out = ::__ycxx::__detail::__fmt_put_n<__charT>(static_cast<_Out&&>(out), n.__zeros, &__z, 1);
  }
  out = __put_rest(at, n.__nrest);
  return ::__ycxx::__detail::__fmt_put_n<__charT>(static_cast<_Out&&>(out), __after, s.fill, s.__fill_len);
}

// An integer with an integer presentation type (Table 107), U unsigned.
template <class __charT, class _Up, class _Context>
constexpr typename _Context::iterator __fmt_write_integer(_Context& __ctx, _Up __magnitude, bool __negative,
                                                       const __fmt_spec<__charT>& s) {
  [[indeterminate]] char __buf[sizeof(_Up) * 8 + 1];
  char* const end = __buf + sizeof(__buf);
  if (s.__width_kind == __fmt_dyn::none && s.width == 0 && !s.__localized && (s.type == 0 || s.type == 'd') &&
      s.sign == __fmt_sign::none) {
    // The common "{}": the digits and a '-', nothing to pad or group.
    char* first = ::__ycxx::__detail::__charconv_write_unsigned(end, __magnitude, 10);
    if (__negative)
      *--first = '-';
    return ::__ycxx::__detail::__fmt_put_ascii<__charT>(__ctx.out(), first, static_cast<std::size_t>(end - first));
  }
  const std::size_t width = ::__ycxx::__detail::__fmt_width(s, __ctx);
  unsigned base = 10;
  __fmt_number n;
  switch (s.type) {
  case 'b': base = 2, n.prefix = "0b"; break;
  case 'B': base = 2, n.prefix = "0B"; break;
  case 'o': base = 8, n.prefix = __magnitude != 0 ? "0" : ""; break;
  case 'x': base = 16, n.prefix = "0x"; break;
  case 'X': base = 16, n.prefix = "0X"; break;
  default: break;
  }
  if (s.__alt)
    n.__prefix_len = n.prefix[0] == 0 ? 0 : n.prefix[1] == 0 ? 1 : 2;
  char* const first = ::__ycxx::__detail::__charconv_write_unsigned(end, __magnitude, base);
  if (s.type == 'X')
    for (char* __q = first; __q != end; ++__q)
      if (*__q >= 'a' && *__q <= 'f')
        *__q = static_cast<char>(*__q - 'a' + 'A');
  n.sign = __negative ? '-' : s.sign == __fmt_sign::plus ? '+' : s.sign == __fmt_sign::space ? ' ' : 0;
  n.digits = first;
  n.__ndigits = static_cast<std::size_t>(end - first);
  if (s.__localized) {
    const __fmt_numpunct<__charT> __np = ::__ycxx::__detail::__fmt_get_numpunct<__charT>(__ctx);
    return ::__ycxx::__detail::__fmt_write_number<__charT>(__ctx.out(), s, width, n, &__np);
  }
  return ::__ycxx::__detail::__fmt_write_number<__charT>(__ctx.out(), s, width, n, static_cast<const __fmt_numpunct<__charT>*>(nullptr));
}

// A character with presentation c (or an integer with type c, after the range check).
template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_write_char_value(_Context& __ctx, __charT c, const __fmt_spec<__charT>& s) {
  const std::size_t width = ::__ycxx::__detail::__fmt_width(s, __ctx);
  const std::size_t __est = width == 0 ? 0 : ::__ycxx::__detail::__uni::width(&c, 1);
  return ::__ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), s, __fmt_align::left, width, __est, &c, 1);
}

// Whether an integer is in the range of charT ([format.string.std] Table 107, type c).
template <class __charT, class _Tp>
constexpr bool __fmt_fits_char(_Tp __v) noexcept {
  using _UT = std::make_unsigned_t<_Tp>;
  using _UC = std::make_unsigned_t<__charT>;
  if constexpr (std::is_signed_v<_Tp>) {
    if (__v < 0) {
      if constexpr (!std::is_signed_v<__charT>)
        return false;
      else if constexpr (sizeof(_Tp) <= sizeof(__charT))
        return true;
      else
        return __v >= static_cast<_Tp>(std::numeric_limits<__charT>::min());
    }
  }
  const _UC m = static_cast<_UC>(std::numeric_limits<__charT>::max());
  if constexpr (sizeof(_UT) >= sizeof(_UC))
    return static_cast<_UT>(__v) <= static_cast<_UT>(m);
  else
    return static_cast<_UC>(static_cast<_UT>(__v)) <= m;
}

template <class __charT, class _Tp, class _Context>
constexpr typename _Context::iterator __fmt_format_int(_Context& __ctx, _Tp value, const __fmt_spec<__charT>& s) {
  using _Up = std::make_unsigned_t<_Tp>;
  if (s.type == 'c') {
    if (!::__ycxx::__detail::__fmt_fits_char<__charT>(value))
      ::__ycxx::__detail::__throw_format_error("std::format: the integer is not representable in the character type");
    return ::__ycxx::__detail::__fmt_write_char_value(__ctx, static_cast<__charT>(value), s);
  }
  if constexpr (std::is_signed_v<_Tp>) {
    const bool __neg = value < 0;
    const _Up __mag = __neg ? static_cast<_Up>(_Up(0) - static_cast<_Up>(value)) : static_cast<_Up>(value);
    return ::__ycxx::__detail::__fmt_write_integer(__ctx, __mag, __neg, s);
  } else {
    return ::__ycxx::__detail::__fmt_write_integer(__ctx, static_cast<_Up>(value), false, s);
  }
}

// ---- characters, strings, bool and pointers ---------------------------------------------------

// The escaped representation of [p, p + n) appended to buf.
template <class __charT>
constexpr void __fmt_escape_to(__ycxx::__adl_free::__fmt_buf<__charT>& __buf, const __charT* p, std::size_t n, bool __is_char) {
  ::__ycxx::__detail::__uni::__escape(p, n, __is_char, [&__buf](const __charT* __q, std::size_t k) { __buf.append(__q, k); });
}

// A string with type none, s or ? ([format.string.std]/15, Table 106).
template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_write_string(_Context& __ctx, const __charT* p, std::size_t n,
                                                      const __fmt_spec<__charT>& s, bool __is_char) {
  const std::size_t width = ::__ycxx::__detail::__fmt_width(s, __ctx);
  const long long __prec = ::__ycxx::__detail::__fmt_precision(s, __ctx);
  if (s.type == '?') {
    [[indeterminate]] __fmt_dynbuf<__charT> __esc;
    ::__ycxx::__detail::__fmt_escape_to(__esc, p, n, __is_char);
    const __uni::__width_result r = ::__ycxx::__detail::__uni::__width_prefix(
        __esc.data(), __esc.size(), __prec < 0 ? static_cast<std::size_t>(-1) : static_cast<std::size_t>(__prec));
    return ::__ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), s, __fmt_align::left, width, r.width, __esc.data(), r.__units);
  }
  if (__prec < 0 && width == 0)
    return ::__ycxx::__detail::__fmt_put<__charT>(__ctx.out(), p, n);
  const __uni::__width_result r =
      ::__ycxx::__detail::__uni::__width_prefix(p, n, __prec < 0 ? static_cast<std::size_t>(-1) : static_cast<std::size_t>(__prec));
  return ::__ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), s, __fmt_align::left, width, r.width, p, r.__units);
}

template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_format_char(_Context& __ctx, __charT c, const __fmt_spec<__charT>& s) {
  switch (s.type) {
  case 0:
  case 'c': return ::__ycxx::__detail::__fmt_write_char_value(__ctx, c, s);
  case '?': return ::__ycxx::__detail::__fmt_write_string(__ctx, &c, 1, s, true);
  default:
    using _Up = std::make_unsigned_t<__charT>;
    return ::__ycxx::__detail::__fmt_write_integer(__ctx, static_cast<_Up>(c), false, s);
  }
}

template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_format_bool(_Context& __ctx, bool b, const __fmt_spec<__charT>& s) {
  if (s.type != 0 && s.type != 's')
    return ::__ycxx::__detail::__fmt_write_integer(__ctx, static_cast<unsigned char>(b), false, s);
  const std::size_t width = ::__ycxx::__detail::__fmt_width(s, __ctx);
  if (s.__localized) {
    const std::basic_string<__charT> name = ::__ycxx::__detail::__fmt_get_boolname<__charT>(__ctx, b);
    const std::size_t __est = width == 0 ? 0 : ::__ycxx::__detail::__uni::width(name.data(), name.size());
    return ::__ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), s, __fmt_align::left, width, __est, name.data(), name.size());
  }
  const std::basic_string_view<__charT> name =
      b ? ::__ycxx::__detail::__fmt_lit<__charT>("true", L"true") : ::__ycxx::__detail::__fmt_lit<__charT>("false", L"false");
  return ::__ycxx::__detail::__fmt_write_padded<__charT>(__ctx.out(), s, __fmt_align::left, width, name.size(), name.data(),
                                                 name.size());
}

// Pointers: to_chars(reinterpret_cast<uintptr_t>(value), 16) with a 0x prefix (Table 111). A
// null pointer is formatted without the cast, so nullptr_t is constexpr-enabled.
template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_format_pointer(_Context& __ctx, const void* p, const __fmt_spec<__charT>& s) {
  std::uintptr_t __v = 0;
  if (p != nullptr)
    __v = reinterpret_cast<std::uintptr_t>(p);
  __fmt_spec<__charT> t = s;
  t.__alt = true;
  t.type = s.type == 'P' ? 'X' : 'x';
  return ::__ycxx::__detail::__fmt_write_integer(__ctx, __v, false, t);
}

// ---- floating point ----------------------------------------------------------------------------

// The number of integer digits of the largest finite T, plus slack.
template <class _Tp>
inline constexpr std::size_t __fmt_max_int_digits =
    static_cast<std::size_t>(__ycxx::__detail::__fp_format<_Tp>.__max_exp) * 30103 / 100000 + 3;

// Heap storage for a floating-point conversion that does not fit in the local buffer.
struct __fmt_heap_chars {
  char* p = nullptr;
  std::size_t n = 0;
  __fmt_heap_chars() = default;
  __fmt_heap_chars(const __fmt_heap_chars&) = delete;
  __fmt_heap_chars& operator=(const __fmt_heap_chars&) = delete;
  ~__fmt_heap_chars() {
    if (p != nullptr)
      std::allocator<char>().deallocate(p, n);
  }
  char* get(std::size_t size) {
    p = std::allocator<char>().allocate(size);
    n = size;
    return p;
  }
};

// The exponent of a to_chars scientific result "d.ddde+XX".
constexpr int __fmt_sci_exponent(const char* first, const char* last) noexcept {
  const char* e = last;
  while (e != first && e[-1] != 'e')
    --e;
  bool __neg = *e == '-';
  int __x = 0;
  for (++e; e != last; ++e)
    __x = __x * 10 + (*e - '0');
  return __neg ? -__x : __x;
}

// The largest precision a floating-point conversion is computed with: more than the digits of
// the exact decimal (or hexadecimal) value of any finite value of the supported types (16,494
// fractional digits for the smallest binary128 subnormal), so every digit beyond it is a 0 (and
// the e/f choice of g is the same as with the full precision).
inline constexpr long long __fmt_float_prec_cap = 1 << 15;

template <class __charT, class _Tp, class _Context>
typename _Context::iterator __fmt_format_float(_Context& __ctx, _Tp value, const __fmt_spec<__charT>& s) {
  const std::size_t width = ::__ycxx::__detail::__fmt_width(s, __ctx);
  long long __prec = ::__ycxx::__detail::__fmt_precision(s, __ctx);
  std::chars_format __f = std::chars_format::general;
  bool __shortest = false;
  switch (s.type) {
  case 'a': case 'A': __f = std::chars_format::hex; break;
  case 'e': case 'E': __f = std::chars_format::scientific; __prec = __prec < 0 ? 6 : __prec; break;
  case 'f': case 'F': __f = std::chars_format::fixed; __prec = __prec < 0 ? 6 : __prec; break;
  case 'g': case 'G': __f = std::chars_format::general; __prec = __prec < 0 ? 6 : __prec; break;
  default: __shortest = __prec < 0; break;
  }
  // A precision beyond the cap: computed with the cap, the remaining zeros appended to the
  // fraction (types a, e, f and #g keep them; g and none would remove them).
  std::size_t __extra_zeros = 0;
  if (__prec > __fmt_float_prec_cap) {
    if (s.type != 0 && ((s.type != 'g' && s.type != 'G') || s.__alt))
      __extra_zeros = static_cast<std::size_t>(__prec - __fmt_float_prec_cap);
    __prec = __fmt_float_prec_cap;
  }
  const std::size_t __need =
      64 + (__prec > 0 ? static_cast<std::size_t>(__prec) : 0) + (__f == std::chars_format::fixed ? __fmt_max_int_digits<_Tp> : 0);
  [[indeterminate]] char __y_local[256];
  __fmt_heap_chars __heap;
  char* const __buf = __need <= sizeof(__y_local) ? __y_local : __heap.get(__need);
  char* const __bufend = __buf + __need - 1; // one spare character for the '.' of the alternate form
  std::to_chars_result r;
  if (__shortest) {
    r = ::__ycxx::__detail::__to_chars_shortest(__buf, __bufend, value);
  } else if ((s.type == 'g' || s.type == 'G') && s.__alt) {
    // %#g: trailing zeros are kept ([format.string.std]/7, C 7.23.6.1): P significant digits,
    // fixed notation when the exponent X of the scientific form satisfies P > X >= -4. (Type
    // none with a precision is a to_chars general conversion, not g: its zeros are removed.)
    const int p = __prec == 0 ? 1 : static_cast<int>(__prec);
    r = ::__ycxx::__detail::__to_chars_float(__buf, __bufend, value, std::chars_format::scientific, p - 1);
    const char c = *__buf == '-' ? __buf[1] : __buf[0];
    if (c >= '0' && c <= '9') {
      const int __x = ::__ycxx::__detail::__fmt_sci_exponent(__buf, r.ptr);
      if (p > __x && __x >= -4)
        r = ::__ycxx::__detail::__to_chars_float(__buf, __bufend, value, std::chars_format::fixed, p - 1 - __x);
    }
  } else if (__prec < 0) {
    r = ::__ycxx::__detail::__to_chars_float(__buf, __bufend, value, __f);
  } else {
    r = ::__ycxx::__detail::__to_chars_float(__buf, __bufend, value, __f, static_cast<int>(__prec));
  }
  char* b = __buf;
  char* e = r.ptr;
  __fmt_number n;
  if (*b == '-') {
    n.sign = '-';
    ++b;
  } else {
    n.sign = s.sign == __fmt_sign::plus ? '+' : s.sign == __fmt_sign::space ? ' ' : 0;
  }
  const bool __finite = *b >= '0' && *b <= '9';
  if (s.type == 'A' || s.type == 'E' || s.type == 'F' || s.type == 'G')
    for (char* __q = b; __q != e; ++__q)
      if (*__q >= 'a' && *__q <= 'z')
        *__q = static_cast<char>(*__q - 'a' + 'A');
  if (s.__alt && __finite) {
    char* dot = b;
    while (dot != e && *dot != '.' && *dot != 'e' && *dot != 'E' && *dot != 'p' && *dot != 'P')
      ++dot;
    if (dot == e || *dot != '.') {
      for (char* __q = e; __q != dot; --__q)
        *__q = __q[-1];
      *dot = '.';
      ++e;
    }
  }
  n.__zero_ok = __finite;
  n.digits = b;
  char* d = b;
  if (__finite)
    while (d != e && *d >= '0' && *d <= '9')
      ++d;
  else
    d = e;
  n.__ndigits = static_cast<std::size_t>(d - b);
  n.__rest = d;
  n.__nrest = static_cast<std::size_t>(e - d);
  if (__finite && __extra_zeros != 0) {
    n.__zeros = __extra_zeros;
    while (n.__zeros_at != n.__nrest && d[n.__zeros_at] != 'e' && d[n.__zeros_at] != 'E' && d[n.__zeros_at] != 'p' &&
           d[n.__zeros_at] != 'P')
      ++n.__zeros_at;
  }
  if (s.__localized && __finite) {
    const __fmt_numpunct<__charT> __np = ::__ycxx::__detail::__fmt_get_numpunct<__charT>(__ctx);
    return ::__ycxx::__detail::__fmt_write_number<__charT>(__ctx.out(), s, width, n, &__np);
  }
  return ::__ycxx::__detail::__fmt_write_number<__charT>(__ctx.out(), s, width, n, static_cast<const __fmt_numpunct<__charT>*>(nullptr));
}

// ---- the replacement-field scanner ([format.string.general]) ------------------------------------

// Calls text(p, n) for literal text and field(id) for each replacement field, with pc at the
// field's format-spec; field leaves pc at the closing '}'.
template <class __charT, class _Text, class _Field>
constexpr void __fmt_scan(std::basic_string_view<__charT> __fmt, std::basic_format_parse_context<__charT>& __pc, _Text&& __text,
                        _Field&& field) {
  const __charT* p = __fmt.data();
  const __charT* const e = p + __fmt.size();
  while (p != e) {
    const __charT* __q = p;
    while (__q != e && *__q != __charT('{') && *__q != __charT('}'))
      ++__q;
    if (__q != p)
      __text(p, static_cast<std::size_t>(__q - p));
    if (__q == e)
      return;
    if (*__q == __charT('}')) {
      if (__q + 1 == e || __q[1] != __charT('}'))
        ::__ycxx::__detail::__throw_format_error("std::format: unmatched '}' in the format string");
      __text(__q, 1);
      p = __q + 2;
      continue;
    }
    if (++__q == e)
      ::__ycxx::__detail::__throw_format_error("std::format: unmatched '{' in the format string");
    if (*__q == __charT('{')) {
      __text(__q, 1);
      p = __q + 1;
      continue;
    }
    std::size_t id;
    if (*__q == __charT('}') || *__q == __charT(':')) {
      id = __pc.next_arg_id();
    } else {
      __q = ::__ycxx::__detail::__fmt_parse_arg_id(__q, e, id);
      __pc.check_arg_id(id);
    }
    if (__q == e)
      ::__ycxx::__detail::__throw_format_error("std::format: unmatched '{' in the format string");
    if (*__q == __charT(':'))
      ++__q;
    else if (*__q != __charT('}'))
      ::__ycxx::__detail::__throw_format_error("std::format: invalid replacement field");
    __pc.advance_to(__q);
    field(id);
    __q = __pc.begin();
    if (__q == e || *__q != __charT('}'))
      ::__ycxx::__detail::__throw_format_error("std::format: unterminated replacement field");
    p = __q + 1;
  }
}

// Formats one argument of a built-in kind: parses its std-format-spec (unless empty) and
// writes it.
template <__fmt_cat _Cat, class __charT, class _Fp>
constexpr void __fmt_builtin(std::basic_format_parse_context<__charT>& __pc, __fmt_context<__charT>& __ctx, _Fp&& write) {
  __fmt_spec<__charT> s;
  if (__pc.begin() != __pc.end() && *__pc.begin() != __charT('}'))
    __pc.advance_to(::__ycxx::__detail::__fmt_parse_spec(__pc, s, _Cat));
  __ctx.advance_to(write(s));
}

// The formatting engine: vformat_to into buf.
template <class __charT>
constexpr void __fmt_vformat(__ycxx::__adl_free::__fmt_buf<__charT>& __buf, std::basic_string_view<__charT> __fmt, __fmt_args<__charT> __args,
                           const std::locale* __loc) {
  auto __pc = __fmt_access::__parse_context<__charT>(__fmt, __fmt_access::size(__args), nullptr);
  auto __ctx = __fmt_access::__context<__charT>(__buf, __args, __loc);
  ::__ycxx::__detail::__fmt_scan(__fmt, __pc, [&__buf](const __charT* p, std::size_t n) { __buf.append(p, n); },
                           [&](std::size_t id) {
    const std::basic_format_arg<__fmt_context<__charT>> arg = __args.get(id);
    const auto& __v = __fmt_access::value(arg);
    switch (__fmt_access::kind(arg)) {
    case __fmt_kind::none:
      ::__ycxx::__detail::__throw_format_error("std::format: argument index out of range");
    case __fmt_kind::__boolean:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__boolean>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_bool(__ctx, __v.b, s); });
    case __fmt_kind::character:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::character>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_char(__ctx, __v.c, s); });
    case __fmt_kind::__int_:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__integer>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_int(__ctx, __v.i, s); });
    case __fmt_kind::__uint_:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__integer>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_int(__ctx, __v.__u, s); });
    case __fmt_kind::__llong:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__integer>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_int(__ctx, __v.__ll, s); });
    case __fmt_kind::__ullong:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__integer>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_int(__ctx, __v.__ull, s); });
    case __fmt_kind::__float_:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__floating>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_float(__ctx, __v.__f, s); });
    case __fmt_kind::__double_:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__floating>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_float(__ctx, __v.d, s); });
    case __fmt_kind::__ldouble:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::__floating>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_float(__ctx, __v.__ld, s); });
    case __fmt_kind::__cstring:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::string>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) {
        return ::__ycxx::__detail::__fmt_write_string(__ctx, __v.s, std::char_traits<__charT>::length(__v.s), s);
      });
    case __fmt_kind::string:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::string>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_write_string(__ctx, __v.sv.data(), __v.sv.size(), s); });
    case __fmt_kind::pointer:
      return ::__ycxx::__detail::__fmt_builtin<__fmt_cat::pointer>(__pc, __ctx, [&](const __fmt_spec<__charT>& s) { return ::__ycxx::__detail::__fmt_format_pointer(__ctx, __v.p, s); });
    case __fmt_kind::handle:
      return __v.h.format(__pc, __ctx);
    }
  });
}

// The compile-time check of basic_format_string ([format.fmt.string]/3).
template <class _Tp, class __charT>
constexpr const __charT* __fmt_check_parse(std::basic_format_parse_context<__charT>& __pc) {
  std::formatter<_Tp, __charT> __f;
  return __f.parse(__pc);
}
template <class __charT, class... _Args>
consteval void __fmt_check(std::basic_string_view<__charT> __fmt) {
  constexpr bool formattable = (__fmt_formattable_with<std::remove_reference_t<_Args>, __fmt_context<__charT>> && ...);
  static_assert(formattable, "std::format: an argument type has no enabled formatter (std::formattable is false)");
  if constexpr (formattable) {
    constexpr __fmt_kind __kinds[] = {::__ycxx::__detail::__fmt_kind_of<std::remove_cvref_t<_Args>, __charT>()..., __fmt_kind::none};
    using __parse_fn = const __charT* (*)(std::basic_format_parse_context<__charT>&);
    constexpr __parse_fn __parsers[] = {&::__ycxx::__detail::__fmt_check_parse<std::remove_cvref_t<_Args>, __charT>..., nullptr};
    auto __pc = __fmt_access::__parse_context<__charT>(__fmt, sizeof...(_Args), __kinds);
    ::__ycxx::__detail::__fmt_scan(__fmt, __pc, [](const __charT*, std::size_t) {}, [&](std::size_t id) {
      if (id >= sizeof...(_Args))
        ::__ycxx::__detail::__format_string_argument_index_out_of_range();
      else
        __pc.advance_to(__parsers[id](__pc));
    });
  }
}

// vformat_to for any output iterator.
template <class __charT, class _Out>
constexpr _Out __fmt_vformat_to(_Out out, std::basic_string_view<__charT> __fmt, __fmt_args<__charT> __args, const std::locale* __loc) {
  if constexpr (__is_same(_Out, __ycxx::__adl_free::__fmt_iter<__charT>)) {
    ::__ycxx::__detail::__fmt_vformat(__fmt_access::__buffer(out), __fmt, __args, __loc);
    return out;
  } else if constexpr (__is_same(_Out, __charT*)) {
    __fmt_ptr_sink<__charT> __sink(out);
    ::__ycxx::__detail::__fmt_vformat(__sink, __fmt, __args, __loc);
    return __sink.finish();
  } else {
    [[indeterminate]] __fmt_iter_sink<__charT, _Out> __sink(static_cast<_Out&&>(out));
    ::__ycxx::__detail::__fmt_vformat(__sink, __fmt, __args, __loc);
    return __sink.finish();
  }
}
template <class __charT>
constexpr std::basic_string<__charT> __fmt_vformat_string(std::basic_string_view<__charT> __fmt, __fmt_args<__charT> __args,
                                                      const std::locale* __loc) {
  [[indeterminate]] __fmt_dynbuf<__charT> __buf;
  ::__ycxx::__detail::__fmt_vformat(__buf, __fmt, __args, __loc);
  return std::basic_string<__charT>(__buf.data(), __buf.size());
}
template <class __charT, class _Out>
constexpr std::format_to_n_result<_Out> __fmt_vformat_to_n(_Out out, std::iter_difference_t<_Out> n,
                                                        std::basic_string_view<__charT> __fmt, __fmt_args<__charT> __args,
                                                        const std::locale* __loc);
template <class __charT>
constexpr std::size_t __fmt_vformatted_size(std::basic_string_view<__charT> __fmt, __fmt_args<__charT> __args,
                                          const std::locale* __loc) {
  [[indeterminate]] __fmt_count_sink<__charT, decltype(nullptr)> __sink(nullptr, 0);
  ::__ycxx::__detail::__fmt_vformat(__sink, __fmt, __args, __loc);
  return __sink.finish();
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [format.fmt.string]
template <class __charT, class... _Args>
struct basic_format_string {
private:
  basic_string_view<__charT> str;

public:
  template <class _Tp>
    requires convertible_to<const _Tp&, basic_string_view<__charT>>
  consteval basic_format_string(const _Tp& s) : str(s) {
    __ycxx::__detail::__fmt_check<__charT, _Args...>(str);
  }
  constexpr basic_format_string(__ycxx::__adl_free::__dynamic_format_string<__charT> s) noexcept : str(s.__str_) {}
  constexpr basic_string_view<__charT> get() const noexcept { return str; }
};
template <class... _Args>
using format_string = basic_format_string<char, type_identity_t<_Args>...>;
template <class... _Args>
using wformat_string = basic_format_string<wchar_t, type_identity_t<_Args>...>;

constexpr __ycxx::__adl_free::__dynamic_format_string<char> dynamic_format(string_view __fmt) noexcept { return __fmt; }
constexpr __ycxx::__adl_free::__dynamic_format_string<wchar_t> dynamic_format(wstring_view __fmt) noexcept { return __fmt; }
// runtime_format: the C++26 name of dynamic_format before P3953.
constexpr __ycxx::__adl_free::__dynamic_format_string<char> runtime_format(string_view __fmt) noexcept { return __fmt; }
constexpr __ycxx::__adl_free::__dynamic_format_string<wchar_t> runtime_format(wstring_view __fmt) noexcept { return __fmt; }

// [format.functions]
constexpr string vformat(string_view __fmt, format_args __args) {
  return __ycxx::__detail::__fmt_vformat_string<char>(__fmt, __args, nullptr);
}
constexpr wstring vformat(wstring_view __fmt, wformat_args __args) {
  return __ycxx::__detail::__fmt_vformat_string<wchar_t>(__fmt, __args, nullptr);
}
template <class... _Args>
constexpr string format(format_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_string<char>(__fmt.get(), make_format_args(__args...), nullptr);
}
template <class... _Args>
constexpr wstring format(wformat_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_string<wchar_t>(__fmt.get(), make_wformat_args(__args...), nullptr);
}

template <class _Out>
  requires output_iterator<_Out, const char&>
constexpr _Out vformat_to(_Out out, string_view __fmt, format_args __args) {
  return __ycxx::__detail::__fmt_vformat_to<char>(static_cast<_Out&&>(out), __fmt, __args, nullptr);
}
template <class _Out>
  requires output_iterator<_Out, const wchar_t&>
constexpr _Out vformat_to(_Out out, wstring_view __fmt, wformat_args __args) {
  return __ycxx::__detail::__fmt_vformat_to<wchar_t>(static_cast<_Out&&>(out), __fmt, __args, nullptr);
}
template <class _Out, class... _Args>
  requires output_iterator<_Out, const char&>
constexpr _Out format_to(_Out out, format_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to<char>(static_cast<_Out&&>(out), __fmt.get(), make_format_args(__args...), nullptr);
}
template <class _Out, class... _Args>
  requires output_iterator<_Out, const wchar_t&>
constexpr _Out format_to(_Out out, wformat_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to<wchar_t>(static_cast<_Out&&>(out), __fmt.get(), make_wformat_args(__args...),
                                               nullptr);
}

template <class _Out, class... _Args>
  requires output_iterator<_Out, const char&>
constexpr format_to_n_result<_Out> format_to_n(_Out out, iter_difference_t<_Out> n, format_string<_Args...> __fmt,
                                              _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to_n<char>(static_cast<_Out&&>(out), n, __fmt.get(), make_format_args(__args...), nullptr);
}
template <class _Out, class... _Args>
  requires output_iterator<_Out, const wchar_t&>
constexpr format_to_n_result<_Out> format_to_n(_Out out, iter_difference_t<_Out> n, wformat_string<_Args...> __fmt,
                                              _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to_n<wchar_t>(static_cast<_Out&&>(out), n, __fmt.get(), make_wformat_args(__args...),
                                                 nullptr);
}
template <class... _Args>
constexpr size_t formatted_size(format_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformatted_size<char>(__fmt.get(), make_format_args(__args...), nullptr);
}
template <class... _Args>
constexpr size_t formatted_size(wformat_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformatted_size<wchar_t>(__fmt.get(), make_wformat_args(__args...), nullptr);
}

} // namespace std

template <class __charT, class _Out>
constexpr std::format_to_n_result<_Out> __ycxx::__detail::__fmt_vformat_to_n(_Out out, std::iter_difference_t<_Out> n,
                                                                      std::basic_string_view<__charT> __fmt,
                                                                      __fmt_args<__charT> __args, const std::locale* __loc) {
  const std::size_t __limit = n < 0 ? 0 : static_cast<std::size_t>(n);
  [[indeterminate]] __fmt_count_sink<__charT, _Out> __sink(static_cast<_Out&&>(out), __limit);
  ::__ycxx::__detail::__fmt_vformat(__sink, __fmt, __args, __loc);
  const std::size_t __total = __sink.finish();
  return {static_cast<_Out&&>(__sink.out()), static_cast<std::iter_difference_t<_Out>>(__total)};
}
