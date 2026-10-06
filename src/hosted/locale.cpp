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
#include <ycxx/hosted/memory_resource.hpp> // ycxx::detail::pal_lock
#include "locale_named.hpp"

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct locale_impl {
  std::size_t refs; // atomic
  const std::locale::facet** facets;
  std::size_t nfacets;
  char* name; // null: unnamed
};

struct locale_access {
  static void retain(const std::locale::facet* f) noexcept { ::ycxx::detail::ref_add(f->refs_); }
  static void release(const std::locale::facet* f) noexcept {
    if (::ycxx::detail::ref_release(f->refs_))
      delete f;
  }
  static std::size_t index(const std::locale::id& i) noexcept { return i.index(); }
  static locale_impl* impl(const std::locale& l) noexcept { return l.impl_; }
  static std::locale make(locale_impl* p) noexcept { return std::locale(locale_impl_tag(), p); }
};

}} // namespace ycxx::detail

namespace {

using ycxx::detail::locale_access;
using ycxx::detail::locale_impl;

std::size_t next_facet_index = 0; // atomic; ids are 1, 2, ...

// Zero-filled arrays from ::operator new (not new[]: the program's array allocation functions
// see no calls from the locale machinery).
template <class T>
T* new_zeroed(std::size_t n) {
  void* p = ::operator new(n * sizeof(T));
  std::memset(p, 0, n * sizeof(T));
  return static_cast<T*>(p);
}

char* copy_name(const char* s) {
  if (s == nullptr)
    return nullptr;
  const std::size_t n = std::strlen(s) + 1;
  char* p = new_zeroed<char>(n);
  std::memcpy(p, s, n);
  return p;
}

locale_impl* new_impl(std::size_t nfacets, const char* name) {
  locale_impl* p = new locale_impl{1, nullptr, 0, nullptr};
  if constexpr (ycxx::detail::cfg::exceptions) {
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

void retain(locale_impl* p) noexcept { ::ycxx::detail::ref_add(p->refs); }
void release(locale_impl* p) noexcept {
  if (!::ycxx::detail::ref_release(p->refs))
    return;
  for (std::size_t i = 0; i < p->nfacets; ++i)
    if (p->facets[i] != nullptr)
      locale_access::release(p->facets[i]);
  ::operator delete(p->facets);
  ::operator delete(p->name);
  delete p;
}

// A copy of src's facets in a new impl with room for index `need`.
locale_impl* clone(const locale_impl* src, std::size_t need, const char* name) {
  std::size_t n = src->nfacets;
  if (need >= n)
    n = need + 1;
  locale_impl* p = new_impl(n, name);
  for (std::size_t i = 0; i < src->nfacets; ++i) {
    p->facets[i] = src->facets[i];
    if (p->facets[i] != nullptr)
      locale_access::retain(p->facets[i]);
  }
  return p;
}

void set_facet(locale_impl* p, std::size_t i, const std::locale::facet* f) noexcept {
  if (f != nullptr)
    locale_access::retain(f);
  if (p->facets[i] != nullptr)
    locale_access::release(p->facets[i]);
  p->facets[i] = f;
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
  bool seen[ncategories] = {};
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
    seen[c] = true;
    s = *end ? end + 1 : end;
  }
  for (bool b : seen)
    if (!b)
      return false;
  return true;
}
std::string join(const split_name& n) {
  bool same = true;
  for (int c = 1; c < ncategories; ++c)
    same = same && n.part[c] == n.part[0];
  if (same)
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
  const char* v = std::getenv("LC_ALL");
  if (v == nullptr || *v == '\0')
    v = std::getenv(category_names[c]);
  if (v == nullptr || *v == '\0')
    v = std::getenv("LANG");
  if (v == nullptr || *v == '\0' || std::strchr(v, '=') != nullptr)
    return "C";
  if (const char* n = ycxx::detail::classic_locale_name(v))
    return n;
  return ycxx::detail::named_exists(v, c) ? std::string(v) : std::string("C");
}

// Splits a std_name into its names per category (the environment's for "" and for an empty
// part; "POSIX" is "C"); false if malformed. Whether the C library has them is found when the
// facets are built.
bool resolve_parts(const char* name, split_name& parts) {
  if (name == nullptr)
    return false;
  if (*name == '\0') {
    for (int c = 0; c < ncategories; ++c)
      parts.part[c] = environment_name(c);
    return true;
  }
  if (!split(name, parts))
    return false;
  for (int c = 0; c < ncategories; ++c) {
    if (parts.part[c].empty())
      parts.part[c] = environment_name(c);
    else if (const char* n = ycxx::detail::classic_locale_name(parts.part[c].c_str()))
      parts.part[c] = n;
  }
  return true;
}

[[noreturn]] void bad_name(const char* what, const char* name) {
  std::string msg(what);
  msg += ": unsupported locale name \"";
  if (name != nullptr)
    msg += name;
  msg += '"';
  ::ycxx::detail::throw_runtime_error(msg.c_str());
}

// ---- the classic locale ------------------------------------------------------------------------

template <class T>
union immortal {
  T object;
  immortal() {}
  ~immortal() {}
};

// The classic locale's facets and tables live in static storage, never freed: no allocation
// function is called for them, so a program that replaces operator new and counts what is
// outstanding does not count them.
template <class F, class... A>
F* in_static(A... a) {
  alignas(F) static unsigned char storage[sizeof(F)];
  return ::new (static_cast<void*>(storage)) F(a...);
}

locale_impl* make_classic() {
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
    const std::size_t k = locale_access::index(*i);
    if (k > top)
      top = k;
  }
  // in static storage too while the ids fit (they do unless a program assigns many before the
  // classic locale is first used)
  static const std::locale::facet* table[64];
  static char name[] = "C";
  static locale_impl impl{1, table, 64, name};
  locale_impl* p = top + 1 <= 64 ? &impl : new_impl(top + 1, "C");
  for (std::size_t k = 0; k < sizeof ids / sizeof ids[0]; ++k)
    set_facet(p, locale_access::index(*ids[k]), facets[k]);
  return p;
}

const std::locale& classic_locale() {
  static immortal<std::locale> c;
  static const bool built = (::new (static_cast<void*>(__builtin_addressof(c.object)))
                                 std::locale(locale_access::make(make_classic())),
                             true);
  (void)built;
  return c.object;
}

// The classic locale is built before any object of the program with ordinary static
// initialization (it is also built on first use, should a runtime initializer need it sooner),
// so the program never sees its allocations as its own. Mach-O has no initialization priorities
// (YCXX_HAS_INIT_PRIORITY): there it is built before main, by the first ios_base::Init (every
// translation unit including <iostream> has one) or by this object in link order, or on first
// use by an earlier static initializer of the program.
struct build_classic {
  build_classic() { classic_locale(); }
};
#if YCXX_HAS_INIT_PRIORITY
[[gnu::init_priority(100)]] build_classic classic_at_startup;
#else
build_classic classic_at_startup;
#endif

// ---- the facets of a named category -------------------------------------------------------------

// Builds the facets of category c for name (a single name without classic semantics), in the
// order of ids_of(c); null where the classic facet is kept. All or nothing: throws
// runtime_error (naming what) if the C library has no such locale.
void build_named(int c, const char* name, const char* what, const std::locale::facet** out) {
  // held while the facets are built, so they share one opening of the C library's locale
  ycxx::detail::named_locale* h = ycxx::detail::named_open(name, category_bits[c], what);
  struct held {
    ycxx::detail::named_locale* h;
    const std::locale::facet** out;
    int n;
    ~held() {
      ycxx::detail::named_release(h);
      for (int k = 0; k < n; ++k) // facets built before an exception (n is 0 on success)
        if (out[k] != nullptr) {
          locale_access::retain(out[k]);
          locale_access::release(out[k]);
        }
    }
  } guard{h, out, 0};
  const int n = ids_of(c).n;
  for (int k = 0; k < n; ++k)
    out[k] = nullptr;
  guard.n = n;
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
  guard.n = 0;
}

// Installs in p the facets of category c that implement `name` (one name): the classic ones,
// or those built by build_named.
void install_category(locale_impl* p, int c, const char* name, const char* what) {
  const locale_impl* one = locale_access::impl(classic_locale());
  const category_ids& ids = ids_of(c);
  const std::locale::facet* built[12] = {};
  if (ycxx::detail::classic_locale_name(name) == nullptr)
    build_named(c, name, what, built);
  for (int k = 0; k < ids.n; ++k) {
    const std::size_t i = locale_access::index(*ids.ids[k]);
    set_facet(p, i, built[k] != nullptr ? built[k] : (i < one->nfacets ? one->facets[i] : nullptr));
  }
}

// Installs the categories cats of parts in p; on an exception p is released.
void install_parts(locale_impl* p, const split_name& parts, std::locale::category cats) {
  if constexpr (ycxx::detail::cfg::exceptions) {
    try {
      for (int c = 0; c < ncategories; ++c)
        if (cats & category_bits[c])
          install_category(p, c, parts.part[c].c_str(), "std::locale");
    } catch (...) {
      release(p);
      throw;
    }
  } else {
    for (int c = 0; c < ncategories; ++c)
      if (cats & category_bits[c])
        install_category(p, c, parts.part[c].c_str(), "std::locale");
  }
}

// The global locale ([locale.statics]): one for the program, under a lock.
ycxx::detail::pal_lock global_lock;
locale_impl* global_impl = nullptr; // null: the classic locale

struct lock_guard {
  explicit lock_guard(ycxx::detail::pal_lock& l) noexcept : l_(l) { l_.lock(); }
  ~lock_guard() { l_.unlock(); }
  lock_guard(const lock_guard&) = delete;
  ycxx::detail::pal_lock& l_;
};

} // namespace

