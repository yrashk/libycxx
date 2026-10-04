// libycxx core: the formatting library ([format]) except the formatters of ranges and tuples
// (format_ranges.hpp) and the locale-specific parts (ycxx/hosted/format_locale.hpp).
//
// Output. Every basic_format_context the library creates writes through
// ycxx::adl_free::fmt_iter<charT>, an output iterator appending to a type-erased buffer
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
#include <ycxx/core/format_unicode.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/core/utility_base.hpp>

namespace std {
class locale;

// [format.error]
class format_error : public runtime_error {
public:
  constexpr explicit format_error(const string& what_arg) : runtime_error(what_arg) {}
  constexpr explicit format_error(const char* what_arg) : runtime_error(what_arg) {}
};

// [format.formatter]: the primary template is disabled ([format.formatter.spec]/5, /7).
template <class T, class charT = char>
struct formatter {
  formatter() = delete;
  formatter(const formatter&) = delete;
  formatter& operator=(const formatter&) = delete;
};

template <class charT>
class basic_format_parse_context;
template <class Out, class charT>
class basic_format_context;
template <class Context>
class basic_format_arg;
template <class Context>
class basic_format_args;
template <class charT, class... Args>
struct basic_format_string;

template <class Out>
struct format_to_n_result {
  Out out;
  iter_difference_t<Out> size;
};

// [format.formatter.locking]
template <class T>
inline constexpr bool enable_nonlocking_formatter_optimization = false;
} // namespace std

namespace ycxx::detail {

template <class charT>
concept fmt_char = __is_same(charT, char) || __is_same(charT, wchar_t);

// Not constexpr: a call during the compile-time check of a format string makes the string
// ill-formed; the name (or the argument, which the diagnostic shows) says why.
inline void format_error_in_constant_evaluation(const char*) noexcept {}
inline void format_string_argument_index_out_of_range() noexcept {}
inline void format_string_dynamic_argument_has_wrong_type() noexcept {}

[[noreturn]] [[gnu::cold]] constexpr void throw_format_error(const char* what) {
  if consteval {
    // Clang cannot throw during constant evaluation; this shows the message instead.
    if constexpr (cfg::clang)
      ::ycxx::detail::format_error_in_constant_evaluation(what);
  }
  ::ycxx::detail::raise_with(ycxx_error_format_error, what, [what] { return std::format_error(what); });
}

// The alternatives of basic_format_arg ([format.arg]/1), in the order of the exposition-only
// variant.
enum class fmt_kind : unsigned char {
  none,
  boolean,
  character,
  int_,
  uint_,
  llong,
  ullong,
  float_,
  double_,
  ldouble,
  cstring,
  string,
  pointer,
  handle
};

template <class T>
inline constexpr bool fmt_is_string_view = false;
template <class C, class Tr>
inline constexpr bool fmt_is_string_view<std::basic_string_view<C, Tr>> = true;
template <class T>
inline constexpr bool fmt_is_string = false;
template <class C, class Tr, class A>
inline constexpr bool fmt_is_string<std::basic_string<C, Tr, A>> = true;

// The alternative a (cv-unqualified) TD takes in basic_format_arg<Context> with char-type charT
// ([format.arg]/6).
template <class TD, class charT>
consteval fmt_kind fmt_kind_of() {
  using D = std::decay_t<TD>;
  if constexpr (__is_same(TD, bool))
    return fmt_kind::boolean;
  else if constexpr (__is_same(TD, charT) || (__is_same(TD, char) && __is_same(charT, wchar_t)))
    return fmt_kind::character;
  else if constexpr (::ycxx::detail::is_standard_signed_integer<TD> && sizeof(TD) <= sizeof(int))
    return fmt_kind::int_;
  else if constexpr (::ycxx::detail::is_standard_unsigned_integer<TD> && sizeof(TD) <= sizeof(unsigned))
    return fmt_kind::uint_;
  else if constexpr (::ycxx::detail::is_standard_signed_integer<TD> && sizeof(TD) <= sizeof(long long))
    return fmt_kind::llong;
  else if constexpr (::ycxx::detail::is_standard_unsigned_integer<TD> && sizeof(TD) <= sizeof(unsigned long long))
    return fmt_kind::ullong;
  else if constexpr (__is_same(TD, float))
    return fmt_kind::float_;
  else if constexpr (__is_same(TD, double))
    return fmt_kind::double_;
  else if constexpr (__is_same(TD, long double))
    return fmt_kind::ldouble;
  else if constexpr ((fmt_is_string_view<TD> || fmt_is_string<TD>) && requires {
                       requires __is_same(typename TD::value_type, charT);
                     })
    return fmt_kind::string;
  else if constexpr (__is_same(D, charT*) || __is_same(D, const charT*))
    return fmt_kind::cstring;
  else if constexpr (std::is_void_v<std::remove_pointer_t<TD>> || __is_same(TD, decltype(nullptr)))
    return fmt_kind::pointer;
  else
    return fmt_kind::handle;
}

struct fmt_access;

template <std::size_t N>
consteval bool fmt_unique(const fmt_kind (&k)[N]) {
  for (std::size_t i = 0; i != N; ++i)
    for (std::size_t j = i + 1; j != N; ++j)
      if (k[i] == k[j])
        return false;
  return true;
}

} // namespace ycxx::detail

namespace ycxx::adl_free {

// The type-erased output buffer behind fmt_iter: [data_, data_ + size_) holds pending output;
// make_room_ is called when size_ == cap_ and leaves size_ < cap_ (by flushing the contents to
// the destination, or by growing the storage). A destination that no longer needs the characters
// (a counter past its limit) sets discard_; fill then only counts them in discarded_, so a huge
// width or precision costs no time there.
template <class charT>
class fmt_buf {
public:
  using make_room_fn = void (*)(fmt_buf&);

  constexpr fmt_buf(charT* data, std::size_t cap, make_room_fn f) noexcept : data_(data), cap_(cap), make_room_(f) {}
  fmt_buf(const fmt_buf&) = delete;
  fmt_buf& operator=(const fmt_buf&) = delete;

  constexpr void push_back(charT c) {
    if (size_ == cap_)
      make_room_(*this);
    data_[size_++] = c;
  }
  constexpr void append(const charT* p, std::size_t n) {
    while (n != 0) {
      if (size_ == cap_)
        make_room_(*this);
      std::size_t k = cap_ - size_;
      if (k > n)
        k = n;
      for (std::size_t i = 0; i != k; ++i)
        data_[size_ + i] = p[i];
      size_ += k;
      p += k;
      n -= k;
    }
  }
  constexpr void fill(std::size_t n, charT c) {
    if (discard_)
      return discard(n);
    while (n != 0) {
      if (size_ == cap_)
        make_room_(*this);
      std::size_t k = cap_ - size_;
      if (k > n)
        k = n;
      for (std::size_t i = 0; i != k; ++i)
        data_[size_ + i] = c;
      size_ += k;
      n -= k;
    }
  }

  // Counts n characters that are not stored (saturating).
  constexpr void discard(std::size_t n) noexcept {
    discarded_ = n > static_cast<std::size_t>(-1) - discarded_ ? static_cast<std::size_t>(-1) : discarded_ + n;
  }

  charT* data_;
  std::size_t size_ = 0;
  std::size_t cap_;
  make_room_fn make_room_;
  bool discard_ = false;
  std::size_t discarded_ = 0;
};

// format_context::iterator ([format.context]/4): appends to a fmt_buf.
template <class charT>
class fmt_iter {
  fmt_buf<charT>* buf_ = nullptr;
  friend ycxx::detail::fmt_access;

public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;

  constexpr fmt_iter() noexcept = default;
  constexpr explicit fmt_iter(fmt_buf<charT>& b) noexcept : buf_(__builtin_addressof(b)) {}
  constexpr fmt_iter& operator=(const charT& c) {
    buf_->push_back(c);
    return *this;
  }
  constexpr fmt_iter& operator*() noexcept { return *this; }
  constexpr fmt_iter& operator++() noexcept { return *this; }
  constexpr fmt_iter operator++(int) noexcept { return *this; }
};

// dynamic-format-string ([format.syn]): the result of dynamic_format / runtime_format.
template <class charT>
struct dynamic_format_string {
private:
  std::basic_string_view<charT> str_;
  template <class C, class... Args>
  friend struct std::basic_format_string;

public:
  constexpr dynamic_format_string(std::basic_string_view<charT> s) noexcept : str_(s) {}
  dynamic_format_string(const dynamic_format_string&) = delete;
  dynamic_format_string& operator=(const dynamic_format_string&) = delete;
};

// format-arg-store ([format.arg.store]).
template <class Context, class... Args>
class fmt_arg_store {
  friend std::basic_format_args<Context>;
  std::basic_format_arg<Context> args_[sizeof...(Args) == 0 ? 1 : sizeof...(Args)];

public:
  constexpr explicit fmt_arg_store(Args&... a) noexcept;
};

// A disabled formatter specialization ([format.formatter.spec]/7).
struct fmt_disabled {
  fmt_disabled() = delete;
  fmt_disabled(const fmt_disabled&) = delete;
  fmt_disabled& operator=(const fmt_disabled&) = delete;
};

} // namespace ycxx::adl_free

namespace std {

// [format.parse.ctx]
template <class charT>
class basic_format_parse_context {
public:
  using char_type = charT;
  using const_iterator = typename basic_string_view<charT>::const_iterator;
  using iterator = const_iterator;

private:
  enum indexing : unsigned char { unknown, manual, automatic };
  iterator begin_;
  iterator end_;
  indexing indexing_ = unknown;
  size_t next_arg_id_ = 0;
  size_t num_args_ = 0;
  // While a format string is checked at compile time: the kinds of the arguments.
  const ycxx::detail::fmt_kind* kinds_ = nullptr;
  friend ycxx::detail::fmt_access;

  constexpr basic_format_parse_context(basic_string_view<charT> fmt, size_t num_args,
                                       const ycxx::detail::fmt_kind* kinds) noexcept
      : begin_(fmt.begin()), end_(fmt.end()), num_args_(num_args), kinds_(kinds) {}

