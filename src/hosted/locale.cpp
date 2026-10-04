// libycxx hosted runtime: the locale machinery ([locales]) and the facets with out-of-line
// members: ctype<char>'s table, the four required codecvt specializations.
//
// A locale_impl holds an array of facet pointers indexed by locale::id and the locale's name
// (null when unnamed). Every named locale has the classic facets (the names supported are the
// "C" ones, see ycxx/hosted/locale_base.hpp); a name is either one name for all categories or
// a composite "LC_COLLATE=...;LC_CTYPE=...;LC_MONETARY=...;LC_NUMERIC=...;LC_TIME=...;
// LC_MESSAGES=..." when the categories were taken from different names.
#include <locale>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <new>
#include <ycxx/hosted/memory_resource.hpp> // ycxx::detail::pal_lock

namespace ycxx::detail {

struct locale_impl {
  std::size_t refs; // atomic
  const std::locale::facet** facets;
  std::size_t nfacets;
  char* name; // null: unnamed
};

struct locale_access {
  static void retain(const std::locale::facet* f) noexcept { __atomic_add_fetch(&f->refs_, 1, __ATOMIC_RELAXED); }
  static void release(const std::locale::facet* f) noexcept {
    if (__atomic_sub_fetch(&f->refs_, 1, __ATOMIC_ACQ_REL) == 0)
      delete f;
  }
  static std::size_t index(const std::locale::id& i) noexcept { return i.index(); }
  static locale_impl* impl(const std::locale& l) noexcept { return l.impl_; }
  static std::locale make(locale_impl* p) noexcept { return std::locale(p); }
};

} // namespace ycxx::detail

namespace {

using ycxx::detail::locale_access;
using ycxx::detail::locale_impl;

std::size_t next_facet_index = 0; // atomic; ids are 1, 2, ...

char* copy_name(const char* s) {
  if (s == nullptr)
    return nullptr;
  const std::size_t n = std::strlen(s) + 1;
  char* p = new char[n];
  std::memcpy(p, s, n);
  return p;
}

locale_impl* new_impl(std::size_t nfacets, const char* name) {
  locale_impl* p = new locale_impl{1, nullptr, 0, nullptr};
  if constexpr (ycxx::detail::cfg::exceptions) {
    try {
      p->facets = new const std::locale::facet*[nfacets]();
      p->nfacets = nfacets;
      p->name = copy_name(name);
    } catch (...) {
      delete[] p->facets;
      delete p;
      throw;
    }
  } else {
    p->facets = new const std::locale::facet*[nfacets]();
    p->nfacets = nfacets;
    p->name = copy_name(name);
  }
  return p;
}

void retain(locale_impl* p) noexcept { __atomic_add_fetch(&p->refs, 1, __ATOMIC_RELAXED); }
void release(locale_impl* p) noexcept {
  if (__atomic_sub_fetch(&p->refs, 1, __ATOMIC_ACQ_REL) != 0)
    return;
  for (std::size_t i = 0; i < p->nfacets; ++i)
    if (p->facets[i] != nullptr)
      locale_access::release(p->facets[i]);
  delete[] p->facets;
  delete[] p->name;
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
        &std::codecvt<char32_t, char8_t, std::mbstate_t>::id},
       6},
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

// A supported single locale name, normalized ("POSIX" is "C"); null if unsupported.
const char* normalize_simple(const char* name) {
  if (std::strcmp(name, "C") == 0 || std::strcmp(name, "POSIX") == 0)
    return "C";
  if (std::strcmp(name, "C.UTF-8") == 0 || std::strcmp(name, "C.utf8") == 0)
    return name;
  return nullptr;
}

// The environment's name for category c ([locale.cons]/4: ""), falling back to "C" for names
// whose conventions are not supported.
const char* environment_name(int c) {
  const char* v = std::getenv("LC_ALL");
  if (v == nullptr || *v == '\0')
    v = std::getenv(category_names[c]);
  if (v == nullptr || *v == '\0')
    v = std::getenv("LANG");
  if (v == nullptr || *v == '\0')
    return "C";
  const char* n = normalize_simple(v);
  return n != nullptr ? n : "C";
}

