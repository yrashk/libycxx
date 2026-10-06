// libycxx hosted runtime: the locale machinery ([locales]) and the facets with out-of-line
// members: ctype<char>'s table, the four required codecvt specializations.
//
// A locale_impl holds an array of facet pointers indexed by locale::id and the locale's name
// (null when unnamed). A name is either one name for all categories or a composite
// "LC_COLLATE=...;LC_CTYPE=...;LC_MONETARY=...;LC_NUMERIC=...;LC_TIME=...;LC_MESSAGES=..." when
// the categories were taken from different names. The categories of a name with the classic
// semantics ("C", "POSIX", "C.UTF-8") have the classic facets; those of any other name the
// _byname facets built on the C library's locale of that name (src/hosted/locale_named.cpp),
// where the draft makes the facet depend on the locale: the codecvt facets other than
// codecvt<wchar_t, char, mbstate_t>, num_get/num_put and money_get/money_put stay the classic
// ones (they read the locale through the other facets).
#include <locale>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <new>
#include <ycxx/core/single_threaded.hpp>
#include <ycxx/hosted/memory_resource.hpp> // __ycxx::__detail::__pal_lock
#include "locale_named.hpp"

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __locale_impl {
  std::size_t __refs; // atomic
  const std::locale::facet** facets;
  std::size_t nfacets;
  char* name; // null: unnamed
};

struct __locale_access {
  static void __retain(const std::locale::facet* __f) noexcept { ::__ycxx::__detail::__ref_add(__f->__refs_); }
  static void release(const std::locale::facet* __f) noexcept {
    if (::__ycxx::__detail::__ref_release(__f->__refs_))
      delete __f;
  }
  static std::size_t index(const std::locale::id& i) noexcept { return i.index(); }
  static __locale_impl* __y_impl(const std::locale& __l) noexcept { return __l.__impl_; }
  static std::locale __make(__locale_impl* p) noexcept { return std::locale(__locale_impl_tag(), p); }
};

}} // namespace __ycxx::__detail