  template <class T>
  static consteval ycxx::detail::fmt_kind kind_of() {
    static_assert(ycxx::detail::is_any_of<T, bool, charT, int, unsigned, long long, unsigned long long, float, double,
                                          long double, const charT*, basic_string_view<charT>, const void*>,
                  "std::basic_format_parse_context::check_dynamic_spec: Ts must be bool, char_type, int, unsigned "
                  "int, long long int, unsigned long long int, float, double, long double, const char_type*, "
                  "basic_string_view<char_type> or const void*");
    return ycxx::detail::fmt_kind_of<T, charT>();
  }

public:
  constexpr explicit basic_format_parse_context(basic_string_view<charT> fmt) noexcept
      : begin_(fmt.begin()), end_(fmt.end()) {}
  basic_format_parse_context(const basic_format_parse_context&) = delete;
  basic_format_parse_context& operator=(const basic_format_parse_context&) = delete;

  constexpr const_iterator begin() const noexcept { return begin_; }
  constexpr const_iterator end() const noexcept { return end_; }
  constexpr void advance_to(const_iterator it) { begin_ = it; }

  constexpr size_t next_arg_id() {
    if (indexing_ == manual)
      ycxx::detail::throw_format_error("std::format: automatic and manual argument indexing are mixed");
    indexing_ = automatic;
    if consteval {
      if (next_arg_id_ >= num_args_)
        ycxx::detail::format_string_argument_index_out_of_range();
    }
    return next_arg_id_++;
  }
  constexpr void check_arg_id(size_t id) {
    if (indexing_ == automatic)
      ycxx::detail::throw_format_error("std::format: automatic and manual argument indexing are mixed");
    indexing_ = manual;
    if consteval {
      if (id >= num_args_)
        ycxx::detail::format_string_argument_index_out_of_range();
    }
  }
  template <class... Ts>
  constexpr void check_dynamic_spec(size_t id) noexcept {
    static_assert(sizeof...(Ts) >= 1, "std::basic_format_parse_context::check_dynamic_spec: Ts must not be empty");
    constexpr ycxx::detail::fmt_kind kinds[] = {kind_of<Ts>()...};
    static_assert(ycxx::detail::fmt_unique(kinds), "std::basic_format_parse_context::check_dynamic_spec: the types in Ts must be unique");
    if consteval {
      if (id >= num_args_)
        ycxx::detail::format_string_argument_index_out_of_range();
      else if (kinds_ != nullptr) {
        bool found = false;
        for (ycxx::detail::fmt_kind k : kinds)
          found = found || k == kinds_[id];
        if (!found)
          ycxx::detail::format_string_dynamic_argument_has_wrong_type();
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

} // namespace std

namespace ycxx::detail {
template <class T, class Context, class Formatter = typename Context::template formatter_type<std::remove_const_t<T>>>
concept fmt_formattable_with =
    std::semiregular<Formatter> &&
    requires(Formatter& f, const Formatter& cf, T&& t, Context fc,
             std::basic_format_parse_context<typename Context::char_type> pc) {
      { f.parse(pc) } -> std::same_as<typename decltype(pc)::iterator>;
      { cf.format(t, fc) } -> std::same_as<typename Context::iterator>;
    };
} // namespace ycxx::detail

namespace std {

// [format.arg]
template <class Context>
class basic_format_arg {
  using char_type = typename Context::char_type;

public:
  class handle {
    const void* ptr_;
    void (*format_)(basic_format_parse_context<char_type>&, Context&, const void*);
    friend class basic_format_arg;

    template <class T>
      requires(!__is_same(remove_cv_t<T>, handle))
    constexpr explicit handle(T& val) noexcept : ptr_(__builtin_addressof(val)) {
      using TD = remove_const_t<T>;
      using TQ = conditional_t<ycxx::detail::fmt_formattable_with<const TD, Context>, const TD, TD>;
      static_assert(ycxx::detail::fmt_formattable_with<TQ, Context>,
                    "std::basic_format_arg::handle: the argument type is not formattable");
      format_ = [](basic_format_parse_context<char_type>& parse_ctx, Context& format_ctx, const void* ptr) {
        typename Context::template formatter_type<TD> f;
        parse_ctx.advance_to(f.parse(parse_ctx));
        format_ctx.advance_to(f.format(*const_cast<TQ*>(static_cast<const TD*>(ptr)), format_ctx));
      };
    }

  public:
    constexpr void format(basic_format_parse_context<char_type>& parse_ctx, Context& format_ctx) const {
      format_(parse_ctx, format_ctx, ptr_);
    }
  };

private:
  friend ycxx::detail::fmt_access;
  template <class C, class... Args>
  friend class ycxx::adl_free::fmt_arg_store;

  union value {
    monostate none;
    bool b;
    char_type c;
    int i;
    unsigned u;
    long long ll;
    unsigned long long ull;
    float f;
    double d;
    long double ld;
    const char_type* s;
    basic_string_view<char_type> sv;
    const void* p;
    handle h;
    constexpr value() noexcept : none() {}
    constexpr value(bool v) noexcept : b(v) {}
    constexpr value(char_type v) noexcept : c(v) {}
    constexpr value(int v) noexcept : i(v) {}
    constexpr value(unsigned v) noexcept : u(v) {}
    constexpr value(long long v) noexcept : ll(v) {}
    constexpr value(unsigned long long v) noexcept : ull(v) {}
    constexpr value(float v) noexcept : f(v) {}
    constexpr value(double v) noexcept : d(v) {}
    constexpr value(long double v) noexcept : ld(v) {}
    constexpr value(const char_type* v) noexcept : s(v) {}
    constexpr value(basic_string_view<char_type> v) noexcept : sv(v) {}
    constexpr value(const void* v) noexcept : p(v) {}
    constexpr value(handle v) noexcept : h(v) {}
  };

  ycxx::detail::fmt_kind kind_ = ycxx::detail::fmt_kind::none;
  value v_;

  // [format.arg]/4-6.
  template <class T>
  constexpr explicit basic_format_arg(T& v) noexcept;

public:
  constexpr basic_format_arg() noexcept {}
  constexpr explicit operator bool() const noexcept { return kind_ != ycxx::detail::fmt_kind::none; }

  template <class Visitor>
  constexpr decltype(auto) visit(this basic_format_arg arg, Visitor&& vis) {
    using K = ycxx::detail::fmt_kind;
    switch (arg.kind_) {
    case K::boolean: return static_cast<Visitor&&>(vis)(arg.v_.b);
    case K::character: return static_cast<Visitor&&>(vis)(arg.v_.c);
    case K::int_: return static_cast<Visitor&&>(vis)(arg.v_.i);
    case K::uint_: return static_cast<Visitor&&>(vis)(arg.v_.u);
    case K::llong: return static_cast<Visitor&&>(vis)(arg.v_.ll);
    case K::ullong: return static_cast<Visitor&&>(vis)(arg.v_.ull);
    case K::float_: return static_cast<Visitor&&>(vis)(arg.v_.f);
    case K::double_: return static_cast<Visitor&&>(vis)(arg.v_.d);
    case K::ldouble: return static_cast<Visitor&&>(vis)(arg.v_.ld);
    case K::cstring: return static_cast<Visitor&&>(vis)(arg.v_.s);
    case K::string: return static_cast<Visitor&&>(vis)(arg.v_.sv);
    case K::pointer: return static_cast<Visitor&&>(vis)(arg.v_.p);
    case K::handle: return static_cast<Visitor&&>(vis)(arg.v_.h);
    case K::none: break;
    }
    return static_cast<Visitor&&>(vis)(arg.v_.none);
  }
  template <class R, class Visitor>
  constexpr R visit(this basic_format_arg arg, Visitor&& vis) {
    return arg.visit([&vis](auto& x) -> R {
      if constexpr (is_void_v<R>)
        static_cast<void>(static_cast<Visitor&&>(vis)(x));
      else
        return static_cast<Visitor&&>(vis)(x);
    });
  }
};

// [format.args]
template <class Context>
class basic_format_args {
  size_t size_;
  const basic_format_arg<Context>* data_;
  friend ycxx::detail::fmt_access;

public:
  template <class... Args>
  constexpr basic_format_args(const ycxx::adl_free::fmt_arg_store<Context, Args...>& store) noexcept
      : size_(sizeof...(Args)), data_(store.args_) {}
  constexpr basic_format_arg<Context> get(size_t i) const noexcept {
    return i < size_ ? data_[i] : basic_format_arg<Context>();
  }
};
template <class Context, class... Args>
basic_format_args(ycxx::adl_free::fmt_arg_store<Context, Args...>) -> basic_format_args<Context>;

// [format.context]
template <class Out, class charT>
class basic_format_context {
  basic_format_args<basic_format_context> args_;
  Out out_;
  const std::locale* loc_; // null: std::locale()
  friend ycxx::detail::fmt_access;

  constexpr basic_format_context(Out out, basic_format_args<basic_format_context> args, const std::locale* loc)
      : args_(args), out_(static_cast<Out&&>(out)), loc_(loc) {}

public:
  using iterator = Out;
  using char_type = charT;
  template <class T>
  using formatter_type = formatter<T, charT>;

  basic_format_context(const basic_format_context&) = delete;
  basic_format_context& operator=(const basic_format_context&) = delete;

  constexpr basic_format_arg<basic_format_context> arg(size_t id) const noexcept { return args_.get(id); }
  std::locale locale(); // defined in ycxx/hosted/format_locale.hpp
  constexpr iterator out() { return static_cast<Out&&>(out_); }
  constexpr void advance_to(iterator it) { out_ = static_cast<Out&&>(it); }
};

using format_context = basic_format_context<ycxx::adl_free::fmt_iter<char>, char>;
using wformat_context = basic_format_context<ycxx::adl_free::fmt_iter<wchar_t>, wchar_t>;
using format_args = basic_format_args<format_context>;
using wformat_args = basic_format_args<wformat_context>;

// [format.formattable]
} // namespace std

namespace ycxx::detail {
template <class charT>
using fmt_context = std::basic_format_context<ycxx::adl_free::fmt_iter<charT>, charT>;
template <class charT>
using fmt_args = std::basic_format_args<fmt_context<charT>>;
} // namespace ycxx::detail

namespace std {
template <class T, class charT>
concept formattable =
    ycxx::detail::fmt_formattable_with<remove_reference_t<T>, basic_format_context<ycxx::adl_free::fmt_iter<charT>, charT>>;

template <class Context>
template <class T>
constexpr basic_format_arg<Context>::basic_format_arg(T& v) noexcept {
  static_assert(ycxx::detail::fmt_formattable_with<T, Context>,
                "std::make_format_args: an argument type has no enabled formatter");
  using TD = remove_const_t<T>;
  using K = ycxx::detail::fmt_kind;
  constexpr K k = ycxx::detail::fmt_kind_of<TD, char_type>();
  kind_ = k;
  if constexpr (k == K::boolean)
    v_ = value(static_cast<bool>(v));
  else if constexpr (k == K::character) {
    if constexpr (__is_same(TD, char) && __is_same(char_type, wchar_t))
      v_ = value(static_cast<wchar_t>(static_cast<unsigned char>(v)));
    else
      v_ = value(static_cast<char_type>(v));
  } else if constexpr (k == K::int_)
    v_ = value(static_cast<int>(v));
  else if constexpr (k == K::uint_)
    v_ = value(static_cast<unsigned>(v));
  else if constexpr (k == K::llong)
    v_ = value(static_cast<long long>(v));
  else if constexpr (k == K::ullong)
    v_ = value(static_cast<unsigned long long>(v));
  else if constexpr (k == K::float_ || k == K::double_ || k == K::ldouble)
    v_ = value(v);
  else if constexpr (k == K::string)
    v_ = value(basic_string_view<char_type>(v.data(), v.size()));
  else if constexpr (k == K::cstring)
    v_ = value(static_cast<const char_type*>(v));
  else if constexpr (k == K::pointer)
    v_ = value(static_cast<const void*>(v));
  else
    v_ = value(handle(v));
}

// [format.arg.store]
template <class Context = format_context, class... Args>
constexpr ycxx::adl_free::fmt_arg_store<Context, Args...> make_format_args(Args&... fmt_args) {
  return ycxx::adl_free::fmt_arg_store<Context, Args...>(fmt_args...);
}
template <class... Args>
constexpr ycxx::adl_free::fmt_arg_store<wformat_context, Args...> make_wformat_args(Args&... args) {
  return ycxx::adl_free::fmt_arg_store<wformat_context, Args...>(args...);
}
} // namespace std

template <class Context, class... Args>
constexpr ycxx::adl_free::fmt_arg_store<Context, Args...>::fmt_arg_store(Args&... a) noexcept
    : args_{std::basic_format_arg<Context>(a)...} {}

namespace ycxx::detail {

// Access to the private members of the formatting classes.
struct fmt_access {
  template <class charT>
  static constexpr std::basic_format_parse_context<charT> parse_context(std::basic_string_view<charT> fmt,
                                                                        std::size_t num_args, const fmt_kind* kinds) {
    return std::basic_format_parse_context<charT>(fmt, num_args, kinds);
  }
  template <class charT>
  static constexpr fmt_context<charT> context(ycxx::adl_free::fmt_buf<charT>& buf, fmt_args<charT> args,
                                              const std::locale* loc) {
    return fmt_context<charT>(ycxx::adl_free::fmt_iter<charT>(buf), args, loc);
  }
  // A context like ctx (same arguments and locale) writing to buf.
  template <class charT>
  static constexpr fmt_context<charT> context_like(const fmt_context<charT>& ctx, ycxx::adl_free::fmt_buf<charT>& buf) {
    return fmt_context<charT>(ycxx::adl_free::fmt_iter<charT>(buf), ctx.args_, ctx.loc_);
  }
  template <class Out, class charT>
  static constexpr const std::locale* locale_ptr(const std::basic_format_context<Out, charT>& ctx) {
    return ctx.loc_;
  }
  template <class charT>
  static constexpr ycxx::adl_free::fmt_buf<charT>& buffer(ycxx::adl_free::fmt_iter<charT> it) {
    return *it.buf_;
  }
  template <class Context>
  static constexpr std::size_t size(const std::basic_format_args<Context>& a) {
    return a.size_;
  }
  template <class Context>
  static constexpr fmt_kind kind(const std::basic_format_arg<Context>& a) {
    return a.kind_;
  }
  template <class Context>
  static constexpr const auto& value(const std::basic_format_arg<Context>& a) {
    return a.v_;
  }
};

// ---- output helpers ---------------------------------------------------------------------------

template <class charT, class Out>
constexpr Out fmt_put(Out out, const charT* p, std::size_t n) {
  if constexpr (__is_same(Out, ycxx::adl_free::fmt_iter<charT>)) {
    fmt_access::buffer(out).append(p, n);
  } else {
    for (std::size_t i = 0; i != n; ++i) {
      *out = p[i];
      ++out;
    }
  }
  return out;
}
template <class charT, class Out>
constexpr Out fmt_put(Out out, charT c) {
  if constexpr (__is_same(Out, ycxx::adl_free::fmt_iter<charT>)) {
    fmt_access::buffer(out).push_back(c);
  } else {
    *out = c;
    ++out;
  }
  return out;
}
// [p, p + n) of ASCII characters, widened to charT.
template <class charT, class Out>
constexpr Out fmt_put_ascii(Out out, const char* p, std::size_t n) {
  if constexpr (__is_same(charT, char)) {
    return ::ycxx::detail::fmt_put<char>(static_cast<Out&&>(out), p, n);
  } else {
    for (std::size_t i = 0; i != n; ++i)
      out = ::ycxx::detail::fmt_put<charT>(static_cast<Out&&>(out), static_cast<charT>(p[i]));
    return out;
  }
}
template <class charT, class Out>
constexpr Out fmt_put_n(Out out, std::size_t n, const charT* unit, std::size_t len) {
  if constexpr (__is_same(Out, ycxx::adl_free::fmt_iter<charT>)) {
    ycxx::adl_free::fmt_buf<charT>& b = fmt_access::buffer(out);
    if (len == 1) {
      b.fill(n, *unit);
      return out;
    }
    if (b.discard_) {
      b.discard(n > static_cast<std::size_t>(-1) / len ? static_cast<std::size_t>(-1) : n * len);
      return out;
    }
  }
  for (std::size_t i = 0; i != n; ++i)
    out = ::ycxx::detail::fmt_put<charT>(static_cast<Out&&>(out), unit, len);
  return out;
}

// STATICALLY-WIDEN: the string literal for charT.
template <class charT>
constexpr std::basic_string_view<charT> fmt_lit(const char* s, const wchar_t* ws) noexcept {
  if constexpr (__is_same(charT, char))
    return s;
  else
    return ws;
}

// ---- buffers ----------------------------------------------------------------------------------

inline constexpr std::size_t fmt_local_size = 256;

// Growing storage: a local array first, then the heap. vformat and the formatters that need the
// width of their output before writing it use this one.
template <class charT>
class fmt_dynbuf : public ycxx::adl_free::fmt_buf<charT> {
  charT local_[fmt_local_size];
  bool heap_ = false;

  static constexpr void grow(ycxx::adl_free::fmt_buf<charT>& b) {
    fmt_dynbuf& self = static_cast<fmt_dynbuf&>(b);
    const std::size_t cap = self.cap_ * 2;
    charT* p = std::allocator<charT>().allocate(cap);
    for (std::size_t i = 0; i != self.size_; ++i)
      p[i] = self.data_[i];
    if (self.heap_)
      std::allocator<charT>().deallocate(self.data_, self.cap_);
    self.data_ = p;
    self.cap_ = cap;
    self.heap_ = true;
  }

public:
  constexpr fmt_dynbuf() noexcept : ycxx::adl_free::fmt_buf<charT>(local_, fmt_local_size, &grow) {}
  constexpr ~fmt_dynbuf() {
    if (heap_)
      std::allocator<charT>().deallocate(this->data_, this->cap_);
  }
  constexpr const charT* data() const noexcept { return this->data_; }
  constexpr std::size_t size() const noexcept { return this->size_; }
  constexpr std::basic_string_view<charT> view() const noexcept { return {this->data_, this->size_}; }
};

// Output to an output iterator, through a local array.
template <class charT, class Out>
class fmt_iter_sink : public ycxx::adl_free::fmt_buf<charT> {
  charT local_[fmt_local_size];
  Out out_;

  static constexpr void flush(ycxx::adl_free::fmt_buf<charT>& b) {
    fmt_iter_sink& self = static_cast<fmt_iter_sink&>(b);
    for (std::size_t i = 0; i != self.size_; ++i) {
      *self.out_ = self.data_[i];
      ++self.out_;
    }
    self.size_ = 0;
  }

public:
  constexpr explicit fmt_iter_sink(Out out) : ycxx::adl_free::fmt_buf<charT>(local_, fmt_local_size, &flush), out_(static_cast<Out&&>(out)) {}
  constexpr Out finish() {
    flush(*this);
    return static_cast<Out&&>(out_);
  }
};

// Output to [p, ...): written in place, never flushed ([format.functions]/16: the caller
// provides room for the whole result).
template <class charT>
class fmt_ptr_sink : public ycxx::adl_free::fmt_buf<charT> {
  static constexpr void never(ycxx::adl_free::fmt_buf<charT>&) {}

public:
  constexpr explicit fmt_ptr_sink(charT* p) noexcept
      : ycxx::adl_free::fmt_buf<charT>(p, static_cast<std::size_t>(-1) / sizeof(charT) / 2, &never) {}
  constexpr charT* finish() noexcept { return this->data_ + this->size_; }
};

// Counts the output (formatted_size), and writes its first `limit` characters to an output
// iterator (format_to_n).
template <class charT, class Out>
class fmt_count_sink : public ycxx::adl_free::fmt_buf<charT> {
  charT local_[fmt_local_size];
  std::size_t count_ = 0;
  std::size_t limit_;
  Out out_;

  static constexpr void flush(ycxx::adl_free::fmt_buf<charT>& b) {
    fmt_count_sink& self = static_cast<fmt_count_sink&>(b);
    if constexpr (!__is_same(Out, decltype(nullptr))) {
      for (std::size_t i = 0; i != self.size_ && self.count_ + i < self.limit_; ++i) {
        *self.out_ = self.data_[i];
        ++self.out_;
      }
    }
    self.count_ += self.size_;
    self.size_ = 0;
    // Past the limit only the count matters.
    self.discard_ = self.count_ >= self.limit_;
  }

public:
  constexpr fmt_count_sink(Out out, std::size_t limit)
      : ycxx::adl_free::fmt_buf<charT>(local_, fmt_local_size, &flush), limit_(limit), out_(static_cast<Out&&>(out)) {
    this->discard_ = limit == 0;
  }
  constexpr std::size_t finish() {
    flush(*this);
    const std::size_t d = this->discarded_;
    return d > static_cast<std::size_t>(-1) - count_ ? static_cast<std::size_t>(-1) : count_ + d;
  }
  constexpr Out& out() noexcept { return out_; }
};

// ---- the std-format-spec ([format.string.std]) ------------------------------------------------

enum class fmt_align : unsigned char { none, left, right, center };
enum class fmt_sign : unsigned char { none, plus, minus, space };
enum class fmt_dyn : unsigned char { none, value, arg };
// The argument categories whose formatters interpret the std-format-spec.
enum class fmt_cat : unsigned char { integer, character, boolean, floating, string, pointer };

template <class charT>
struct fmt_spec {
  charT fill[4] = {charT(' ')};
  unsigned char fill_len = 1;
  fmt_align align = fmt_align::none;
  fmt_sign sign = fmt_sign::none;
  bool alt = false;
  bool zero = false;
  bool localized = false;
  fmt_dyn width_kind = fmt_dyn::none;
  fmt_dyn prec_kind = fmt_dyn::none;
  char type = 0; // presentation type, 0 for none
  std::size_t width = 0;     // value, or argument index
  std::size_t precision = 0; // value, or argument index
};

template <class charT>
constexpr bool fmt_is_digit(charT c) noexcept {
  return c >= charT('0') && c <= charT('9');
}

// A nonnegative-integer at p (p != e, *p is a digit). The grammar sets no upper bound; a value
// beyond size_t saturates (no output can be that wide, and no argument has that index).
template <class charT>
constexpr const charT* fmt_parse_number(const charT* p, const charT* e, std::size_t& value) {
  constexpr std::size_t max = static_cast<std::size_t>(-1);
  std::size_t v = 0;
  for (; p != e && ::ycxx::detail::fmt_is_digit(*p); ++p) {
    const std::size_t d = static_cast<std::size_t>(*p - charT('0'));
    v = v > (max - d) / 10 ? max : v * 10 + d;
  }
  value = v;
  return p;
}

// arg-id ([format.string.general]): 0 or a positive-integer.
template <class charT>
constexpr const charT* fmt_parse_arg_id(const charT* p, const charT* e, std::size_t& id) {
  if (*p == charT('0')) {
    id = 0;
    return p + 1;
  }
  if (!::ycxx::detail::fmt_is_digit(*p))
    ::ycxx::detail::throw_format_error("std::format: invalid argument index in the format string");
  return ::ycxx::detail::fmt_parse_number(p, e, id);
}

// { arg-id(opt) } in a width or precision: p points after '{'. Records the argument index.
template <class charT>
constexpr const charT* fmt_parse_dynamic(std::basic_format_parse_context<charT>& pc, const charT* p, const charT* e,
                                         std::size_t& id) {
  if (p == e)
    ::ycxx::detail::throw_format_error("std::format: unterminated dynamic width or precision");
  if (*p == charT('}')) {
    id = pc.next_arg_id();
  } else {
    p = ::ycxx::detail::fmt_parse_arg_id(p, e, id);
    if (p == e || *p != charT('}'))
      ::ycxx::detail::throw_format_error("std::format: invalid dynamic width or precision");
    pc.check_arg_id(id);
  }
  pc.check_dynamic_spec_integral(id);
  return p + 1;
}

constexpr bool fmt_is_align(char32_t c) noexcept {
  return c == U'<' || c == U'>' || c == U'^';
}
constexpr fmt_align fmt_align_of(char32_t c) noexcept {
  return c == U'<' ? fmt_align::left : c == U'>' ? fmt_align::right : fmt_align::center;
}

// fill-and-align (opt) at p; `colon_ok` is false for range-fill and tuple-fill.
template <class charT>
constexpr const charT* fmt_parse_fill_align(const charT* p, const charT* e, fmt_spec<charT>& s, bool colon_ok = true) {
  if (p == e)
    return p;
  const uni::decoded d = ::ycxx::detail::uni::decode(p, e);
  if (*p == charT('}') || (!colon_ok && *p == charT(':')))
    return p; // '}' ends the spec; a range-fill or tuple-fill is never ':', which starts the underlying spec
  if (static_cast<std::size_t>(e - p) > d.len && ::ycxx::detail::fmt_is_align(static_cast<char32_t>(p[d.len]))) {
    if (!d.ok || *p == charT('{'))
      ::ycxx::detail::throw_format_error("std::format: invalid fill character");
    for (unsigned i = 0; i != d.len; ++i)
      s.fill[i] = p[i];
    s.fill_len = static_cast<unsigned char>(d.len);
    s.align = ::ycxx::detail::fmt_align_of(static_cast<char32_t>(p[d.len]));
    return p + d.len + 1;
  }
  if (::ycxx::detail::fmt_is_align(static_cast<char32_t>(*p))) {
    s.align = ::ycxx::detail::fmt_align_of(static_cast<char32_t>(*p));
    return p + 1;
  }
  return p;
}

// width (opt) at p.
template <class charT>
constexpr const charT* fmt_parse_width(std::basic_format_parse_context<charT>& pc, const charT* p, const charT* e,
                                       fmt_spec<charT>& s) {
  if (p != e && *p >= charT('1') && *p <= charT('9')) {
    p = ::ycxx::detail::fmt_parse_number(p, e, s.width);
    s.width_kind = fmt_dyn::value;
  } else if (p != e && *p == charT('{')) {
    p = ::ycxx::detail::fmt_parse_dynamic(pc, p + 1, e, s.width);
    s.width_kind = fmt_dyn::arg;
  }
  return p;
}

template <class charT>
constexpr bool fmt_spec_end(const charT* p, const charT* e) noexcept {
  return p == e || *p == charT('}');
}

// Parses a std-format-spec and checks it against the category ([format.string.std]/5-24).
template <class charT>
constexpr const charT* fmt_parse_spec(std::basic_format_parse_context<charT>& pc, fmt_spec<charT>& s, fmt_cat cat) {
  const charT* p = pc.begin();
  const charT* const e = pc.end();
  if (::ycxx::detail::fmt_spec_end(p, e))
    return p;
  p = ::ycxx::detail::fmt_parse_fill_align(p, e, s);
  if (p != e) {
    if (*p == charT('+'))
      s.sign = fmt_sign::plus, ++p;
    else if (*p == charT('-'))
      s.sign = fmt_sign::minus, ++p;
    else if (*p == charT(' '))
      s.sign = fmt_sign::space, ++p;
  }
  if (p != e && *p == charT('#'))
    s.alt = true, ++p;
  if (p != e && *p == charT('0'))
    s.zero = true, ++p;
  p = ::ycxx::detail::fmt_parse_width(pc, p, e, s);
  if (p != e && *p == charT('.')) {
    ++p;
    if (p != e && ::ycxx::detail::fmt_is_digit(*p)) {
      p = ::ycxx::detail::fmt_parse_number(p, e, s.precision);
      s.prec_kind = fmt_dyn::value;
    } else if (p != e && *p == charT('{')) {
      p = ::ycxx::detail::fmt_parse_dynamic(pc, p + 1, e, s.precision);
      s.prec_kind = fmt_dyn::arg;
    } else {
      ::ycxx::detail::throw_format_error("std::format: missing precision after '.'");
    }
  }
  if (p != e && *p == charT('L'))
    s.localized = true, ++p;
  if (p != e && *p != charT('}')) {
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
  if (!::ycxx::detail::fmt_spec_end(p, e))
    ::ycxx::detail::throw_format_error("std::format: invalid format specification");

  // The type, and which options it admits.
  const char t = s.type;
  const auto is_one_of = [t](const char* set) {
    for (; *set != 0; ++set)
      if (*set == t)
        return true;
    return false;
  };
  bool type_ok = t == 0;
  bool int_pres = t != 0 && is_one_of("bBdoxX");
  switch (cat) {
  case fmt_cat::integer:
    type_ok = type_ok || is_one_of("bBcdoxX");
    int_pres = true; // sign, # and 0 apply to every integer presentation
    break;
  case fmt_cat::character: type_ok = type_ok || is_one_of("cbBdoxX?"); break;
  case fmt_cat::boolean: type_ok = type_ok || is_one_of("sbBdoxX"); break;
  case fmt_cat::floating:
    type_ok = type_ok || is_one_of("aAeEfFgG");
    int_pres = true;
    break;
  case fmt_cat::string: type_ok = type_ok || is_one_of("s?"); break;
  case fmt_cat::pointer: type_ok = type_ok || is_one_of("pP"); break;
  }
  if (!type_ok)
    ::ycxx::detail::throw_format_error("std::format: invalid presentation type for the argument");
  if ((s.sign != fmt_sign::none || s.alt) && !int_pres)
    ::ycxx::detail::throw_format_error("std::format: the sign and # options need an arithmetic presentation");
  if (s.zero && !int_pres && cat != fmt_cat::pointer)
    ::ycxx::detail::throw_format_error("std::format: the 0 option needs an arithmetic or pointer presentation");
  if (s.prec_kind != fmt_dyn::none && cat != fmt_cat::floating && cat != fmt_cat::string)
    ::ycxx::detail::throw_format_error("std::format: precision is valid only for floating-point and string types");
  if (s.localized && (cat == fmt_cat::string || cat == fmt_cat::pointer))
    ::ycxx::detail::throw_format_error("std::format: the L option is valid only for arithmetic types");
  return p;
}

// The value of a dynamic width or precision ([format.string.std]/10): used as is, whatever its
// size (only a negative value is an error); one beyond size_t saturates, like a number written in
// the format string. A width larger than the output can be is honoured: format then fails to
// allocate (bad_alloc), while formatted_size and format_to_n only count the padding.
template <class Context>
constexpr std::size_t fmt_dynamic_value(const Context& ctx, std::size_t id) {
  return ctx.arg(id).visit([](auto v) -> std::size_t {
    using V = decltype(v);
    if constexpr (__is_same(V, int) || __is_same(V, long long) || __is_same(V, unsigned) ||
                  __is_same(V, unsigned long long)) {
      if constexpr (__is_same(V, int) || __is_same(V, long long)) {
        if (v < 0)
          ::ycxx::detail::throw_format_error("std::format: negative dynamic width or precision");
      }
      if (static_cast<unsigned long long>(v) > static_cast<unsigned long long>(static_cast<std::size_t>(-1)))
        return static_cast<std::size_t>(-1);
      return static_cast<std::size_t>(v);
    } else {
      ::ycxx::detail::throw_format_error("std::format: dynamic width or precision is not an integer");
    }
  });
}
template <class charT, class Context>
constexpr std::size_t fmt_width(const fmt_spec<charT>& s, const Context& ctx) {
  return s.width_kind == fmt_dyn::arg ? ::ycxx::detail::fmt_dynamic_value(ctx, s.width) : s.width;
}
// -1: no precision; a precision beyond LLONG_MAX is LLONG_MAX.
template <class charT, class Context>
constexpr long long fmt_precision(const fmt_spec<charT>& s, const Context& ctx) {
  if (s.prec_kind == fmt_dyn::none)
    return -1;
  const std::size_t p =
      s.prec_kind == fmt_dyn::arg ? ::ycxx::detail::fmt_dynamic_value(ctx, s.precision) : s.precision;
  return p > static_cast<std::size_t>(__LONG_LONG_MAX__) ? __LONG_LONG_MAX__ : static_cast<long long>(p);
}

// Writes [p, p + n), of estimated width `est`, padded to `width` ([format.string.std]/4).
template <class charT, class Out>
constexpr Out fmt_write_padded(Out out, const fmt_spec<charT>& s, fmt_align def, std::size_t width, std::size_t est,
                               const charT* p, std::size_t n) {
  if (width <= est)
    return ::ycxx::detail::fmt_put<charT>(static_cast<Out&&>(out), p, n);
  const std::size_t pad = width - est;
  const fmt_align a = s.align == fmt_align::none ? def : s.align;
  const std::size_t before = a == fmt_align::right ? pad : a == fmt_align::center ? pad / 2 : 0;
  out = ::ycxx::detail::fmt_put_n<charT>(static_cast<Out&&>(out), before, s.fill, s.fill_len);
  out = ::ycxx::detail::fmt_put<charT>(static_cast<Out&&>(out), p, n);
  return ::ycxx::detail::fmt_put_n<charT>(static_cast<Out&&>(out), pad - before, s.fill, s.fill_len);
}

// ---- numbers -------------------------------------------------------------------------------

// The numpunct values the L option uses ([format.string.std]/17), from the context's locale; the
// two functions are defined with <format> (ycxx/hosted/format_locale.hpp), as is
// basic_format_context::locale(), and instantiated for format_context and wformat_context in the
// hosted runtime (for the headers that include only this one).
template <class charT>
struct fmt_numpunct {
  std::string grouping;
  charT thousands_sep;
  charT decimal_point;
};
template <class charT, class Context>
fmt_numpunct<charT> fmt_get_numpunct(Context& ctx);
template <class charT, class Context>
std::basic_string<charT> fmt_get_boolname(Context& ctx, bool value);

// The size of digit group t (0: the rightmost) of numpunct grouping g, 0 for unlimited.
constexpr std::size_t fmt_group_size(const std::string& g, std::size_t t) noexcept {
  if (g.empty())
    return 0;
  const char c = t < g.size() ? g[t] : g[g.size() - 1];
  return c <= 0 || c == __SCHAR_MAX__ ? 0 : static_cast<std::size_t>(c);
}

// The parts of a formatted number: sign, prefix, the integer digits (grouped with L) and the
// rest (fraction and exponent; its '.' becomes the locale's decimal point with L).
struct fmt_number {
  char sign = 0;
  const char* prefix = "";
  std::size_t prefix_len = 0;
  const char* digits = nullptr;
  std::size_t ndigits = 0;
  const char* rest = nullptr;
  std::size_t nrest = 0;
  bool zero_ok = true; // false for infinities and NaNs ([format.string.std]/8)
  // '0's inserted at rest + zeros_at: the digits of a precision beyond fmt_float_prec_cap
  std::size_t zeros = 0, zeros_at = 0;
};

template <class charT, class Out>
constexpr Out fmt_write_number(Out out, const fmt_spec<charT>& s, std::size_t width, const fmt_number& n,
                               const fmt_numpunct<charT>* np) {
  // Digit groups: group t from the right has size fmt_group_size(t); the leftmost one takes
  // what remains.
  std::size_t groups = 1, grouped = 0;
  if (np != nullptr) {
    for (std::size_t t = 0;; ++t) {
      const std::size_t sz = ::ycxx::detail::fmt_group_size(np->grouping, t);
      if (sz == 0 || grouped + sz >= n.ndigits)
        break;
      grouped += sz;
      ++groups;
    }
  }
  const std::size_t total = (n.sign != 0) + n.prefix_len + n.ndigits + (groups - 1) + n.nrest + n.zeros;
  std::size_t zeros = 0, before = 0, after = 0;
  if (width > total) {
    if (s.zero && s.align == fmt_align::none && n.zero_ok) {
      zeros = width - total;
    } else {
      const std::size_t pad = width - total;
      const fmt_align a = s.align == fmt_align::none ? fmt_align::right : s.align;
      before = a == fmt_align::right ? pad : a == fmt_align::center ? pad / 2 : 0;
      after = pad - before;
    }
  }
  out = ::ycxx::detail::fmt_put_n<charT>(static_cast<Out&&>(out), before, s.fill, s.fill_len);
  if (n.sign != 0)
    out = ::ycxx::detail::fmt_put<charT>(static_cast<Out&&>(out), static_cast<charT>(n.sign));
  out = ::ycxx::detail::fmt_put_ascii<charT>(static_cast<Out&&>(out), n.prefix, n.prefix_len);
  if (zeros != 0) {
    const charT z = charT('0');
    out = ::ycxx::detail::fmt_put_n<charT>(static_cast<Out&&>(out), zeros, &z, 1);
  }
  if (groups == 1) {
    out = ::ycxx::detail::fmt_put_ascii<charT>(static_cast<Out&&>(out), n.digits, n.ndigits);
  } else {
    const char* d = n.digits;
    const std::size_t first = n.ndigits - grouped;
    out = ::ycxx::detail::fmt_put_ascii<charT>(static_cast<Out&&>(out), d, first);
    d += first;
    for (std::size_t t = groups - 1; t-- > 0;) {
      const std::size_t sz = ::ycxx::detail::fmt_group_size(np->grouping, t);
      out = ::ycxx::detail::fmt_put<charT>(static_cast<Out&&>(out), np->thousands_sep);
      out = ::ycxx::detail::fmt_put_ascii<charT>(static_cast<Out&&>(out), d, sz);
      d += sz;
    }
  }
  auto put_rest = [&](std::size_t from, std::size_t to) {
    if (np == nullptr)
      return ::ycxx::detail::fmt_put_ascii<charT>(static_cast<Out&&>(out), n.rest + from, to - from);
    for (std::size_t i = from; i != to; ++i)
      out = ::ycxx::detail::fmt_put<charT>(static_cast<Out&&>(out),
                                           n.rest[i] == '.' ? np->decimal_point : static_cast<charT>(n.rest[i]));
    return static_cast<Out&&>(out);
  };
  const std::size_t at = n.zeros_at < n.nrest ? n.zeros_at : n.nrest;
  out = put_rest(0, at);
  if (n.zeros != 0) {
    const charT z = charT('0');
    out = ::ycxx::detail::fmt_put_n<charT>(static_cast<Out&&>(out), n.zeros, &z, 1);
  }
  out = put_rest(at, n.nrest);
  return ::ycxx::detail::fmt_put_n<charT>(static_cast<Out&&>(out), after, s.fill, s.fill_len);
}

// An integer with an integer presentation type (Table 107), U unsigned.
template <class charT, class U, class Context>
constexpr typename Context::iterator fmt_write_integer(Context& ctx, U magnitude, bool negative,
                                                       const fmt_spec<charT>& s) {
  const std::size_t width = ::ycxx::detail::fmt_width(s, ctx);
  char buf[sizeof(U) * 8 + 1];
  char* const end = buf + sizeof(buf);
  unsigned base = 10;
  fmt_number n;
  switch (s.type) {
  case 'b': base = 2, n.prefix = "0b"; break;
  case 'B': base = 2, n.prefix = "0B"; break;
  case 'o': base = 8, n.prefix = magnitude != 0 ? "0" : ""; break;
  case 'x': base = 16, n.prefix = "0x"; break;
  case 'X': base = 16, n.prefix = "0X"; break;
  default: break;
  }
  if (s.alt)
    n.prefix_len = n.prefix[0] == 0 ? 0 : n.prefix[1] == 0 ? 1 : 2;
  char* const first = ::ycxx::detail::charconv_write_unsigned(end, magnitude, base);
  if (s.type == 'X')
    for (char* q = first; q != end; ++q)
      if (*q >= 'a' && *q <= 'f')
        *q = static_cast<char>(*q - 'a' + 'A');
  n.sign = negative ? '-' : s.sign == fmt_sign::plus ? '+' : s.sign == fmt_sign::space ? ' ' : 0;
  n.digits = first;
  n.ndigits = static_cast<std::size_t>(end - first);
  if (s.localized) {
    const fmt_numpunct<charT> np = ::ycxx::detail::fmt_get_numpunct<charT>(ctx);
    return ::ycxx::detail::fmt_write_number<charT>(ctx.out(), s, width, n, &np);
  }
  return ::ycxx::detail::fmt_write_number<charT>(ctx.out(), s, width, n, static_cast<const fmt_numpunct<charT>*>(nullptr));
}

// A character with presentation c (or an integer with type c, after the range check).
template <class charT, class Context>
constexpr typename Context::iterator fmt_write_char_value(Context& ctx, charT c, const fmt_spec<charT>& s) {
  const std::size_t width = ::ycxx::detail::fmt_width(s, ctx);
  const std::size_t est = width == 0 ? 0 : ::ycxx::detail::uni::width(&c, 1);
  return ::ycxx::detail::fmt_write_padded<charT>(ctx.out(), s, fmt_align::left, width, est, &c, 1);
}

// Whether an integer is in the range of charT ([format.string.std] Table 107, type c).
template <class charT, class T>
constexpr bool fmt_fits_char(T v) noexcept {
  using UT = std::make_unsigned_t<T>;
  using UC = std::make_unsigned_t<charT>;
  if constexpr (std::is_signed_v<T>) {
    if (v < 0) {
      if constexpr (!std::is_signed_v<charT>)
        return false;
      else if constexpr (sizeof(T) <= sizeof(charT))
        return true;
      else
        return v >= static_cast<T>(std::numeric_limits<charT>::min());
    }
  }
  const UC m = static_cast<UC>(std::numeric_limits<charT>::max());
  if constexpr (sizeof(UT) >= sizeof(UC))
    return static_cast<UT>(v) <= static_cast<UT>(m);
  else
    return static_cast<UC>(static_cast<UT>(v)) <= m;
}

template <class charT, class T, class Context>
constexpr typename Context::iterator fmt_format_int(Context& ctx, T value, const fmt_spec<charT>& s) {
  using U = std::make_unsigned_t<T>;
  if (s.type == 'c') {
    if (!::ycxx::detail::fmt_fits_char<charT>(value))
      ::ycxx::detail::throw_format_error("std::format: the integer is not representable in the character type");
    return ::ycxx::detail::fmt_write_char_value(ctx, static_cast<charT>(value), s);
  }
  if constexpr (std::is_signed_v<T>) {
    const bool neg = value < 0;
    const U mag = neg ? static_cast<U>(U(0) - static_cast<U>(value)) : static_cast<U>(value);
    return ::ycxx::detail::fmt_write_integer(ctx, mag, neg, s);
  } else {
    return ::ycxx::detail::fmt_write_integer(ctx, static_cast<U>(value), false, s);
  }
}

// ---- characters, strings, bool and pointers ---------------------------------------------------

// The escaped representation of [p, p + n) appended to buf.
template <class charT>
constexpr void fmt_escape_to(ycxx::adl_free::fmt_buf<charT>& buf, const charT* p, std::size_t n, bool is_char) {
  ::ycxx::detail::uni::escape(p, n, is_char, [&buf](const charT* q, std::size_t k) { buf.append(q, k); });
}

// A string with type none, s or ? ([format.string.std]/15, Table 106).
template <class charT, class Context>
constexpr typename Context::iterator fmt_write_string(Context& ctx, const charT* p, std::size_t n,
                                                      const fmt_spec<charT>& s, bool is_char = false) {
  const std::size_t width = ::ycxx::detail::fmt_width(s, ctx);
  const long long prec = ::ycxx::detail::fmt_precision(s, ctx);
  if (s.type == '?') {
    fmt_dynbuf<charT> esc;
    ::ycxx::detail::fmt_escape_to(esc, p, n, is_char);
    const uni::width_result r = ::ycxx::detail::uni::width_prefix(
        esc.data(), esc.size(), prec < 0 ? static_cast<std::size_t>(-1) : static_cast<std::size_t>(prec));
    return ::ycxx::detail::fmt_write_padded<charT>(ctx.out(), s, fmt_align::left, width, r.width, esc.data(), r.units);
  }
  if (prec < 0 && width == 0)
    return ::ycxx::detail::fmt_put<charT>(ctx.out(), p, n);
  const uni::width_result r =
      ::ycxx::detail::uni::width_prefix(p, n, prec < 0 ? static_cast<std::size_t>(-1) : static_cast<std::size_t>(prec));
  return ::ycxx::detail::fmt_write_padded<charT>(ctx.out(), s, fmt_align::left, width, r.width, p, r.units);
}

template <class charT, class Context>
constexpr typename Context::iterator fmt_format_char(Context& ctx, charT c, const fmt_spec<charT>& s) {
  switch (s.type) {
  case 0:
  case 'c': return ::ycxx::detail::fmt_write_char_value(ctx, c, s);
  case '?': return ::ycxx::detail::fmt_write_string(ctx, &c, 1, s, true);
  default:
    using U = std::make_unsigned_t<charT>;
    return ::ycxx::detail::fmt_write_integer(ctx, static_cast<U>(c), false, s);
  }
}

template <class charT, class Context>
constexpr typename Context::iterator fmt_format_bool(Context& ctx, bool b, const fmt_spec<charT>& s) {
  if (s.type != 0 && s.type != 's')
    return ::ycxx::detail::fmt_write_integer(ctx, static_cast<unsigned char>(b), false, s);
  const std::size_t width = ::ycxx::detail::fmt_width(s, ctx);
  if (s.localized) {
    const std::basic_string<charT> name = ::ycxx::detail::fmt_get_boolname<charT>(ctx, b);
    const std::size_t est = width == 0 ? 0 : ::ycxx::detail::uni::width(name.data(), name.size());
    return ::ycxx::detail::fmt_write_padded<charT>(ctx.out(), s, fmt_align::left, width, est, name.data(), name.size());
  }
  const std::basic_string_view<charT> name =
      b ? ::ycxx::detail::fmt_lit<charT>("true", L"true") : ::ycxx::detail::fmt_lit<charT>("false", L"false");
  return ::ycxx::detail::fmt_write_padded<charT>(ctx.out(), s, fmt_align::left, width, name.size(), name.data(),
                                                 name.size());
}

// Pointers: to_chars(reinterpret_cast<uintptr_t>(value), 16) with a 0x prefix (Table 111). A
// null pointer is formatted without the cast, so nullptr_t is constexpr-enabled.
template <class charT, class Context>
constexpr typename Context::iterator fmt_format_pointer(Context& ctx, const void* p, const fmt_spec<charT>& s) {
  std::uintptr_t v = 0;
  if (p != nullptr)
    v = reinterpret_cast<std::uintptr_t>(p);
  fmt_spec<charT> t = s;
  t.alt = true;
  t.type = s.type == 'P' ? 'X' : 'x';
  return ::ycxx::detail::fmt_write_integer(ctx, v, false, t);
}

// ---- floating point ----------------------------------------------------------------------------

template <class T>
inline constexpr bool fmt_is_float =
    __is_same(T, std::remove_cv_t<T>) && ycxx::detail::is_floating_v<T> && requires { ycxx::detail::fp_kind_of<T>(); };

// The number of integer digits of the largest finite T, plus slack.
template <class T>
inline constexpr std::size_t fmt_max_int_digits =
    static_cast<std::size_t>(ycxx::detail::fp_format<T>.max_exp) * 30103 / 100000 + 3;

// Heap storage for a floating-point conversion that does not fit in the local buffer.
struct fmt_heap_chars {
  char* p = nullptr;
  std::size_t n = 0;
  fmt_heap_chars() = default;
  fmt_heap_chars(const fmt_heap_chars&) = delete;
  fmt_heap_chars& operator=(const fmt_heap_chars&) = delete;
  ~fmt_heap_chars() {
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
constexpr int fmt_sci_exponent(const char* first, const char* last) noexcept {
  const char* e = last;
  while (e != first && e[-1] != 'e')
    --e;
  bool neg = *e == '-';
  int x = 0;
  for (++e; e != last; ++e)
    x = x * 10 + (*e - '0');
  return neg ? -x : x;
}

// The largest precision a floating-point conversion is computed with: more than the digits of
// the exact decimal (or hexadecimal) value of any finite value of the supported types (16,494
// fractional digits for the smallest binary128 subnormal), so every digit beyond it is a 0 (and
// the e/f choice of g is the same as with the full precision).
inline constexpr long long fmt_float_prec_cap = 1 << 15;

template <class charT, class T, class Context>
typename Context::iterator fmt_format_float(Context& ctx, T value, const fmt_spec<charT>& s) {
  const std::size_t width = ::ycxx::detail::fmt_width(s, ctx);
  long long prec = ::ycxx::detail::fmt_precision(s, ctx);
  std::chars_format f = std::chars_format::general;
  bool shortest = false;
  switch (s.type) {
  case 'a': case 'A': f = std::chars_format::hex; break;
  case 'e': case 'E': f = std::chars_format::scientific; prec = prec < 0 ? 6 : prec; break;
  case 'f': case 'F': f = std::chars_format::fixed; prec = prec < 0 ? 6 : prec; break;
  case 'g': case 'G': f = std::chars_format::general; prec = prec < 0 ? 6 : prec; break;
  default: shortest = prec < 0; break;
  }
  // A precision beyond the cap: computed with the cap, the remaining zeros appended to the
  // fraction (types a, e, f and #g keep them; g and none would remove them).
  std::size_t extra_zeros = 0;
  if (prec > fmt_float_prec_cap) {
    if (s.type != 0 && ((s.type != 'g' && s.type != 'G') || s.alt))
      extra_zeros = static_cast<std::size_t>(prec - fmt_float_prec_cap);
    prec = fmt_float_prec_cap;
  }
  const std::size_t need =
      64 + (prec > 0 ? static_cast<std::size_t>(prec) : 0) + (f == std::chars_format::fixed ? fmt_max_int_digits<T> : 0);
  char local[256];
  fmt_heap_chars heap;
  char* const buf = need <= sizeof(local) ? local : heap.get(need);
  char* const bufend = buf + need - 1; // one spare character for the '.' of the alternate form
  std::to_chars_result r;
  if (shortest) {
    r = ::ycxx::detail::to_chars_shortest(buf, bufend, value);
  } else if ((s.type == 'g' || s.type == 'G') && s.alt) {
    // %#g: trailing zeros are kept ([format.string.std]/7, C 7.23.6.1): P significant digits,
    // fixed notation when the exponent X of the scientific form satisfies P > X >= -4. (Type
    // none with a precision is a to_chars general conversion, not g: its zeros are removed.)
    const int p = prec == 0 ? 1 : static_cast<int>(prec);
    r = ::ycxx::detail::to_chars_float(buf, bufend, value, std::chars_format::scientific, p - 1);
    const char c = *buf == '-' ? buf[1] : buf[0];
    if (c >= '0' && c <= '9') {
      const int x = ::ycxx::detail::fmt_sci_exponent(buf, r.ptr);
      if (p > x && x >= -4)
        r = ::ycxx::detail::to_chars_float(buf, bufend, value, std::chars_format::fixed, p - 1 - x);
    }
  } else if (prec < 0) {
    r = ::ycxx::detail::to_chars_float(buf, bufend, value, f);
  } else {
    r = ::ycxx::detail::to_chars_float(buf, bufend, value, f, static_cast<int>(prec));
  }
  char* b = buf;
  char* e = r.ptr;
  fmt_number n;
  if (*b == '-') {
    n.sign = '-';
    ++b;
  } else {
    n.sign = s.sign == fmt_sign::plus ? '+' : s.sign == fmt_sign::space ? ' ' : 0;
  }
  const bool finite = *b >= '0' && *b <= '9';
  if (s.type == 'A' || s.type == 'E' || s.type == 'F' || s.type == 'G')
    for (char* q = b; q != e; ++q)
      if (*q >= 'a' && *q <= 'z')
        *q = static_cast<char>(*q - 'a' + 'A');
  if (s.alt && finite) {
    char* dot = b;
    while (dot != e && *dot != '.' && *dot != 'e' && *dot != 'E' && *dot != 'p' && *dot != 'P')
      ++dot;
    if (dot == e || *dot != '.') {
      for (char* q = e; q != dot; --q)
        *q = q[-1];
      *dot = '.';
      ++e;
    }
  }
  n.zero_ok = finite;
  n.digits = b;
  char* d = b;
  if (finite)
    while (d != e && *d >= '0' && *d <= '9')
      ++d;
  else
    d = e;
  n.ndigits = static_cast<std::size_t>(d - b);
  n.rest = d;
  n.nrest = static_cast<std::size_t>(e - d);
  if (finite && extra_zeros != 0) {
    n.zeros = extra_zeros;
    while (n.zeros_at != n.nrest && d[n.zeros_at] != 'e' && d[n.zeros_at] != 'E' && d[n.zeros_at] != 'p' &&
           d[n.zeros_at] != 'P')
      ++n.zeros_at;
  }
  if (s.localized && finite) {
    const fmt_numpunct<charT> np = ::ycxx::detail::fmt_get_numpunct<charT>(ctx);
    return ::ycxx::detail::fmt_write_number<charT>(ctx.out(), s, width, n, &np);
  }
  return ::ycxx::detail::fmt_write_number<charT>(ctx.out(), s, width, n, static_cast<const fmt_numpunct<charT>*>(nullptr));
}

// ---- the replacement-field scanner ([format.string.general]) ------------------------------------

// Calls text(p, n) for literal text and field(id) for each replacement field, with pc at the
// field's format-spec; field leaves pc at the closing '}'.
template <class charT, class Text, class Field>
constexpr void fmt_scan(std::basic_string_view<charT> fmt, std::basic_format_parse_context<charT>& pc, Text&& text,
                        Field&& field) {
  const charT* p = fmt.data();
  const charT* const e = p + fmt.size();
  while (p != e) {
    const charT* q = p;
    while (q != e && *q != charT('{') && *q != charT('}'))
      ++q;
    if (q != p)
      text(p, static_cast<std::size_t>(q - p));
    if (q == e)
      return;
    if (*q == charT('}')) {
      if (q + 1 == e || q[1] != charT('}'))
        ::ycxx::detail::throw_format_error("std::format: unmatched '}' in the format string");
      text(q, 1);
      p = q + 2;
      continue;
    }
    if (++q == e)
      ::ycxx::detail::throw_format_error("std::format: unmatched '{' in the format string");
    if (*q == charT('{')) {
      text(q, 1);
      p = q + 1;
      continue;
    }
    std::size_t id;
    if (*q == charT('}') || *q == charT(':')) {
      id = pc.next_arg_id();
    } else {
      q = ::ycxx::detail::fmt_parse_arg_id(q, e, id);
      pc.check_arg_id(id);
    }
    if (q == e)
      ::ycxx::detail::throw_format_error("std::format: unmatched '{' in the format string");
    if (*q == charT(':'))
      ++q;
    else if (*q != charT('}'))
      ::ycxx::detail::throw_format_error("std::format: invalid replacement field");
    pc.advance_to(q);
    field(id);
    q = pc.begin();
    if (q == e || *q != charT('}'))
      ::ycxx::detail::throw_format_error("std::format: unterminated replacement field");
    p = q + 1;
  }
}

// Formats one argument of a built-in kind: parses its std-format-spec (unless empty) and
// writes it.
template <fmt_cat Cat, class charT, class F>
constexpr void fmt_builtin(std::basic_format_parse_context<charT>& pc, fmt_context<charT>& ctx, F&& write) {
  fmt_spec<charT> s;
  if (pc.begin() != pc.end() && *pc.begin() != charT('}'))
    pc.advance_to(::ycxx::detail::fmt_parse_spec(pc, s, Cat));
  ctx.advance_to(write(s));
}

// The formatting engine: vformat_to into buf.
template <class charT>
constexpr void fmt_vformat(ycxx::adl_free::fmt_buf<charT>& buf, std::basic_string_view<charT> fmt, fmt_args<charT> args,
                           const std::locale* loc) {
  auto pc = fmt_access::parse_context<charT>(fmt, fmt_access::size(args), nullptr);
  auto ctx = fmt_access::context<charT>(buf, args, loc);
  ::ycxx::detail::fmt_scan(fmt, pc, [&buf](const charT* p, std::size_t n) { buf.append(p, n); },
                           [&](std::size_t id) {
    const std::basic_format_arg<fmt_context<charT>> arg = args.get(id);
    const auto& v = fmt_access::value(arg);
    switch (fmt_access::kind(arg)) {
    case fmt_kind::none:
      ::ycxx::detail::throw_format_error("std::format: argument index out of range");
    case fmt_kind::boolean:
      return ::ycxx::detail::fmt_builtin<fmt_cat::boolean>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_bool(ctx, v.b, s); });
    case fmt_kind::character:
      return ::ycxx::detail::fmt_builtin<fmt_cat::character>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_char(ctx, v.c, s); });
    case fmt_kind::int_:
      return ::ycxx::detail::fmt_builtin<fmt_cat::integer>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_int(ctx, v.i, s); });
    case fmt_kind::uint_:
      return ::ycxx::detail::fmt_builtin<fmt_cat::integer>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_int(ctx, v.u, s); });
    case fmt_kind::llong:
      return ::ycxx::detail::fmt_builtin<fmt_cat::integer>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_int(ctx, v.ll, s); });
    case fmt_kind::ullong:
      return ::ycxx::detail::fmt_builtin<fmt_cat::integer>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_int(ctx, v.ull, s); });
    case fmt_kind::float_:
      return ::ycxx::detail::fmt_builtin<fmt_cat::floating>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_float(ctx, v.f, s); });
    case fmt_kind::double_:
      return ::ycxx::detail::fmt_builtin<fmt_cat::floating>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_float(ctx, v.d, s); });
    case fmt_kind::ldouble:
      return ::ycxx::detail::fmt_builtin<fmt_cat::floating>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_float(ctx, v.ld, s); });
    case fmt_kind::cstring:
      return ::ycxx::detail::fmt_builtin<fmt_cat::string>(pc, ctx, [&](const fmt_spec<charT>& s) {
        return ::ycxx::detail::fmt_write_string(ctx, v.s, std::char_traits<charT>::length(v.s), s);
      });
    case fmt_kind::string:
      return ::ycxx::detail::fmt_builtin<fmt_cat::string>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_write_string(ctx, v.sv.data(), v.sv.size(), s); });
    case fmt_kind::pointer:
      return ::ycxx::detail::fmt_builtin<fmt_cat::pointer>(pc, ctx, [&](const fmt_spec<charT>& s) { return ::ycxx::detail::fmt_format_pointer(ctx, v.p, s); });
    case fmt_kind::handle:
      return v.h.format(pc, ctx);
    }
  });
}