// Resolves a std_name to its normalized (possibly composite) form; false if not supported.
bool resolve_name(const char* name, std::string& out) {
  if (name == nullptr)
    return false;
  split_name parts;
  if (*name == '\0') {
    for (int c = 0; c < ncategories; ++c)
      parts.part[c] = environment_name(c);
  } else if (!split(name, parts)) {
    return false;
  }
  for (int c = 0; c < ncategories; ++c) {
    const char* n = parts.part[c].empty() ? environment_name(c) : normalize_simple(parts.part[c].c_str());
    if (n == nullptr)
      return false;
    parts.part[c] = n;
  }
  out = join(parts);
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

locale_impl* make_classic() {
  // refs == 1: the classic facets are never deleted
  const std::locale::facet* facets[] = {
      new std::collate<char>(1),
      new std::collate<wchar_t>(1),
      new std::ctype<char>(nullptr, false, 1),
      new std::ctype<wchar_t>(1),
      new std::codecvt<char, char, std::mbstate_t>(1),
      new std::codecvt<wchar_t, char, std::mbstate_t>(1),
      new std::codecvt<char16_t, char8_t, std::mbstate_t>(1),
      new std::codecvt<char32_t, char8_t, std::mbstate_t>(1),
      new std::moneypunct<char, false>(1),
      new std::moneypunct<char, true>(1),
      new std::moneypunct<wchar_t, false>(1),
      new std::moneypunct<wchar_t, true>(1),
      new std::money_get<char>(1),
      new std::money_get<wchar_t>(1),
      new std::money_put<char>(1),
      new std::money_put<wchar_t>(1),
      new std::numpunct<char>(1),
      new std::numpunct<wchar_t>(1),
      new std::num_get<char>(1),
      new std::num_get<wchar_t>(1),
      new std::num_put<char>(1),
      new std::num_put<wchar_t>(1),
      new std::time_get<char>(1),
      new std::time_get<wchar_t>(1),
      new std::time_put<char>(1),
      new std::time_put<wchar_t>(1),
      new std::messages<char>(1),
      new std::messages<wchar_t>(1),
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
  locale_impl* p = new_impl(top + 1, "C");
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

namespace std {

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
  string name;
  if (!resolve_name(std_name, name))
    bad_name("std::locale", std_name);
  impl_ = clone(classic_locale().impl_, 0, name.c_str());
}

locale::locale(const locale& other, const char* std_name, category cats) : impl_(nullptr) {
  string name;
  if (!resolve_name(std_name, name))
    bad_name("std::locale", std_name);
  const locale_impl* one = classic_locale().impl_;
  string result;
  if (other.impl_->name != nullptr) {
    split_name a, b;
    split(other.impl_->name, a);
    split(name.c_str(), b);
    for (int c = 0; c < ncategories; ++c)
      if (cats & category_bits[c])
        a.part[c] = b.part[c];
    result = join(a);
  }
  locale_impl* p = clone(other.impl_, one->nfacets, other.impl_->name != nullptr ? result.c_str() : nullptr);
  for (int c = 0; c < ncategories; ++c) {
    if (!(cats & category_bits[c]))
      continue;
    const category_ids& ids = ids_of(c);
    for (int k = 0; k < ids.n; ++k) {
      const size_t i = locale_access::index(*ids.ids[k]);
      set_facet(p, i, i < one->nfacets ? one->facets[i] : nullptr);
    }
  }
  impl_ = p;
}

locale::locale(const locale& other, const locale& one, category cats) : impl_(nullptr) {
  string result;
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
  retain(loc.impl_);
  {
    lock_guard g(global_lock);
    old = global_impl;
    global_impl = loc.impl_;
  }
  if (old == nullptr) { // the classic locale, whose reference the global did not hold
    old = c.impl_;
    retain(old);
  }
  if (loc.impl_->name != nullptr) {
    split_name parts;
    split(loc.impl_->name, parts);
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

// The pending UTF-16 surrogate kept in an mbstate_t between calls (0: none).
char32_t load_state(const std::mbstate_t& st) noexcept {
  char32_t v;
  __builtin_memcpy(&v, st.__state, sizeof v);
  return v;
}
void store_state(std::mbstate_t& st, char32_t v) noexcept { __builtin_memcpy(st.__state, &v, sizeof v); }

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

namespace std {

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

// codecvt<char16_t, char8_t, mbstate_t>: UTF-16 <-> UTF-8. A supplementary character is split
// across calls through the state ([locale.codecvt.virtuals]/4): do_out consumes a high
// surrogate alone and keeps it; do_in with room for one unit writes the high surrogate and
// keeps the low one, consuming the UTF-8 sequence when the low one is written.
locale::id codecvt<char16_t, char8_t, mbstate_t>::id;
codecvt<char16_t, char8_t, mbstate_t>::~codecvt() {}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_out(mbstate_t& state, const char16_t* from,
                                                                   const char16_t* from_end, const char16_t*& from_next,
                                                                   char8_t* to, char8_t* to_end,
                                                                   char8_t*& to_next) const {
  char32_t high = load_state(state);
  for (; from != from_end; ++from) {
    const char16_t u = *from;
    char32_t cp;
    if (high != 0) {
      if (u < 0xDC00 || u > 0xDFFF) {
        from_next = from;
        to_next = to;
        return error;
      }
      cp = 0x10000 + ((high - 0xD800) << 10) + (u - 0xDC00);
    } else if (u >= 0xD800 && u <= 0xDBFF) {
      high = u;
      store_state(state, high);
      continue;
    } else if (u >= 0xDC00 && u <= 0xDFFF) {
      from_next = from;
      to_next = to;
      return error;
    } else {
      cp = u;
    }
    unsigned char buf[4];
    const int n = encode_utf8(cp, buf);
    if (to_end - to < n) {
      // the pending high surrogate (if any) stays in the state; u is not consumed
      from_next = from;
      to_next = to;
      return partial;
    }
    for (int i = 0; i < n; ++i)
      *to++ = static_cast<char8_t>(buf[i]);
    high = 0;
    store_state(state, 0);
  }
  from_next = from;
  to_next = to;
  return ok;
}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_in(mbstate_t& state, const char8_t* from,
                                                                  const char8_t* from_end, const char8_t*& from_next,
                                                                  char16_t* to, char16_t* to_end,
                                                                  char16_t*& to_next) const {
  while (from != from_end) {
    if (to == to_end) {
      from_next = from;
      to_next = to;
      return partial;
    }
    char32_t cp;
    const int n =
        decode_utf8(reinterpret_cast<const unsigned char*>(from), reinterpret_cast<const unsigned char*>(from_end), cp);
    if (n <= 0) {
      from_next = from;
      to_next = to;
      return n == 0 ? partial : error;
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
      return ok;
    }
    *to++ = lo;
    from += n;
  }
  from_next = from;
  to_next = to;
  return ok;
}
codecvt_base::result codecvt<char16_t, char8_t, mbstate_t>::do_unshift(mbstate_t&, char8_t* to, char8_t*,
                                                                       char8_t*& to_next) const {
  to_next = to;
  return noconv;
}
int codecvt<char16_t, char8_t, mbstate_t>::do_encoding() const noexcept { return 0; }
bool codecvt<char16_t, char8_t, mbstate_t>::do_always_noconv() const noexcept { return false; }
int codecvt<char16_t, char8_t, mbstate_t>::do_length(mbstate_t&, const char8_t* from, const char8_t* end,
                                                     size_t max) const {
  return utf8_length(from, end, max, 2);
}
int codecvt<char16_t, char8_t, mbstate_t>::do_max_length() const noexcept { return 4; }

} // namespace std

namespace ycxx::detail {

void check_locale_name(const char* name, const char* what) {
  std::string resolved;
  if (!resolve_name(name, resolved))
    bad_name(what, name);
}

} // namespace ycxx::detail