namespace [[gnu::visibility("hidden")]] std {

locale::facet::~facet() {}

size_t locale::id::assign() const noexcept {
  const size_t mine = __atomic_add_fetch(&next_facet_index, 1, __ATOMIC_RELAXED);
  size_t expected = 0;
  if (__atomic_compare_exchange_n(&index_, &expected, mine, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    return mine;
  return expected;
}

locale::locale() noexcept {
  const locale& c = classic_locale();
  lock_guard g(global_lock);
  impl_ = global_impl != nullptr ? global_impl : c.impl_;
  retain(impl_);
}

locale::locale(const locale& other) noexcept : impl_(other.impl_) { retain(impl_); }

locale::locale(const char* std_name) : impl_(nullptr) {
  split_name parts;
  if (!resolve_parts(std_name, parts))
    bad_name("std::locale", std_name);
  const string name = join(parts);
  locale_impl* p = clone(classic_locale().impl_, 0, name.c_str());
  install_parts(p, parts, all);
  impl_ = p;
}

locale::locale(const locale& other, const char* std_name, category cats) : impl_(nullptr) {
  split_name b;
  if (!resolve_parts(std_name, b))
    bad_name("std::locale", std_name);
  const locale_impl* one = classic_locale().impl_;
  string result;
  if (other.impl_->name != nullptr) {
    split_name a;
    split(other.impl_->name, a);
    for (int c = 0; c < ncategories; ++c)
      if (cats & category_bits[c])
        a.part[c] = b.part[c];
    result = join(a);
  }
  locale_impl* p = clone(other.impl_, one->nfacets, other.impl_->name != nullptr ? result.c_str() : nullptr);
  install_parts(p, b, cats);
  impl_ = p;
}

locale::locale(const locale& other, const locale& one, category cats) : impl_(nullptr) {
  string result;
  // [locale.cons]/15 (LWG 3676): with cats none, named iff other is; otherwise iff both are
  const bool named = other.impl_->name != nullptr && (cats == none || one.impl_->name != nullptr);
  if (named) {
    split_name a, b;
    split(other.impl_->name, a);
    if (cats != none) {
      split(one.impl_->name, b);
      for (int c = 0; c < ncategories; ++c)
        if (cats & category_bits[c])
          a.part[c] = b.part[c];
    }
    result = join(a);
  }
  locale_impl* p = clone(other.impl_, one.impl_->nfacets, named ? result.c_str() : nullptr);
  for (int c = 0; c < ncategories; ++c) {
    if (!(cats & category_bits[c]))
      continue;
    const category_ids& ids = ids_of(c);
    for (int k = 0; k < ids.n; ++k) {
      const size_t i = locale_access::index(*ids.ids[k]);
      set_facet(p, i, i < one.impl_->nfacets ? one.impl_->facets[i] : nullptr);
    }
  }
  impl_ = p;
}

locale::locale(const locale& other, const facet* f, const id& i) : impl_(nullptr) {
  if (f == nullptr) {
    impl_ = other.impl_;
    retain(impl_);
    return;
  }
  const size_t k = i.index();
  locale_impl* p = clone(other.impl_, k, nullptr);
  set_facet(p, k, f);
  impl_ = p;
}

locale::~locale() { release(impl_); }

const locale& locale::operator=(const locale& other) noexcept {
  retain(other.impl_);
  release(impl_);
  impl_ = other.impl_;
  return *this;
}

string locale::name() const { return impl_->name != nullptr ? string(impl_->name) : string("*"); }

bool locale::operator==(const locale& other) const {
  if (impl_ == other.impl_)
    return true;
  return impl_->name != nullptr && other.impl_->name != nullptr && std::strcmp(impl_->name, other.impl_->name) == 0;
}

const locale::facet* locale::find(const id& i) const noexcept {
  const size_t k = i.index();
  return k < impl_->nfacets ? impl_->facets[k] : nullptr;
}

locale locale::global(const locale& loc) {
  const locale& c = classic_locale();
  locale_impl* old;
  split_name parts; // before the reference is taken: split may throw bad_alloc
  const bool named = loc.impl_->name != nullptr;
  if (named)
    split(loc.impl_->name, parts);
  retain(loc.impl_);
  {
    // setlocale under the same lock, so that concurrent calls leave the C library's locale and
    // the global locale set by the same call
    lock_guard g(global_lock);
    old = global_impl;
    global_impl = loc.impl_;
    if (named) {
      bool same = true;
      for (int k = 1; k < ncategories; ++k)
        same = same && parts.part[k] == parts.part[0];
      if (same) {
        ::setlocale(LC_ALL, parts.part[0].c_str());
      } else {
        for (int k = 0; k < ncategories; ++k)
          ::setlocale(c_categories[k], parts.part[k].c_str());
      }
    }
  }
  if (old == nullptr) { // the classic locale, whose reference the global did not hold
    old = c.impl_;
    retain(old);
  }
  return locale_access::make(old); // adopts the global's reference
}

const locale& locale::classic() { return classic_locale(); }

// ---- ctype<char> ------------------------------------------------------------------------------

locale::id ctype<char>::id;

const ctype_base::mask* ctype<char>::classic_table() noexcept {
  static constexpr struct table {
    mask m[table_size];
    constexpr table() : m() {
      for (int c = 0; c < 128; ++c)
        m[c] = ::ycxx::detail::ascii_masks.m[c];
    }
  } t;
  return t.m;
}

ctype<char>::~ctype() {
  if (del_)
    delete[] table_;
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
char32_t load_state(const std::mbstate_t& st) noexcept {
  char32_t v;
  __builtin_memcpy(&v, &st, sizeof v);
  return v;
}
void store_state(std::mbstate_t& st, char32_t v) noexcept { __builtin_memcpy(&st, &v, sizeof v); }

// do_unshift of the UTF-16 facets. A high surrogate taken by utf16_out and still waiting for its
// low half cannot be terminated: alone it is not a character UTF-8 can encode (Table 94: error,
// rather than noconv, which would drop it silently). A low surrogate pending from utf16_in
// belongs to the other direction and needs no termination.
template <class E>
result utf16_unshift(const std::mbstate_t& state, E* to, E*& to_next) noexcept {
  to_next = to;
  const char32_t pending = load_state(state);
  return pending >= 0xD800 && pending <= 0xDBFF ? std::codecvt_base::error : std::codecvt_base::noconv;
}

// Decodes one UTF-8 sequence at [p, end): returns its length (0: incomplete, -1: invalid).
int decode_utf8(const unsigned char* p, const unsigned char* end, char32_t& cp) noexcept {
  const unsigned char b = p[0];
  int len;
  char32_t v;
  if (b < 0x80) {
    cp = b;
    return 1;
  }
  if (b >= 0xC2 && b <= 0xDF) {
    len = 2;
    v = b & 0x1F;
  } else if (b >= 0xE0 && b <= 0xEF) {
    len = 3;
    v = b & 0x0F;
  } else if (b >= 0xF0 && b <= 0xF4) {
    len = 4;
    v = b & 0x07;
  } else {
    return -1;
  }
  for (int i = 1; i < len; ++i) {
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
    v = (v << 6) | (c & 0x3F);
  }
  cp = v;
  return len;
}

int encode_utf8(char32_t cp, unsigned char* out) noexcept {
  if (cp < 0x80) {
    out[0] = static_cast<unsigned char>(cp);
    return 1;
  }
  if (cp < 0x800) {
    out[0] = static_cast<unsigned char>(0xC0 | (cp >> 6));
    out[1] = static_cast<unsigned char>(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    out[0] = static_cast<unsigned char>(0xE0 | (cp >> 12));
    out[1] = static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F));
    out[2] = static_cast<unsigned char>(0x80 | (cp & 0x3F));
    return 3;
  }
  out[0] = static_cast<unsigned char>(0xF0 | (cp >> 18));
  out[1] = static_cast<unsigned char>(0x80 | ((cp >> 12) & 0x3F));
  out[2] = static_cast<unsigned char>(0x80 | ((cp >> 6) & 0x3F));
  out[3] = static_cast<unsigned char>(0x80 | (cp & 0x3F));
  return 4;
}

bool valid_scalar(char32_t cp) noexcept { return cp < 0xD800 || (cp > 0xDFFF && cp <= 0x10FFFF); }

// UTF-32 (I = wchar_t or char32_t) to UTF-8 (E = char or char8_t).
template <class I, class E>
result utf32_out(const I* from, const I* from_end, const I*& from_next, E* to, E* to_end, E*& to_next) {
  for (; from != from_end; ++from) {
    const char32_t cp = static_cast<char32_t>(*from);
    if (!valid_scalar(cp)) {
      from_next = from;
      to_next = to;
      return std::codecvt_base::error;
    }
    unsigned char buf[4];
    const int n = encode_utf8(cp, buf);
    if (to_end - to < n) {
      from_next = from;
      to_next = to;
      return std::codecvt_base::partial;
    }
    for (int i = 0; i < n; ++i)
      *to++ = static_cast<E>(buf[i]);
  }
  from_next = from;
  to_next = to;
  return std::codecvt_base::ok;
}

template <class I, class E>
result utf32_in(const E* from, const E* from_end, const E*& from_next, I* to, I* to_end, I*& to_next) {
  while (from != from_end) {
    if (to == to_end) {
      from_next = from;
      to_next = to;
      return std::codecvt_base::partial;
    }
    char32_t cp;
    const int n = decode_utf8(reinterpret_cast<const unsigned char*>(from), reinterpret_cast<const unsigned char*>(from_end), cp);
    if (n <= 0) {
      from_next = from;
      to_next = to;
      return n == 0 ? std::codecvt_base::partial : std::codecvt_base::error;
    }
    *to++ = static_cast<I>(cp);
    from += n;
  }
  from_next = from;
  to_next = to;
  return std::codecvt_base::ok;
}

// UTF-16 to UTF-8 (E = char or char8_t). A supplementary character is split across calls through
// the state ([locale.codecvt.virtuals]/4): a high surrogate is consumed alone and kept.
template <class E>
result utf16_out(std::mbstate_t& state, const char16_t* from, const char16_t* from_end, const char16_t*& from_next,
                 E* to, E* to_end, E*& to_next) {
  char32_t high = load_state(state);
  // where the pending high surrogate was taken: the start when it came in through the state
  const char16_t* high_at = from;
  for (; from != from_end; ++from) {
    const char16_t u = *from;
    char32_t cp;
    if (high != 0) {
      if (u < 0xDC00 || u > 0xDFFF) {
        // the unpaired high surrogate is the character that cannot be converted
        // ([locale.codecvt.virtuals]/3: from_next is one beyond the last converted element)
        from_next = high_at;
        to_next = to;
        return std::codecvt_base::error;
      }
      cp = 0x10000 + ((high - 0xD800) << 10) + (u - 0xDC00);
    } else if (u >= 0xD800 && u <= 0xDBFF) {
      high = u;
      high_at = from;
      store_state(state, high);
      continue;
    } else if (u >= 0xDC00 && u <= 0xDFFF) {
      from_next = from;
      to_next = to;
      return std::codecvt_base::error;
    } else {
      cp = u;
    }
    unsigned char buf[4];
    const int n = encode_utf8(cp, buf);
    if (to_end - to < n) {
      // the pending high surrogate (if any) stays in the state; u is not consumed
      from_next = from;
      to_next = to;
      return std::codecvt_base::partial;
    }
    for (int i = 0; i < n; ++i)
      *to++ = static_cast<E>(buf[i]);
    high = 0;
    store_state(state, 0);
  }
  from_next = from;
  to_next = to;
  return std::codecvt_base::ok;
}

// UTF-8 to UTF-16: with room for one unit only, the high surrogate is written and the low one
// kept in the state; the UTF-8 sequence is consumed when the low one is written.
template <class E>
result utf16_in(std::mbstate_t& state, const E* from, const E* from_end, const E*& from_next, char16_t* to,
                char16_t* to_end, char16_t*& to_next) {
  while (from != from_end) {
    if (to == to_end) {
      from_next = from;
      to_next = to;
      return std::codecvt_base::partial;
    }
    char32_t cp;
    const int n =
        decode_utf8(reinterpret_cast<const unsigned char*>(from), reinterpret_cast<const unsigned char*>(from_end), cp);
    if (n <= 0) {
      from_next = from;
      to_next = to;
      return n == 0 ? std::codecvt_base::partial : std::codecvt_base::error;
    }
    if (cp < 0x10000) {
      *to++ = static_cast<char16_t>(cp);
      from += n;
      continue;
    }
    const char16_t hi = static_cast<char16_t>(0xD800 + ((cp - 0x10000) >> 10));
    const char16_t lo = static_cast<char16_t>(0xDC00 + ((cp - 0x10000) & 0x3FF));
    if (load_state(state) != 0) {
      // the high surrogate was written by the previous call
      *to++ = lo;
      store_state(state, 0);
      from += n;
      continue;
    }
    *to++ = hi;
    if (to == to_end) {
      store_state(state, lo);
      from_next = from;
      to_next = to;
      return std::codecvt_base::ok;
    }
    *to++ = lo;
    from += n;
  }
  from_next = from;
  to_next = to;
  return std::codecvt_base::ok;
}

template <class E>
int utf8_length(const E* from, const E* end, std::size_t max, int units_per_cp_max) {
  const E* p = from;
  std::size_t produced = 0;
  while (p != end && produced < max) {
    char32_t cp;
    const int n = decode_utf8(reinterpret_cast<const unsigned char*>(p), reinterpret_cast<const unsigned char*>(end), cp);
    if (n <= 0)
      break;
    const std::size_t units = (units_per_cp_max == 2 && cp >= 0x10000) ? 2 : 1;
    if (produced + units > max)
      break;
    produced += units;
    p += n;
  }
  return static_cast<int>(p - from);
}

} // namespace

namespace [[gnu::visibility("hidden")]] std {

// codecvt<char, char, mbstate_t>: the degenerate conversion.
locale::id codecvt<char, char, mbstate_t>::id;
codecvt<char, char, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<char, char, mbstate_t>::do_out(mbstate_t&, const char* from, const char*,
                                                            const char*& from_next, char* to, char*,
                                                            char*& to_next) const {
  from_next = from;
  to_next = to;
  return noconv;
}
codecvt_base::result codecvt<char, char, mbstate_t>::do_in(mbstate_t&, const char* from, const char*,
                                                           const char*& from_next, char* to, char*,
                                                           char*& to_next) const {
  from_next = from;
  to_next = to;
  return noconv;
}
codecvt_base::result codecvt<char, char, mbstate_t>::do_unshift(mbstate_t&, char* to, char*, char*& to_next) const {
  to_next = to;
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
codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_out(mbstate_t&, const wchar_t* from, const wchar_t* from_end,
                                                               const wchar_t*& from_next, char* to, char* to_end,
                                                               char*& to_next) const {
  return utf32_out(from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_in(mbstate_t&, const char* from, const char* from_end,
                                                              const char*& from_next, wchar_t* to, wchar_t* to_end,
                                                              wchar_t*& to_next) const {
  return utf32_in(from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<wchar_t, char, mbstate_t>::do_unshift(mbstate_t&, char* to, char*, char*& to_next) const {
  to_next = to;
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
                                                                   const char32_t* from_end, const char32_t*& from_next,
                                                                   char8_t* to, char8_t* to_end,
                                                                   char8_t*& to_next) const {
  return utf32_out(from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char32_t, char8_t, mbstate_t>::do_in(mbstate_t&, const char8_t* from,
                                                                  const char8_t* from_end, const char8_t*& from_next,
                                                                  char32_t* to, char32_t* to_end,
                                                                  char32_t*& to_next) const {
  return utf32_in(from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char32_t, char8_t, mbstate_t>::do_unshift(mbstate_t&, char8_t* to, char8_t*,
                                                                       char8_t*& to_next) const {
  to_next = to;
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
                                                                   const char16_t* from_end, const char16_t*& from_next,
                                                                   char8_t* to, char8_t* to_end,
                                                                   char8_t*& to_next) const {
  return utf16_out(state, from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_in(mbstate_t& state, const char8_t* from,
                                                                  const char8_t* from_end, const char8_t*& from_next,
                                                                  char16_t* to, char16_t* to_end,
                                                                  char16_t*& to_next) const {
  return utf16_in(state, from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_unshift(mbstate_t& state, char8_t* to, char8_t*,
                                                                       char8_t*& to_next) const {
  return utf16_unshift(state, to, to_next);
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
                                                                   const char32_t* from_end, const char32_t*& from_next,
                                                                   char* to, char* to_end,
                                                                   char*& to_next) const {
  return utf32_out(from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char32_t, char, mbstate_t>::do_in(mbstate_t&, const char* from,
                                                                  const char* from_end, const char*& from_next,
                                                                  char32_t* to, char32_t* to_end,
                                                                  char32_t*& to_next) const {
  return utf32_in(from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char32_t, char, mbstate_t>::do_unshift(mbstate_t&, char* to, char*,
                                                                       char*& to_next) const {
  to_next = to;
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
                                                                   const char16_t* from_end, const char16_t*& from_next,
                                                                   char* to, char* to_end,
                                                                   char*& to_next) const {
  return utf16_out(state, from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char16_t, char, mbstate_t>::do_in(mbstate_t& state, const char* from,
                                                                  const char* from_end, const char*& from_next,
                                                                  char16_t* to, char16_t* to_end,
                                                                  char16_t*& to_next) const {
  return utf16_in(state, from, from_end, from_next, to, to_end, to_next);
}
codecvt_base::result codecvt<char16_t, char, mbstate_t>::do_unshift(mbstate_t& state, char* to, char*,
                                                                       char*& to_next) const {
  return utf16_unshift(state, to, to_next);
}
int codecvt<char16_t, char, mbstate_t>::do_encoding() const noexcept { return 0; }
bool codecvt<char16_t, char, mbstate_t>::do_always_noconv() const noexcept { return false; }
int codecvt<char16_t, char, mbstate_t>::do_length(mbstate_t&, const char* from, const char* end,
                                                     size_t max) const {
  return utf8_length(from, end, max, 2);
}
int codecvt<char16_t, char, mbstate_t>::do_max_length() const noexcept { return 4; }

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

void check_locale_name(const char* name, const char* what) {
  split_name parts;
  if (!resolve_parts(name, parts))
    bad_name(what, name);
  for (int c = 0; c < ncategories; ++c)
    if (classic_locale_name(parts.part[c].c_str()) == nullptr && !named_exists(parts.part[c].c_str(), c))
      bad_name(what, parts.part[c].c_str());
}

const char* classic_locale_name(const char* name) noexcept {
  if (std::strcmp(name, "C") == 0 || std::strcmp(name, "POSIX") == 0)
    return "C";
  if (std::strcmp(name, "C.UTF-8") == 0 || std::strcmp(name, "C.utf8") == 0)
    return name;
  return nullptr;
}

bool locale_name_part(const char* name, int c, std::string& out) {
  split_name parts;
  if (!resolve_parts(name, parts))
    return false;
  out = parts.part[c];
  return true;
}

}} // namespace ycxx::detail