// The compile-time check of basic_format_string ([format.fmt.string]/3).
template <class T, class charT>
constexpr const charT* fmt_check_parse(std::basic_format_parse_context<charT>& pc) {
  std::formatter<T, charT> f;
  return f.parse(pc);
}
template <class charT, class... Args>
consteval void fmt_check(std::basic_string_view<charT> fmt) {
  constexpr bool formattable = (fmt_formattable_with<std::remove_reference_t<Args>, fmt_context<charT>> && ...);
  static_assert(formattable, "std::format: an argument type has no enabled formatter (std::formattable is false)");
  if constexpr (formattable) {
    constexpr fmt_kind kinds[] = {::ycxx::detail::fmt_kind_of<std::remove_cvref_t<Args>, charT>()..., fmt_kind::none};
    using parse_fn = const charT* (*)(std::basic_format_parse_context<charT>&);
    constexpr parse_fn parsers[] = {&::ycxx::detail::fmt_check_parse<std::remove_cvref_t<Args>, charT>..., nullptr};
    auto pc = fmt_access::parse_context<charT>(fmt, sizeof...(Args), kinds);
    ::ycxx::detail::fmt_scan(fmt, pc, [](const charT*, std::size_t) {}, [&](std::size_t id) {
      if (id >= sizeof...(Args))
        ::ycxx::detail::format_string_argument_index_out_of_range();
      else
        pc.advance_to(parsers[id](pc));
    });
  }
}