namespace {

using __ycxx::__detail::__locale_access;
using __ycxx::__detail::__locale_impl;

std::size_t next_facet_index = 0; // atomic; ids are 1, 2, ...

// Zero-filled arrays from ::operator new (not new[]: the program's array allocation functions
// see no calls from the locale machinery).
template <class _Tp>
_Tp* new_zeroed(std::size_t n) {
  void* p = ::operator new(n * sizeof(_Tp));
  std::memset(p, 0, n * sizeof(_Tp));
  return static_cast<_Tp*>(p);
}

char* copy_name(const char* s) {
  if (s == nullptr)
    return nullptr;
  const std::size_t n = std::strlen(s) + 1;
  char* p = new_zeroed<char>(n);
  std::memcpy(p, s, n);
  return p;
}

__locale_impl* new_impl(std::size_t nfacets, const char* name) {
  __locale_impl* p = new __locale_impl{1, nullptr, 0, nullptr};
  if constexpr (__ycxx::__detail::__cfg::exceptions) {
    try {
      p->facets = new_zeroed<const std::locale::facet*>(nfacets);
      p->nfacets = nfacets;
      p->name = copy_name(name);
    } catch (...) {
      ::operator delete(p->facets);
      delete p;
      throw;
    }
  } else {
    p->facets = new_zeroed<const std::locale::facet*>(nfacets);
    p->nfacets = nfacets;
    p->name = copy_name(name);
  }
  return p;
}

void __retain(__locale_impl* p) noexcept { ::__ycxx::__detail::__ref_add(p->__refs); }
void release(__locale_impl* p) noexcept {
  if (!::__ycxx::__detail::__ref_release(p->__refs))
    return;
  for (std::size_t i = 0; i < p->nfacets; ++i)
    if (p->facets[i] != nullptr)
      __locale_access::release(p->facets[i]);
  ::operator delete(p->facets);
  ::operator delete(p->name);
  delete p;
}

// A copy of src's facets in a new impl with room for index `__need`.
__locale_impl* __clone(const __locale_impl* __src, std::size_t __need, const char* name) {
  std::size_t n = __src->nfacets;
  if (__need >= n)
    n = __need + 1;
  __locale_impl* p = new_impl(n, name);
  for (std::size_t i = 0; i < __src->nfacets; ++i) {
    p->facets[i] = __src->facets[i];
    if (p->facets[i] != nullptr)
      __locale_access::__retain(p->facets[i]);
  }
  return p;
}

void set_facet(__locale_impl* p, std::size_t i, const std::locale::facet* __f) noexcept {
  if (__f != nullptr)
    __locale_access::__retain(__f);
  if (p->facets[i] != nullptr)
    __locale_access::release(p->facets[i]);
  p->facets[i] = __f;
}

// ---- categories --------------------------------------------------------------------------------

constexpr int ncategories = 6;
constexpr std::locale::category category_bits[ncategories] = {std::locale::collate, std::locale::ctype,
                                                               std::locale::monetary, std::locale::numeric,
                                                               std::locale::time, std::locale::messages};
constexpr const char* category_names[ncategories] = {"LC_COLLATE", "LC_CTYPE", "LC_MONETARY",
                                                     "LC_NUMERIC", "LC_TIME", "LC_MESSAGES"};
constexpr int c_categories[ncategories] = {LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC, LC_TIME, LC_MESSAGES};

// The facet ids of each category ([locale.category] Table 91).
struct category_ids {
  const std::locale::id* ids[12];
  int n;
};
const category_ids& ids_of(int c) {
  static const category_ids table[ncategories] = {
      {{&std::collate<char>::id, &std::collate<wchar_t>::id}, 2},
      {{&std::ctype<char>::id, &std::ctype<wchar_t>::id, &std::codecvt<char, char, std::mbstate_t>::id,
        &std::codecvt<wchar_t, char, std::mbstate_t>::id, &std::codecvt<char16_t, char8_t, std::mbstate_t>::id,
        &std::codecvt<char32_t, char8_t, std::mbstate_t>::id, &std::codecvt<char16_t, char, std::mbstate_t>::id,
        &std::codecvt<char32_t, char, std::mbstate_t>::id},
       8},
      {{&std::moneypunct<char, false>::id, &std::moneypunct<char, true>::id, &std::moneypunct<wchar_t, false>::id,
        &std::moneypunct<wchar_t, true>::id, &std::money_get<char>::id, &std::money_get<wchar_t>::id,
        &std::money_put<char>::id, &std::money_put<wchar_t>::id},
       8},
      {{&std::numpunct<char>::id, &std::numpunct<wchar_t>::id, &std::num_get<char>::id, &std::num_get<wchar_t>::id,
        &std::num_put<char>::id, &std::num_put<wchar_t>::id},
       6},
      {{&std::time_get<char>::id, &std::time_get<wchar_t>::id, &std::time_put<char>::id, &std::time_put<wchar_t>::id},
       4},
      {{&std::messages<char>::id, &std::messages<wchar_t>::id}, 2},
  };
  return table[c];
}

// ---- names -------------------------------------------------------------------------------------

// The per-category names of a (possibly composite) locale name.
struct split_name {
  std::string part[ncategories];
};
bool split(const char* name, split_name& out) {
  if (std::strchr(name, '=') == nullptr) {
    for (auto& p : out.part)
      p = name;
    return true;
  }
  bool __seen[ncategories] = {};
  const char* s = name;
  while (*s) {
    const char* eq = std::strchr(s, '=');
    if (eq == nullptr)
      return false;
    const char* end = std::strchr(eq, ';');
    if (end == nullptr)
      end = eq + std::strlen(eq);
    int c = 0;
    while (c < ncategories && !(std::strlen(category_names[c]) == static_cast<std::size_t>(eq - s) &&
                                std::strncmp(category_names[c], s, static_cast<std::size_t>(eq - s)) == 0))
      ++c;
    if (c == ncategories)
      return false;
    out.part[c].assign(eq + 1, end);
    __seen[c] = true;
    s = *end ? end + 1 : end;
  }
  for (bool b : __seen)
    if (!b)
      return false;
  return true;
}
std::string join(const split_name& n) {
  bool __same = true;
  for (int c = 1; c < ncategories; ++c)
    __same = __same && n.part[c] == n.part[0];
  if (__same)
    return n.part[0];
  std::string r;
  for (int c = 0; c < ncategories; ++c) {
    if (c != 0)
      r += ';';
    r += category_names[c];
    r += '=';
    r += n.part[c];
  }
  return r;
}

// The environment's name for category c ([locale.cons]/4: ""): LC_ALL, LC_<category>, LANG;
// "C" when none is set or the C library has no locale of that name.
std::string environment_name(int c) {
  const char* __v = std::getenv("LC_ALL");
  if (__v == nullptr || *__v == '\0')
    __v = std::getenv(category_names[c]);
  if (__v == nullptr || *__v == '\0')
    __v = std::getenv("LANG");
  if (__v == nullptr || *__v == '\0' || std::strchr(__v, '=') != nullptr)
    return "C";
  if (const char* n = __ycxx::__detail::__classic_locale_name(__v))
    return n;
  return __ycxx::__detail::__named_exists(__v, c) ? std::string(__v) : std::string("C");
}

// Splits a std_name into its names per category (the environment's for "" and for an empty
// part; "POSIX" is "C"); false if malformed. Whether the C library has them is found when the
// facets are built.
bool resolve_parts(const char* name, split_name& __parts) {
  if (name == nullptr)
    return false;
  if (*name == '\0') {
    for (int c = 0; c < ncategories; ++c)
      __parts.part[c] = environment_name(c);
    return true;
  }
  if (!split(name, __parts))
    return false;
  for (int c = 0; c < ncategories; ++c) {
    if (__parts.part[c].empty())
      __parts.part[c] = environment_name(c);
    else if (const char* n = __ycxx::__detail::__classic_locale_name(__parts.part[c].c_str()))
      __parts.part[c] = n;
  }
  return true;
}

[[noreturn]] void bad_name(const char* what, const char* name) {
  std::string __msg(what);
  __msg += ": unsupported locale name \"";
  if (name != nullptr)
    __msg += name;
  __msg += '"';
  ::__ycxx::__detail::__throw_runtime_error(__msg.c_str());
}

// ---- the classic locale ------------------------------------------------------------------------

template <class _Tp>
union immortal {
  _Tp __object;
  immortal() {}
  ~immortal() {}
};

// The classic locale's facets and tables live in static storage, never freed: no allocation
// function is called for them, so a program that replaces operator new and counts what is
// outstanding does not count them.
template <class _Fp, class... _Ap>
_Fp* in_static(_Ap... a) {
  alignas(_Fp) static unsigned char __storage[sizeof(_Fp)];
  return ::new (static_cast<void*>(__storage)) _Fp(a...);
}

__locale_impl* make_classic() {
  // refs == 1: the classic facets are never deleted
  const std::locale::facet* facets[] = {
      in_static<std::collate<char>>(1),
      in_static<std::collate<wchar_t>>(1),
      in_static<std::ctype<char>>(nullptr, false, 1),
      in_static<std::ctype<wchar_t>>(1),
      in_static<std::codecvt<char, char, std::mbstate_t>>(1),
      in_static<std::codecvt<wchar_t, char, std::mbstate_t>>(1),
      in_static<std::codecvt<char16_t, char8_t, std::mbstate_t>>(1),
      in_static<std::codecvt<char32_t, char8_t, std::mbstate_t>>(1),
      in_static<std::codecvt<char16_t, char, std::mbstate_t>>(1),
      in_static<std::codecvt<char32_t, char, std::mbstate_t>>(1),
      in_static<std::moneypunct<char, false>>(1),
      in_static<std::moneypunct<char, true>>(1),
      in_static<std::moneypunct<wchar_t, false>>(1),
      in_static<std::moneypunct<wchar_t, true>>(1),
      in_static<std::money_get<char>>(1),
      in_static<std::money_get<wchar_t>>(1),
      in_static<std::money_put<char>>(1),
      in_static<std::money_put<wchar_t>>(1),
      in_static<std::numpunct<char>>(1),
      in_static<std::numpunct<wchar_t>>(1),
      in_static<std::num_get<char>>(1),
      in_static<std::num_get<wchar_t>>(1),
      in_static<std::num_put<char>>(1),
      in_static<std::num_put<wchar_t>>(1),
      in_static<std::time_get<char>>(1),
      in_static<std::time_get<wchar_t>>(1),
      in_static<std::time_put<char>>(1),
      in_static<std::time_put<wchar_t>>(1),
      in_static<std::messages<char>>(1),
      in_static<std::messages<wchar_t>>(1),
  };
  const std::locale::id* ids[] = {
      &std::collate<char>::id,
      &std::collate<wchar_t>::id,
      &std::ctype<char>::id,
      &std::ctype<wchar_t>::id,
      &std::codecvt<char, char, std::mbstate_t>::id,
      &std::codecvt<wchar_t, char, std::mbstate_t>::id,
      &std::codecvt<char16_t, char8_t, std::mbstate_t>::id,
      &std::codecvt<char32_t, char8_t, std::mbstate_t>::id,
      &std::codecvt<char16_t, char, std::mbstate_t>::id,
      &std::codecvt<char32_t, char, std::mbstate_t>::id,
      &std::moneypunct<char, false>::id,
      &std::moneypunct<char, true>::id,
      &std::moneypunct<wchar_t, false>::id,
      &std::moneypunct<wchar_t, true>::id,
      &std::money_get<char>::id,
      &std::money_get<wchar_t>::id,
      &std::money_put<char>::id,
      &std::money_put<wchar_t>::id,
      &std::numpunct<char>::id,
      &std::numpunct<wchar_t>::id,
      &std::num_get<char>::id,
      &std::num_get<wchar_t>::id,
      &std::num_put<char>::id,
      &std::num_put<wchar_t>::id,
      &std::time_get<char>::id,
      &std::time_get<wchar_t>::id,
      &std::time_put<char>::id,
      &std::time_put<wchar_t>::id,
      &std::messages<char>::id,
      &std::messages<wchar_t>::id,
  };
  std::size_t top = 0;
  for (const std::locale::id* i : ids) {
    const std::size_t k = __locale_access::index(*i);
    if (k > top)
      top = k;
  }
  // in static storage too while the ids fit (they do unless a program assigns many before the
  // classic locale is first used)
  static const std::locale::facet* table[64];
  static char name[] = "C";
  static __locale_impl __y_impl{1, table, 64, name};
  __locale_impl* p = top + 1 <= 64 ? &__y_impl : new_impl(top + 1, "C");
  for (std::size_t k = 0; k < sizeof ids / sizeof ids[0]; ++k)
    set_facet(p, __locale_access::index(*ids[k]), facets[k]);
  return p;
}

const std::locale& classic_locale() {
  static immortal<std::locale> c;
  static const bool __built = (::new (static_cast<void*>(__builtin_addressof(c.__object)))
                                 std::locale(__locale_access::__make(make_classic())),
                             true);
  (void)__built;
  return c.__object;
}

// The classic locale is built before any object of the program with ordinary static
// initialization (it is also built on first use, should a runtime initializer need it sooner),
// so the program never sees its allocations as its own. Mach-O has no initialization priorities
// (_YCXX_HAS_INIT_PRIORITY): there it is built before main, by the first ios_base::Init (every
// translation unit including <iostream> has one) or by this object in link order, or on first
// use by an earlier static initializer of the program.
struct build_classic {
  build_classic() { classic_locale(); }
};
#if _YCXX_HAS_INIT_PRIORITY
[[__gnu__::__init_priority__(100)]] build_classic classic_at_startup;
#else
build_classic classic_at_startup;
#endif

// ---- the facets of a named category -------------------------------------------------------------

// Builds the facets of category c for name (a single name without classic semantics), in the
// order of ids_of(c); null where the classic facet is kept. All or nothing: throws
// runtime_error (naming what) if the C library has no such locale.
void build_named(int c, const char* name, const char* what, const std::locale::facet** out) {
  // held while the facets are built, so they share one opening of the C library's locale
  __ycxx::__detail::__named_locale* h = __ycxx::__detail::__named_open(name, category_bits[c], what);
  struct held {
    __ycxx::__detail::__named_locale* h;
    const std::locale::facet** out;
    int n;
    ~held() {
      __ycxx::__detail::__named_release(h);
      for (int k = 0; k < n; ++k) // facets built before an exception (n is 0 on success)
        if (out[k] != nullptr) {
          __locale_access::__retain(out[k]);
          __locale_access::release(out[k]);
        }
    }
  } __guard{h, out, 0};
  const int n = ids_of(c).n;
  for (int k = 0; k < n; ++k)
    out[k] = nullptr;
  __guard.n = n;
  switch (c) {
  case 0:
    out[0] = new std::collate_byname<char>(name);
    out[1] = new std::collate_byname<wchar_t>(name);
    break;
  case 1:
    out[0] = new std::ctype_byname<char>(name);
    out[1] = new std::ctype_byname<wchar_t>(name);
    out[3] = new std::codecvt_byname<wchar_t, char, std::mbstate_t>(name);
    break;
  case 2:
    out[0] = new std::moneypunct_byname<char, false>(name);
    out[1] = new std::moneypunct_byname<char, true>(name);
    out[2] = new std::moneypunct_byname<wchar_t, false>(name);
    out[3] = new std::moneypunct_byname<wchar_t, true>(name);
    break;
  case 3:
    out[0] = new std::numpunct_byname<char>(name);
    out[1] = new std::numpunct_byname<wchar_t>(name);
    break;
  case 4:
    out[0] = new std::time_get_byname<char>(name);
    out[1] = new std::time_get_byname<wchar_t>(name);
    out[2] = new std::time_put_byname<char>(name);
    out[3] = new std::time_put_byname<wchar_t>(name);
    break;
  default:
    out[0] = new std::messages_byname<char>(name);
    out[1] = new std::messages_byname<wchar_t>(name);
    break;
  }
  __guard.n = 0;
}

// Installs in p the facets of category c that implement `name` (one name): the classic ones,
// or those built by build_named.
void install_category(__locale_impl* p, int c, const char* name, const char* what) {
  const __locale_impl* __one = __locale_access::__y_impl(classic_locale());
  const category_ids& ids = ids_of(c);
  const std::locale::facet* __built[12] = {};
  if (__ycxx::__detail::__classic_locale_name(name) == nullptr)
    build_named(c, name, what, __built);
  for (int k = 0; k < ids.n; ++k) {
    const std::size_t i = __locale_access::index(*ids.ids[k]);
    set_facet(p, i, __built[k] != nullptr ? __built[k] : (i < __one->nfacets ? __one->facets[i] : nullptr));
  }
}

// Installs the categories cats of parts in p; on an exception p is released.
void install_parts(__locale_impl* p, const split_name& __parts, std::locale::category __cats) {
  if constexpr (__ycxx::__detail::__cfg::exceptions) {
    try {
      for (int c = 0; c < ncategories; ++c)
        if (__cats & category_bits[c])
          install_category(p, c, __parts.part[c].c_str(), "std::locale");
    } catch (...) {
      release(p);
      throw;
    }
  } else {
    for (int c = 0; c < ncategories; ++c)
      if (__cats & category_bits[c])
        install_category(p, c, __parts.part[c].c_str(), "std::locale");
  }
}

// The global locale ([locale.statics]): one for the program, under a lock.
__ycxx::__detail::__pal_lock global_lock;
__locale_impl* global_impl = nullptr; // null: the classic locale

struct lock_guard {
  explicit lock_guard(__ycxx::__detail::__pal_lock& __l) noexcept : l_(__l) { l_.lock(); }
  ~lock_guard() { l_.unlock(); }
  lock_guard(const lock_guard&) = delete;
  __ycxx::__detail::__pal_lock& l_;
};

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] std {

locale::facet::~facet() {}

size_t locale::id::assign() const noexcept {
  const size_t __mine = __atomic_add_fetch(&next_facet_index, 1, __ATOMIC_RELAXED);
  size_t expected = 0;
  if (__atomic_compare_exchange_n(&__index_, &expected, __mine, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    return __mine;
  return expected;
}

locale::locale() noexcept {
  const locale& c = classic_locale();
  lock_guard __g(global_lock);
  __impl_ = global_impl != nullptr ? global_impl : c.__impl_;
  __retain(__impl_);
}

locale::locale(const locale& other) noexcept : __impl_(other.__impl_) { __retain(__impl_); }

locale::locale(const char* __std_name) : __impl_(nullptr) {
  split_name __parts;
  if (!resolve_parts(__std_name, __parts))
    bad_name("std::locale", __std_name);
  const string name = join(__parts);
  __locale_impl* p = __clone(classic_locale().__impl_, 0, name.c_str());
  install_parts(p, __parts, all);
  __impl_ = p;
}

locale::locale(const locale& other, const char* __std_name, category __cats) : __impl_(nullptr) {
  split_name b;
  if (!resolve_parts(__std_name, b))
    bad_name("std::locale", __std_name);
  const __locale_impl* __one = classic_locale().__impl_;
  string result;
  if (other.__impl_->name != nullptr) {
    split_name a;
    split(other.__impl_->name, a);
    for (int c = 0; c < ncategories; ++c)
      if (__cats & category_bits[c])
        a.part[c] = b.part[c];
    result = join(a);
  }
  __locale_impl* p = __clone(other.__impl_, __one->nfacets, other.__impl_->name != nullptr ? result.c_str() : nullptr);
  install_parts(p, b, __cats);
  __impl_ = p;
}

locale::locale(const locale& other, const locale& __one, category __cats) : __impl_(nullptr) {
  string result;
  // [locale.cons]/15 (LWG 3676): with cats none, named iff other is; otherwise iff both are
  const bool named = other.__impl_->name != nullptr && (__cats == none || __one.__impl_->name != nullptr);
  if (named) {
    split_name a, b;
    split(other.__impl_->name, a);
    if (__cats != none) {
      split(__one.__impl_->name, b);
      for (int c = 0; c < ncategories; ++c)
        if (__cats & category_bits[c])
          a.part[c] = b.part[c];
    }
    result = join(a);
  }
  __locale_impl* p = __clone(other.__impl_, __one.__impl_->nfacets, named ? result.c_str() : nullptr);
  for (int c = 0; c < ncategories; ++c) {
    if (!(__cats & category_bits[c]))
      continue;
    const category_ids& ids = ids_of(c);
    for (int k = 0; k < ids.n; ++k) {
      const size_t i = __locale_access::index(*ids.ids[k]);
      set_facet(p, i, i < __one.__impl_->nfacets ? __one.__impl_->facets[i] : nullptr);
    }
  }
  __impl_ = p;
}

locale::locale(const locale& other, const facet* __f, const id& i) : __impl_(nullptr) {
  if (__f == nullptr) {
    __impl_ = other.__impl_;
    __retain(__impl_);
    return;
  }
  const size_t k = i.index();
  __locale_impl* p = __clone(other.__impl_, k, nullptr);
  set_facet(p, k, __f);
  __impl_ = p;
}

locale::~locale() { release(__impl_); }

const locale& locale::operator=(const locale& other) noexcept {
  __retain(other.__impl_);
  release(__impl_);
  __impl_ = other.__impl_;
  return *this;
}

string locale::name() const { return __impl_->name != nullptr ? string(__impl_->name) : string("*"); }

bool locale::operator==(const locale& other) const {
  if (__impl_ == other.__impl_)
    return true;
  return __impl_->name != nullptr && other.__impl_->name != nullptr && std::strcmp(__impl_->name, other.__impl_->name) == 0;
}

const locale::facet* locale::find(const id& i) const noexcept {
  const size_t k = i.index();
  return k < __impl_->nfacets ? __impl_->facets[k] : nullptr;
}

locale locale::global(const locale& __loc) {
  const locale& c = classic_locale();
  __locale_impl* __old;
  split_name __parts; // before the reference is taken: split may throw bad_alloc
  const bool named = __loc.__impl_->name != nullptr;
  if (named)
    split(__loc.__impl_->name, __parts);
  __retain(__loc.__impl_);
  {
    // setlocale under the same lock, so that concurrent calls leave the C library's locale and
    // the global locale set by the same call
    lock_guard __g(global_lock);
    __old = global_impl;
    global_impl = __loc.__impl_;
    if (named) {
      bool __same = true;
      for (int k = 1; k < ncategories; ++k)
        __same = __same && __parts.part[k] == __parts.part[0];
      if (__same) {
        ::setlocale(LC_ALL, __parts.part[0].c_str());
      } else {
        for (int k = 0; k < ncategories; ++k)
          ::setlocale(c_categories[k], __parts.part[k].c_str());
      }
    }
  }
  if (__old == nullptr) { // the classic locale, whose reference the global did not hold
    __old = c.__impl_;
    __retain(__old);
  }
  return __locale_access::__make(__old); // adopts the global's reference
}

const locale& locale::classic() { return classic_locale(); }

// ---- ctype<char> ------------------------------------------------------------------------------

locale::id ctype<char>::id;

const ctype_base::mask* ctype<char>::classic_table() noexcept {
  static constexpr struct table {
    mask m[table_size];
    constexpr table() : m() {
      for (int c = 0; c < 128; ++c)
        m[c] = ::__ycxx::__detail::__ascii_masks.m[c];
    }
  } t;
  return t.m;
}

ctype<char>::~ctype() {
  if (__del_)
    delete[] __table_;
}

} // namespace std

// ---- codecvt ----------------------------------------------------------------------------------

namespace {

using result = std::codecvt_base::result;

// The pending UTF-16 surrogate kept in an mbstate_t between calls (0: none), in its first four
// bytes. std::mbstate_t is the C library's opaque type (glibc and musl: 8 bytes, Darwin: 128);
// only the object representation is used, so a zero-initialised state holds none. These facets
// keep nothing else, so no other state (a side table) is needed.
static_assert(sizeof(std::mbstate_t) >= sizeof(char32_t));
char32_t __load_state(const std::mbstate_t& __st) noexcept {
  char32_t __v;
  __builtin_memcpy(&__v, &__st, sizeof __v);
  return __v;
}
void __store_state(std::mbstate_t& __st, char32_t __v) noexcept { __builtin_memcpy(&__st, &__v, sizeof __v); }

// do_unshift of the UTF-16 facets. A high surrogate taken by utf16_out and still waiting for its
// low half cannot be terminated: alone it is not a character UTF-8 can encode (Table 94: error,
// rather than noconv, which would drop it silently). A low surrogate pending from utf16_in
// belongs to the other direction and needs no termination.
template <class _Ep>
result utf16_unshift(const std::mbstate_t& state, _Ep* to, _Ep*& __to_next) noexcept {
  __to_next = to;
  const char32_t pending = __load_state(state);
  return pending >= 0xD800 && pending <= 0xDBFF ? std::codecvt_base::error : std::codecvt_base::noconv;
}

// Decodes one UTF-8 sequence at [p, end): returns its length (0: incomplete, -1: invalid).
int decode_utf8(const unsigned char* p, const unsigned char* end, char32_t& __cp) noexcept {
  const unsigned char b = p[0];
  int __len;
  char32_t __v;
  if (b < 0x80) {
    __cp = b;
    return 1;
  }
  if (b >= 0xC2 && b <= 0xDF) {
    __len = 2;
    __v = b & 0x1F;
  } else if (b >= 0xE0 && b <= 0xEF) {
    __len = 3;
    __v = b & 0x0F;
  } else if (b >= 0xF0 && b <= 0xF4) {
    __len = 4;
    __v = b & 0x07;
  } else {
    return -1;
  }
  for (int i = 1; i < __len; ++i) {
    if (p + i == end)
      return 0;
    const unsigned char c = p[i];
    if ((c & 0xC0) != 0x80)
      return -1;
    // the second byte's range rules out overlong forms, surrogates and values above 0x10FFFF
    if (i == 1) {
      if (b == 0xE0 && c < 0xA0)
        return -1;
      if (b == 0xED && c > 0x9F)
        return -1;
      if (b == 0xF0 && c < 0x90)
        return -1;
      if (b == 0xF4 && c > 0x8F)
        return -1;
    }
    __v = (__v << 6) | (c & 0x3F);
  }
  __cp = __v;
  return __len;
}

int encode_utf8(char32_t __cp, unsigned char* out) noexcept {
  if (__cp < 0x80) {
    out[0] = static_cast<unsigned char>(__cp);
    return 1;
  }
  if (__cp < 0x800) {
    out[0] = static_cast<unsigned char>(0xC0 | (__cp >> 6));
    out[1] = static_cast<unsigned char>(0x80 | (__cp & 0x3F));
    return 2;
  }
  if (__cp < 0x10000) {
    out[0] = static_cast<unsigned char>(0xE0 | (__cp >> 12));
    out[1] = static_cast<unsigned char>(0x80 | ((__cp >> 6) & 0x3F));
    out[2] = static_cast<unsigned char>(0x80 | (__cp & 0x3F));
    return 3;
  }
  out[0] = static_cast<unsigned char>(0xF0 | (__cp >> 18));
  out[1] = static_cast<unsigned char>(0x80 | ((__cp >> 12) & 0x3F));
  out[2] = static_cast<unsigned char>(0x80 | ((__cp >> 6) & 0x3F));
  out[3] = static_cast<unsigned char>(0x80 | (__cp & 0x3F));
  return 4;
}

bool valid_scalar(char32_t __cp) noexcept { return __cp < 0xD800 || (__cp > 0xDFFF && __cp <= 0x10FFFF); }

// UTF-32 (I = wchar_t or char32_t) to UTF-8 (E = char or char8_t).
template <class _Ip, class _Ep>
result utf32_out(const _Ip* from, const _Ip* __from_end, const _Ip*& __from_next, _Ep* to, _Ep* __to_end, _Ep*& __to_next) {
  for (; from != __from_end; ++from) {
    const char32_t __cp = static_cast<char32_t>(*from);
    if (!valid_scalar(__cp)) {
      __from_next = from;
      __to_next = to;
      return std::codecvt_base::error;
    }
    unsigned char __buf[4];
    const int n = encode_utf8(__cp, __buf);
    if (__to_end - to < n) {
      __from_next = from;
      __to_next = to;
      return std::codecvt_base::partial;
    }
    for (int i = 0; i < n; ++i)
      *to++ = static_cast<_Ep>(__buf[i]);
  }
  __from_next = from;
  __to_next = to;
  return std::codecvt_base::ok;
}

template <class _Ip, class _Ep>
result utf32_in(const _Ep* from, const _Ep* __from_end, const _Ep*& __from_next, _Ip* to, _Ip* __to_end, _Ip*& __to_next) {
  while (from != __from_end) {
    if (to == __to_end) {
      __from_next = from;
      __to_next = to;
      return std::codecvt_base::partial;
    }
    char32_t __cp;
    const int n = decode_utf8(reinterpret_cast<const unsigned char*>(from), reinterpret_cast<const unsigned char*>(__from_end), __cp);
    if (n <= 0) {
      __from_next = from;
      __to_next = to;
      return n == 0 ? std::codecvt_base::partial : std::codecvt_base::error;
    }
    *to++ = static_cast<_Ip>(__cp);
    from += n;
  }
  __from_next = from;
  __to_next = to;
  return std::codecvt_base::ok;
}

// UTF-16 to UTF-8 (E = char or char8_t). A supplementary character is split across calls through
// the state ([locale.codecvt.virtuals]/4): a high surrogate is consumed alone and kept.
template <class _Ep>
result utf16_out(std::mbstate_t& state, const char16_t* from, const char16_t* __from_end, const char16_t*& __from_next,
                 _Ep* to, _Ep* __to_end, _Ep*& __to_next) {
  char32_t __high = __load_state(state);
  // where the pending high surrogate was taken: the start when it came in through the state
  const char16_t* high_at = from;
  for (; from != __from_end; ++from) {
    const char16_t __u = *from;
    char32_t __cp;
    if (__high != 0) {
      if (__u < 0xDC00 || __u > 0xDFFF) {
        // the unpaired high surrogate is the character that cannot be converted
        // ([locale.codecvt.virtuals]/3: from_next is one beyond the last converted element)
        __from_next = high_at;
        __to_next = to;
        return std::codecvt_base::error;
      }
      __cp = 0x10000 + ((__high - 0xD800) << 10) + (__u - 0xDC00);
    } else if (__u >= 0xD800 && __u <= 0xDBFF) {
      __high = __u;
      high_at = from;
      __store_state(state, __high);
      continue;
    } else if (__u >= 0xDC00 && __u <= 0xDFFF) {
      __from_next = from;
      __to_next = to;
      return std::codecvt_base::error;
    } else {
      __cp = __u;
    }
    unsigned char __buf[4];
    const int n = encode_utf8(__cp, __buf);
    if (__to_end - to < n) {
      // the pending high surrogate (if any) stays in the state; u is not consumed
      __from_next = from;
      __to_next = to;
      return std::codecvt_base::partial;
    }
    for (int i = 0; i < n; ++i)
      *to++ = static_cast<_Ep>(__buf[i]);
    __high = 0;
    __store_state(state, 0);
  }
  __from_next = from;
  __to_next = to;
  return std::codecvt_base::ok;
}

// UTF-8 to UTF-16: with room for one unit only, the high surrogate is written and the low one
// kept in the state; the UTF-8 sequence is consumed when the low one is written.
template <class _Ep>
result utf16_in(std::mbstate_t& state, const _Ep* from, const _Ep* __from_end, const _Ep*& __from_next, char16_t* to,
                char16_t* __to_end, char16_t*& __to_next) {
  while (from != __from_end) {
    if (to == __to_end) {
      __from_next = from;
      __to_next = to;
      return std::codecvt_base::partial;
    }
    char32_t __cp;
    const int n =
        decode_utf8(reinterpret_cast<const unsigned char*>(from), reinterpret_cast<const unsigned char*>(__from_end), __cp);
    if (n <= 0) {
      __from_next = from;
      __to_next = to;
      return n == 0 ? std::codecvt_base::partial : std::codecvt_base::error;
    }
    if (__cp < 0x10000) {
      *to++ = static_cast<char16_t>(__cp);
      from += n;
      continue;
    }
    const char16_t __hi = static_cast<char16_t>(0xD800 + ((__cp - 0x10000) >> 10));
    const char16_t __lo = static_cast<char16_t>(0xDC00 + ((__cp - 0x10000) & 0x3FF));
    if (__load_state(state) != 0) {
      // the high surrogate was written by the previous call
      *to++ = __lo;
      __store_state(state, 0);
      from += n;
      continue;
    }
    *to++ = __hi;
    if (to == __to_end) {
      __store_state(state, __lo);
      __from_next = from;
      __to_next = to;
      return std::codecvt_base::ok;
    }
    *to++ = __lo;
    from += n;
  }
  __from_next = from;
  __to_next = to;
  return std::codecvt_base::ok;
}

template <class _Ep>
int utf8_length(const _Ep* from, const _Ep* end, std::size_t max, int units_per_cp_max) {
  const _Ep* p = from;
  std::size_t produced = 0;
  while (p != end && produced < max) {
    char32_t __cp;
    const int n = decode_utf8(reinterpret_cast<const unsigned char*>(p), reinterpret_cast<const unsigned char*>(end), __cp);
    if (n <= 0)
      break;
    const std::size_t __units = (units_per_cp_max == 2 && __cp >= 0x10000) ? 2 : 1;
    if (produced + __units > max)
      break;
    produced += __units;
    p += n;
  }
  return static_cast<int>(p - from);
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] std {

// codecvt<char, char, mbstate_t>: the degenerate conversion.
locale::id codecvt<char, char, mbstate_t>::id;
codecvt<char, char, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<char, char, mbstate_t>::do_out(mbstate_t&, const char* from, const char*,
                                                            const char*& __from_next, char* to, char*,
                                                            char*& __to_next) const {
  __from_next = from;
  __to_next = to;
  return noconv;
}
codecvt_base::result codecvt<char, char, mbstate_t>::do_in(mbstate_t&, const char* from, const char*,
                                                           const char*& __from_next, char* to, char*,
                                                           char*& __to_next) const {
  __from_next = from;
  __to_next = to;
  return noconv;
}
codecvt_base::result codecvt<char, char, mbstate_t>::do_unshift(mbstate_t&, char* to, char*, char*& __to_next) const {
  __to_next = to;
  return noconv;
}
int codecvt<char, char, mbstate_t>::do_encoding() const noexcept { return 1; }
bool codecvt<char, char, mbstate_t>::do_always_noconv() const noexcept { return true; }
int codecvt<char, char, mbstate_t>::do_length(mbstate_t&, const char* from, const char* end, size_t max) const {
  const size_t n = static_cast<size_t>(end - from);
  return static_cast<int>(n < max ? n : max);
}
int codecvt<char, char, mbstate_t>::do_max_length() const noexcept { return 1; }

// codecvt<wchar_t, char, mbstate_t>: UTF-32 <-> UTF-8.
locale::id codecvt<wchar_t, char, mbstate_t>::id;
codecvt<wchar_t, char, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_out(mbstate_t&, const wchar_t* from, const wchar_t* __from_end,
                                                               const wchar_t*& __from_next, char* to, char* __to_end,
                                                               char*& __to_next) const {
  return utf32_out(from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_in(mbstate_t&, const char* from, const char* __from_end,
                                                              const char*& __from_next, wchar_t* to, wchar_t* __to_end,
                                                              wchar_t*& __to_next) const {
  return utf32_in(from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_unshift(mbstate_t&, char* to, char*, char*& __to_next) const {
  __to_next = to;
  return noconv;
}
int codecvt<wchar_t, char, mbstate_t>::do_encoding() const noexcept { return 0; }
bool codecvt<wchar_t, char, mbstate_t>::do_always_noconv() const noexcept { return false; }
int codecvt<wchar_t, char, mbstate_t>::do_length(mbstate_t&, const char* from, const char* end, size_t max) const {
  return utf8_length(from, end, max, 1);
}
int codecvt<wchar_t, char, mbstate_t>::do_max_length() const noexcept { return 4; }

// codecvt<char32_t, char8_t, mbstate_t>: UTF-32 <-> UTF-8.
locale::id codecvt<char32_t, char8_t, mbstate_t>::id;
codecvt<char32_t, char8_t, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<char32_t, char8_t, mbstate_t>::do_out(mbstate_t&, const char32_t* from,
                                                                   const char32_t* __from_end, const char32_t*& __from_next,
                                                                   char8_t* to, char8_t* __to_end,
                                                                   char8_t*& __to_next) const {
  return utf32_out(from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char32_t, char8_t, mbstate_t>::do_in(mbstate_t&, const char8_t* from,
                                                                  const char8_t* __from_end, const char8_t*& __from_next,
                                                                  char32_t* to, char32_t* __to_end,
                                                                  char32_t*& __to_next) const {
  return utf32_in(from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char32_t, char8_t, mbstate_t>::do_unshift(mbstate_t&, char8_t* to, char8_t*,
                                                                       char8_t*& __to_next) const {
  __to_next = to;
  return noconv;
}
int codecvt<char32_t, char8_t, mbstate_t>::do_encoding() const noexcept { return 0; }
bool codecvt<char32_t, char8_t, mbstate_t>::do_always_noconv() const noexcept { return false; }
int codecvt<char32_t, char8_t, mbstate_t>::do_length(mbstate_t&, const char8_t* from, const char8_t* end,
                                                     size_t max) const {
  return utf8_length(from, end, max, 1);
}
int codecvt<char32_t, char8_t, mbstate_t>::do_max_length() const noexcept { return 4; }

// codecvt<char16_t, char8_t, mbstate_t>: UTF-16 <-> UTF-8 (utf16_out / utf16_in).
locale::id codecvt<char16_t, char8_t, mbstate_t>::id;
codecvt<char16_t, char8_t, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_out(mbstate_t& state, const char16_t* from,
                                                                   const char16_t* __from_end, const char16_t*& __from_next,
                                                                   char8_t* to, char8_t* __to_end,
                                                                   char8_t*& __to_next) const {
  return utf16_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_in(mbstate_t& state, const char8_t* from,
                                                                  const char8_t* __from_end, const char8_t*& __from_next,
                                                                  char16_t* to, char16_t* __to_end,
                                                                  char16_t*& __to_next) const {
  return utf16_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_unshift(mbstate_t& state, char8_t* to, char8_t*,
                                                                       char8_t*& __to_next) const {
  return utf16_unshift(state, to, __to_next);
}
int codecvt<char16_t, char8_t, mbstate_t>::do_encoding() const noexcept { return 0; }
bool codecvt<char16_t, char8_t, mbstate_t>::do_always_noconv() const noexcept { return false; }
int codecvt<char16_t, char8_t, mbstate_t>::do_length(mbstate_t&, const char8_t* from, const char8_t* end,
                                                     size_t max) const {
  return utf8_length(from, end, max, 2);
}
int codecvt<char16_t, char8_t, mbstate_t>::do_max_length() const noexcept { return 4; }

// [depr.locale.category]: the deprecated codecvt<char16_t, char, mbstate_t> and
// codecvt<char32_t, char, mbstate_t>, the same conversions with char as the UTF-8 code unit.
// codecvt<char32_t, char, mbstate_t>: UTF-32 <-> UTF-8.
locale::id codecvt<char32_t, char, mbstate_t>::id;
codecvt<char32_t, char, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<char32_t, char, mbstate_t>::do_out(mbstate_t&, const char32_t* from,
                                                                   const char32_t* __from_end, const char32_t*& __from_next,
                                                                   char* to, char* __to_end,
                                                                   char*& __to_next) const {
  return utf32_out(from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char32_t, char, mbstate_t>::do_in(mbstate_t&, const char* from,
                                                                  const char* __from_end, const char*& __from_next,
                                                                  char32_t* to, char32_t* __to_end,
                                                                  char32_t*& __to_next) const {
  return utf32_in(from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char32_t, char, mbstate_t>::do_unshift(mbstate_t&, char* to, char*,
                                                                       char*& __to_next) const {
  __to_next = to;
  return noconv;
}
int codecvt<char32_t, char, mbstate_t>::do_encoding() const noexcept { return 0; }
bool codecvt<char32_t, char, mbstate_t>::do_always_noconv() const noexcept { return false; }
int codecvt<char32_t, char, mbstate_t>::do_length(mbstate_t&, const char* from, const char* end,
                                                     size_t max) const {
  return utf8_length(from, end, max, 1);
}
int codecvt<char32_t, char, mbstate_t>::do_max_length() const noexcept { return 4; }

// codecvt<char16_t, char, mbstate_t>: UTF-16 <-> UTF-8 (utf16_out / utf16_in).
locale::id codecvt<char16_t, char, mbstate_t>::id;
codecvt<char16_t, char, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<char16_t, char, mbstate_t>::do_out(mbstate_t& state, const char16_t* from,
                                                                   const char16_t* __from_end, const char16_t*& __from_next,
                                                                   char* to, char* __to_end,
                                                                   char*& __to_next) const {
  return utf16_out(state, from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char16_t, char, mbstate_t>::do_in(mbstate_t& state, const char* from,
                                                                  const char* __from_end, const char*& __from_next,
                                                                  char16_t* to, char16_t* __to_end,
                                                                  char16_t*& __to_next) const {
  return utf16_in(state, from, __from_end, __from_next, to, __to_end, __to_next);
}
codecvt_base::result codecvt<char16_t, char, mbstate_t>::do_unshift(mbstate_t& state, char* to, char*,
                                                                       char*& __to_next) const {
  return utf16_unshift(state, to, __to_next);
}
int codecvt<char16_t, char, mbstate_t>::do_encoding() const noexcept { return 0; }
bool codecvt<char16_t, char, mbstate_t>::do_always_noconv() const noexcept { return false; }
int codecvt<char16_t, char, mbstate_t>::do_length(mbstate_t&, const char* from, const char* end,
                                                     size_t max) const {
  return utf8_length(from, end, max, 2);
}
int codecvt<char16_t, char, mbstate_t>::do_max_length() const noexcept { return 4; }

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

void __check_locale_name(const char* name, const char* what) {
  split_name __parts;
  if (!resolve_parts(name, __parts))
    bad_name(what, name);
  for (int c = 0; c < ncategories; ++c)
    if (__classic_locale_name(__parts.part[c].c_str()) == nullptr && !__named_exists(__parts.part[c].c_str(), c))
      bad_name(what, __parts.part[c].c_str());
}

const char* __classic_locale_name(const char* name) noexcept {
  if (std::strcmp(name, "C") == 0 || std::strcmp(name, "POSIX") == 0)
    return "C";
  if (std::strcmp(name, "C.UTF-8") == 0 || std::strcmp(name, "C.utf8") == 0)
    return name;
  return nullptr;
}

bool __locale_name_part(const char* name, int c, std::string& out) {
  split_name __parts;
  if (!resolve_parts(name, __parts))
    return false;
  out = __parts.part[c];
  return true;
}

}} // namespace __ycxx::__detail