// vformat_to for any output iterator.
template <class charT, class Out>
constexpr Out fmt_vformat_to(Out out, std::basic_string_view<charT> fmt, fmt_args<charT> args, const std::locale* loc) {
  if constexpr (__is_same(Out, ycxx::adl_free::fmt_iter<charT>)) {
    ::ycxx::detail::fmt_vformat(fmt_access::buffer(out), fmt, args, loc);
    return out;
  } else if constexpr (__is_same(Out, charT*)) {
    fmt_ptr_sink<charT> sink(out);
    ::ycxx::detail::fmt_vformat(sink, fmt, args, loc);
    return sink.finish();
  } else {
    fmt_iter_sink<charT, Out> sink(static_cast<Out&&>(out));
    ::ycxx::detail::fmt_vformat(sink, fmt, args, loc);
    return sink.finish();
  }
}
template <class charT>
constexpr std::basic_string<charT> fmt_vformat_string(std::basic_string_view<charT> fmt, fmt_args<charT> args,
                                                      const std::locale* loc) {
  fmt_dynbuf<charT> buf;
  ::ycxx::detail::fmt_vformat(buf, fmt, args, loc);
  return std::basic_string<charT>(buf.data(), buf.size());
}
template <class charT, class Out>
constexpr std::format_to_n_result<Out> fmt_vformat_to_n(Out out, std::iter_difference_t<Out> n,
                                                        std::basic_string_view<charT> fmt, fmt_args<charT> args,
                                                        const std::locale* loc);
template <class charT>
constexpr std::size_t fmt_vformatted_size(std::basic_string_view<charT> fmt, fmt_args<charT> args,
                                          const std::locale* loc) {
  fmt_count_sink<charT, decltype(nullptr)> sink(nullptr, 0);
  ::ycxx::detail::fmt_vformat(sink, fmt, args, loc);
  return sink.finish();
}

} // namespace ycxx::detail

namespace std {

// [format.fmt.string]
template <class charT, class... Args>
struct basic_format_string {
private:
  basic_string_view<charT> str;

public:
  template <class T>
    requires convertible_to<const T&, basic_string_view<charT>>
  consteval basic_format_string(const T& s) : str(s) {
    ycxx::detail::fmt_check<charT, Args...>(str);
  }
  constexpr basic_format_string(ycxx::adl_free::dynamic_format_string<charT> s) noexcept : str(s.str_) {}
  constexpr basic_string_view<charT> get() const noexcept { return str; }
};
template <class... Args>
using format_string = basic_format_string<char, type_identity_t<Args>...>;
template <class... Args>
using wformat_string = basic_format_string<wchar_t, type_identity_t<Args>...>;

constexpr ycxx::adl_free::dynamic_format_string<char> dynamic_format(string_view fmt) noexcept { return fmt; }
constexpr ycxx::adl_free::dynamic_format_string<wchar_t> dynamic_format(wstring_view fmt) noexcept { return fmt; }
// runtime_format: the C++26 name of dynamic_format before P3953.
constexpr ycxx::adl_free::dynamic_format_string<char> runtime_format(string_view fmt) noexcept { return fmt; }
constexpr ycxx::adl_free::dynamic_format_string<wchar_t> runtime_format(wstring_view fmt) noexcept { return fmt; }

// [format.functions]
constexpr string vformat(string_view fmt, format_args args) {
  return ycxx::detail::fmt_vformat_string<char>(fmt, args, nullptr);
}
constexpr wstring vformat(wstring_view fmt, wformat_args args) {
  return ycxx::detail::fmt_vformat_string<wchar_t>(fmt, args, nullptr);
}
template <class... Args>
constexpr string format(format_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_string<char>(fmt.get(), make_format_args(args...), nullptr);
}
template <class... Args>
constexpr wstring format(wformat_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_string<wchar_t>(fmt.get(), make_wformat_args(args...), nullptr);
}

template <class Out>
  requires output_iterator<Out, const char&>
constexpr Out vformat_to(Out out, string_view fmt, format_args args) {
  return ycxx::detail::fmt_vformat_to<char>(static_cast<Out&&>(out), fmt, args, nullptr);
}
template <class Out>
  requires output_iterator<Out, const wchar_t&>
constexpr Out vformat_to(Out out, wstring_view fmt, wformat_args args) {
  return ycxx::detail::fmt_vformat_to<wchar_t>(static_cast<Out&&>(out), fmt, args, nullptr);
}
template <class Out, class... Args>
  requires output_iterator<Out, const char&>
constexpr Out format_to(Out out, format_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_to<char>(static_cast<Out&&>(out), fmt.get(), make_format_args(args...), nullptr);
}
template <class Out, class... Args>
  requires output_iterator<Out, const wchar_t&>
constexpr Out format_to(Out out, wformat_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_to<wchar_t>(static_cast<Out&&>(out), fmt.get(), make_wformat_args(args...),
                                               nullptr);
}

template <class Out, class... Args>
  requires output_iterator<Out, const char&>
constexpr format_to_n_result<Out> format_to_n(Out out, iter_difference_t<Out> n, format_string<Args...> fmt,
                                              Args&&... args) {
  return ycxx::detail::fmt_vformat_to_n<char>(static_cast<Out&&>(out), n, fmt.get(), make_format_args(args...), nullptr);
}
template <class Out, class... Args>
  requires output_iterator<Out, const wchar_t&>
constexpr format_to_n_result<Out> format_to_n(Out out, iter_difference_t<Out> n, wformat_string<Args...> fmt,
                                              Args&&... args) {
  return ycxx::detail::fmt_vformat_to_n<wchar_t>(static_cast<Out&&>(out), n, fmt.get(), make_wformat_args(args...),
                                                 nullptr);
}
template <class... Args>
constexpr size_t formatted_size(format_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformatted_size<char>(fmt.get(), make_format_args(args...), nullptr);
}
template <class... Args>
constexpr size_t formatted_size(wformat_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformatted_size<wchar_t>(fmt.get(), make_wformat_args(args...), nullptr);
}

} // namespace std

template <class charT, class Out>
constexpr std::format_to_n_result<Out> ycxx::detail::fmt_vformat_to_n(Out out, std::iter_difference_t<Out> n,
                                                                      std::basic_string_view<charT> fmt,
                                                                      fmt_args<charT> args, const std::locale* loc) {
  const std::size_t limit = n < 0 ? 0 : static_cast<std::size_t>(n);
  fmt_count_sink<charT, Out> sink(static_cast<Out&&>(out), limit);
  ::ycxx::detail::fmt_vformat(sink, fmt, args, loc);
  const std::size_t total = sink.finish();
  return {static_cast<Out&&>(sink.out()), static_cast<std::iter_difference_t<Out>>(total)};
}

// ---- the formatter specializations of [format.formatter.spec] -----------------------------------

namespace ycxx::adl_free {

// The formatters interpreting a std-format-spec.
template <class charT, ycxx::detail::fmt_cat Cat>
class fmt_std_formatter {
protected:
  ycxx::detail::fmt_spec<charT> spec_;

public:
  constexpr typename std::basic_format_parse_context<charT>::iterator parse(std::basic_format_parse_context<charT>& pc) {
    return ycxx::detail::fmt_parse_spec(pc, spec_, Cat);
  }
};

template <class charT>
class fmt_string_formatter : public fmt_std_formatter<charT, ycxx::detail::fmt_cat::string> {
public:
  constexpr void set_debug_format() { this->spec_.type = '?'; }

protected:
  template <class FormatContext>
  constexpr typename FormatContext::iterator do_format(const charT* p, std::size_t n, FormatContext& ctx) const {
    return ycxx::detail::fmt_write_string(ctx, p, n, this->spec_);
  }
};

} // namespace ycxx::adl_free

namespace std {

// /2.1: characters.
template <ycxx::detail::fmt_char charT>
struct formatter<charT, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::character> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(charT c, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_char(ctx, c, this->spec_);
  }
  constexpr void set_debug_format() { this->spec_.type = '?'; }
};
template <>
struct formatter<char, wchar_t> : ycxx::adl_free::fmt_std_formatter<wchar_t, ycxx::detail::fmt_cat::character> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(char c, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_char(ctx, static_cast<wchar_t>(static_cast<unsigned char>(c)), this->spec_);
  }
  constexpr void set_debug_format() { this->spec_.type = '?'; }
};

// /2.2: strings.
template <ycxx::detail::fmt_char charT>
struct formatter<charT*, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(charT* s, FormatContext& ctx) const {
    return this->do_format(s, char_traits<charT>::length(s), ctx);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<const charT*, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const charT* s, FormatContext& ctx) const {
    return this->do_format(s, char_traits<charT>::length(s), ctx);
  }
};
template <ycxx::detail::fmt_char charT, size_t N>
struct formatter<charT[N], charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const charT (&s)[N], FormatContext& ctx) const {
    size_t n = 0;
    while (n != N && s[n] != charT())
      ++n;
    return this->do_format(s, n, ctx);
  }
};
template <ycxx::detail::fmt_char charT, class traits, class Allocator>
struct formatter<basic_string<charT, traits, Allocator>, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const basic_string<charT, traits, Allocator>& s,
                                                    FormatContext& ctx) const {
    return this->do_format(s.data(), s.size(), ctx);
  }
};
template <ycxx::detail::fmt_char charT, class traits>
struct formatter<basic_string_view<charT, traits>, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(basic_string_view<charT, traits> s, FormatContext& ctx) const {
    return this->do_format(s.data(), s.size(), ctx);
  }
};

// /4: disabled.
template <>
struct formatter<char*, wchar_t> : ycxx::adl_free::fmt_disabled {};
template <>
struct formatter<const char*, wchar_t> : ycxx::adl_free::fmt_disabled {};
template <size_t N>
struct formatter<char[N], wchar_t> : ycxx::adl_free::fmt_disabled {};
template <class traits, class Allocator>
struct formatter<basic_string<char, traits, Allocator>, wchar_t> : ycxx::adl_free::fmt_disabled {};
template <class traits>
struct formatter<basic_string_view<char, traits>, wchar_t> : ycxx::adl_free::fmt_disabled {};

// /2.3: integers and bool.
template <class T, ycxx::detail::fmt_char charT>
  requires(__is_same(T, remove_cv_t<T>) && ycxx::detail::is_signed_or_unsigned_integer<T>)
struct formatter<T, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::integer> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(T value, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_int(ctx, value, this->spec_);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<bool, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::boolean> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(bool value, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_bool(ctx, value, this->spec_);
  }
};

// /2.4: floating-point types.
template <class T, ycxx::detail::fmt_char charT>
  requires ycxx::detail::fmt_is_float<T>
struct formatter<T, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::floating> {
  template <class FormatContext>
  typename FormatContext::iterator format(T value, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_float(ctx, value, this->spec_);
  }
};

// /2.5, /2.6: pointers.
template <ycxx::detail::fmt_char charT>
struct formatter<nullptr_t, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::pointer> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(nullptr_t, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_pointer(ctx, nullptr, this->spec_);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<void*, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::pointer> {
  template <class FormatContext>
  typename FormatContext::iterator format(void* p, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_pointer(ctx, p, this->spec_);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<const void*, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::pointer> {
  template <class FormatContext>
  typename FormatContext::iterator format(const void* p, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_pointer(ctx, p, this->spec_);
  }
};

// [format.formatter.spec]/3.
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<char> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<wchar_t> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<bool> = true;
template <class T>
  requires(__is_same(T, remove_cv_t<T>) &&
           (ycxx::detail::is_signed_or_unsigned_integer<T> || ycxx::detail::fmt_is_float<T>))
inline constexpr bool enable_nonlocking_formatter_optimization<T> = true;
template <ycxx::detail::fmt_char charT>
inline constexpr bool enable_nonlocking_formatter_optimization<charT*> = true;
template <ycxx::detail::fmt_char charT>
inline constexpr bool enable_nonlocking_formatter_optimization<const charT*> = true;
template <ycxx::detail::fmt_char charT, size_t N>
inline constexpr bool enable_nonlocking_formatter_optimization<charT[N]> = true;
template <ycxx::detail::fmt_char charT, class traits, class Allocator>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_string<charT, traits, Allocator>> = true;
template <ycxx::detail::fmt_char charT, class traits>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_string_view<charT, traits>> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<nullptr_t> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<void*> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<const void*> = true;

} // namespace std
