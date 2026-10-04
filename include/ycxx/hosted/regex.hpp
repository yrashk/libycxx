// libycxx hosted: basic_regex, sub_match, match_results, the regular expression algorithms and
// iterators ([re.regex] .. [re.iter]).
//
// basic_regex holds its traits object, its flags and a shared, immutable compiled program
// (ycxx/hosted/regex_compile.hpp), so copies share the program and swap is constant time. A
// default-constructed or imbued basic_regex has no program and matches nothing.
//
// match_results keeps its sub_match objects in a vector with its allocator, plus the prefix, the
// suffix, the sub_match that operator[] returns past size() (unmatched, at the end of the target
// sequence) and the start of the target sequence for position().
//
// regex_iterator::operator++ searches the rest of the sequence with match_prev_avail as
// [re.regiter.incr] specifies; it also passes match_prev_avail to the match_continuous retry
// after a zero-length match when that match is not at the beginning of the sequence, so that ^
// and \b see the preceding character there.
#pragma once

#include <ycxx/core/iterator_ops.hpp>
#include <ycxx/core/shared_ptr.hpp>
#include <ycxx/core/vector.hpp>
#include <ycxx/hosted/iosfwd.hpp>
#include <ycxx/hosted/regex_engine.hpp>

namespace ycxx::detail {
struct regex_access;
} // namespace ycxx::detail

namespace std {

// [re.regex]
template <class charT, class traits = regex_traits<charT>>
class basic_regex {
public:
  using value_type = charT;
  using traits_type = traits;
  using string_type = typename traits::string_type;
  using flag_type = regex_constants::syntax_option_type;
  using locale_type = typename traits::locale_type;

  static constexpr flag_type icase = regex_constants::icase;
  static constexpr flag_type nosubs = regex_constants::nosubs;
  static constexpr flag_type optimize = regex_constants::optimize;
  static constexpr flag_type collate = regex_constants::collate;
  static constexpr flag_type ECMAScript = regex_constants::ECMAScript;
  static constexpr flag_type basic = regex_constants::basic;
  static constexpr flag_type extended = regex_constants::extended;
  static constexpr flag_type awk = regex_constants::awk;
  static constexpr flag_type grep = regex_constants::grep;
  static constexpr flag_type egrep = regex_constants::egrep;
  static constexpr flag_type multiline = regex_constants::multiline;

  // [re.regex.construct]
  basic_regex() = default;
  explicit basic_regex(const charT* p, flag_type f = regex_constants::ECMAScript) { assign(p, f); }
  basic_regex(const charT* p, size_t len, flag_type f = regex_constants::ECMAScript) { assign(p, len, f); }
  basic_regex(const basic_regex&) = default;
  basic_regex(basic_regex&& e) noexcept
      : traits_(e.traits_), flags_(e.flags_), marks_(e.marks_), prog_(static_cast<shared_ptr<const program>&&>(e.prog_)) {}
  template <class ST, class SA>
  explicit basic_regex(const basic_string<charT, ST, SA>& s, flag_type f = regex_constants::ECMAScript) {
    assign(s, f);
  }
  template <class ForwardIterator>
  basic_regex(ForwardIterator first, ForwardIterator last, flag_type f = regex_constants::ECMAScript) {
    assign(first, last, f);
  }
  basic_regex(initializer_list<charT> il, flag_type f = regex_constants::ECMAScript) { assign(il.begin(), il.end(), f); }
  ~basic_regex() = default;

  // [re.regex.assign]
  basic_regex& operator=(const basic_regex& e) = default;
  basic_regex& operator=(basic_regex&& e) noexcept {
    traits_ = e.traits_;
    flags_ = e.flags_;
    marks_ = e.marks_;
    prog_ = static_cast<shared_ptr<const program>&&>(e.prog_);
    return *this;
  }
  basic_regex& operator=(const charT* p) { return assign(p); }
  basic_regex& operator=(initializer_list<charT> il) { return assign(il.begin(), il.end()); }
  template <class ST, class SA>
  basic_regex& operator=(const basic_string<charT, ST, SA>& s) {
    return assign(s);
  }
  basic_regex& assign(const basic_regex& e) { return *this = e; }
  basic_regex& assign(basic_regex&& e) noexcept { return *this = static_cast<basic_regex&&>(e); }
  basic_regex& assign(const charT* p, flag_type f = regex_constants::ECMAScript) {
    return compile(p, p + traits::length(p), f);
  }
  basic_regex& assign(const charT* p, size_t len, flag_type f = regex_constants::ECMAScript) {
    return compile(p, p + len, f);
  }
  template <class ST, class SA>
  basic_regex& assign(const basic_string<charT, ST, SA>& s, flag_type f = regex_constants::ECMAScript) {
    return compile(s.data(), s.data() + s.size(), f);
  }
  template <class InputIterator>
  basic_regex& assign(InputIterator first, InputIterator last, flag_type f = regex_constants::ECMAScript) {
    const string_type s(first, last);
    return compile(s.data(), s.data() + s.size(), f);
  }
  basic_regex& assign(initializer_list<charT> il, flag_type f = regex_constants::ECMAScript) {
    return compile(il.begin(), il.end(), f);
  }

  // [re.regex.operations]
  unsigned mark_count() const { return marks_; }
  flag_type flags() const { return flags_; }

  // [re.regex.locale]
  locale_type imbue(locale_type loc) {
    locale_type old = traits_.imbue(loc);
    prog_.reset();
    marks_ = 0;
    return old;
  }
  locale_type getloc() const { return traits_.getloc(); }

  // [re.regex.swap]
  void swap(basic_regex& e) {
    traits t = traits_;
    traits_ = e.traits_;
    e.traits_ = t;
    const flag_type f = flags_;
    flags_ = e.flags_;
    e.flags_ = f;
    const unsigned m = marks_;
    marks_ = e.marks_;
    e.marks_ = m;
    prog_.swap(e.prog_);
  }

private:
  friend ::ycxx::detail::regex_access;
  using program = ::ycxx::detail::re_program<charT, traits>;

  // Strong guarantee: *this changes only once the new program is built.
  basic_regex& compile(const charT* first, const charT* last, flag_type f) {
    shared_ptr<program> p = std::make_shared<program>();
    ::ycxx::detail::re_compiler<charT, traits>(traits_, *p).compile(first, last, f);
    flags_ = f;
    marks_ = (f & regex_constants::nosubs) ? 0u : static_cast<unsigned>(p->groups);
    prog_ = static_cast<shared_ptr<program>&&>(p);
    return *this;
  }

  traits traits_;
  flag_type flags_ = regex_constants::ECMAScript;
  unsigned marks_ = 0;
  shared_ptr<const program> prog_;
};

template <class ForwardIterator>
basic_regex(ForwardIterator, ForwardIterator, regex_constants::syntax_option_type = regex_constants::ECMAScript)
    -> basic_regex<typename iterator_traits<ForwardIterator>::value_type>;

using regex = basic_regex<char>;
using wregex = basic_regex<wchar_t>;

// [re.regex.nonmemb]
template <class charT, class traits>
void swap(basic_regex<charT, traits>& e1, basic_regex<charT, traits>& e2) {
  e1.swap(e2);
}

// [re.submatch]
template <class BidirectionalIterator>
class sub_match : public pair<BidirectionalIterator, BidirectionalIterator> {
  using base = pair<BidirectionalIterator, BidirectionalIterator>;

public:
  using value_type = typename iterator_traits<BidirectionalIterator>::value_type;
  using difference_type = typename iterator_traits<BidirectionalIterator>::difference_type;
  using iterator = BidirectionalIterator;
  using string_type = basic_string<value_type>;

  bool matched;

  constexpr sub_match() : base(), matched() {}

  difference_type length() const { return matched ? std::distance(this->first, this->second) : 0; }
  operator string_type() const { return str(); }
  string_type str() const { return matched ? string_type(this->first, this->second) : string_type(); }
  int compare(const sub_match& s) const { return str().compare(s.str()); }
  int compare(const string_type& s) const { return str().compare(s); }
  int compare(const value_type* s) const { return str().compare(s); }
  void swap(sub_match& s) noexcept(is_nothrow_swappable_v<BidirectionalIterator>) {
    this->base::swap(s);
    const bool m = matched;
    matched = s.matched;
    s.matched = m;
  }
};

using csub_match = sub_match<const char*>;
using wcsub_match = sub_match<const wchar_t*>;
using ssub_match = sub_match<string::const_iterator>;
using wssub_match = sub_match<wstring::const_iterator>;

} // namespace std

namespace ycxx::detail {
// SM-CAT(I) of [re.submatch.op]
template <class BiIter>
using regex_sm_cat_t = std::compare_three_way_result_t<std::basic_string<typename std::iterator_traits<BiIter>::value_type>>;
} // namespace ycxx::detail

namespace std {

// [re.submatch.op]

template <class BiIter>
bool operator==(const sub_match<BiIter>& lhs, const sub_match<BiIter>& rhs) {
  return lhs.compare(rhs) == 0;
}
template <class BiIter>
auto operator<=>(const sub_match<BiIter>& lhs, const sub_match<BiIter>& rhs) {
  return static_cast<::ycxx::detail::regex_sm_cat_t<BiIter>>(lhs.compare(rhs) <=> 0);
}
template <class BiIter, class ST, class SA>
bool operator==(const sub_match<BiIter>& lhs, const basic_string<typename iterator_traits<BiIter>::value_type, ST, SA>& rhs) {
  return lhs.compare(typename sub_match<BiIter>::string_type(rhs.data(), rhs.size())) == 0;
}
template <class BiIter, class ST, class SA>
auto operator<=>(const sub_match<BiIter>& lhs, const basic_string<typename iterator_traits<BiIter>::value_type, ST, SA>& rhs) {
  return static_cast<::ycxx::detail::regex_sm_cat_t<BiIter>>(lhs.compare(typename sub_match<BiIter>::string_type(rhs.data(), rhs.size())) <=> 0);
}
template <class BiIter>
bool operator==(const sub_match<BiIter>& lhs, const typename iterator_traits<BiIter>::value_type* rhs) {
  return lhs.compare(rhs) == 0;
}
template <class BiIter>
auto operator<=>(const sub_match<BiIter>& lhs, const typename iterator_traits<BiIter>::value_type* rhs) {
  return static_cast<::ycxx::detail::regex_sm_cat_t<BiIter>>(lhs.compare(rhs) <=> 0);
}
template <class BiIter>
bool operator==(const sub_match<BiIter>& lhs, const typename iterator_traits<BiIter>::value_type& rhs) {
  return lhs.compare(typename sub_match<BiIter>::string_type(1, rhs)) == 0;
}
template <class BiIter>
auto operator<=>(const sub_match<BiIter>& lhs, const typename iterator_traits<BiIter>::value_type& rhs) {
  return static_cast<::ycxx::detail::regex_sm_cat_t<BiIter>>(lhs.compare(typename sub_match<BiIter>::string_type(1, rhs)) <=> 0);
}
template <class charT, class ST, class BiIter>
basic_ostream<charT, ST>& operator<<(basic_ostream<charT, ST>& os, const sub_match<BiIter>& m) {
  return os << m.str();
}

// [re.results]
template <class BidirectionalIterator, class Allocator = allocator<sub_match<BidirectionalIterator>>>
class match_results;
template <class BidirectionalIterator, class Allocator>
class match_results {
  using vector_type = vector<sub_match<BidirectionalIterator>, Allocator>;

public:
  using value_type = sub_match<BidirectionalIterator>;
  using const_reference = const value_type&;
  using reference = value_type&;
  using const_iterator = typename vector_type::const_iterator;
  using iterator = const_iterator;
  using difference_type = typename iterator_traits<BidirectionalIterator>::difference_type;
  using size_type = typename allocator_traits<Allocator>::size_type;
  using allocator_type = Allocator;
  using char_type = typename iterator_traits<BidirectionalIterator>::value_type;
  using string_type = basic_string<char_type>;

  // [re.results.const]
  match_results() : match_results(Allocator()) {}
  explicit match_results(const Allocator& a) : subs_(a) {}
  match_results(const match_results& m) = default;
  match_results(const match_results& m, const Allocator& a)
      : subs_(m.subs_, a), prefix_(m.prefix_), suffix_(m.suffix_), unmatched_(m.unmatched_), base_(m.base_), ready_(m.ready_) {}
  match_results(match_results&& m) noexcept
      : subs_(static_cast<vector_type&&>(m.subs_)), prefix_(m.prefix_), suffix_(m.suffix_), unmatched_(m.unmatched_),
        base_(m.base_), ready_(m.ready_) {}
  match_results(match_results&& m, const Allocator& a)
      : subs_(static_cast<vector_type&&>(m.subs_), a), prefix_(m.prefix_), suffix_(m.suffix_), unmatched_(m.unmatched_),
        base_(m.base_), ready_(m.ready_) {}
  match_results& operator=(const match_results& m) = default;
  match_results& operator=(match_results&& m) = default;
  ~match_results() = default;

  // [re.results.state]
  bool ready() const { return ready_; }

  // [re.results.size]
  size_type size() const { return subs_.size(); }
  size_type max_size() const { return subs_.max_size(); }
  bool empty() const { return subs_.empty(); }

  // [re.results.acc]
  difference_type length(size_type sub = 0) const { return (*this)[sub].length(); }
  difference_type position(size_type sub = 0) const {
    ::ycxx::detail::precondition(ready_, "match_results::position: not ready");
    return std::distance(base_, (*this)[sub].first);
  }
  string_type str(size_type sub = 0) const { return string_type((*this)[sub]); }
  const_reference operator[](size_type n) const {
    ::ycxx::detail::precondition(ready_, "match_results::operator[]: not ready");
    return n < subs_.size() ? subs_[n] : unmatched_;
  }
  const_reference prefix() const {
    ::ycxx::detail::precondition(ready_, "match_results::prefix: not ready");
    return prefix_;
  }
  const_reference suffix() const {
    ::ycxx::detail::precondition(ready_, "match_results::suffix: not ready");
    return suffix_;
  }
  const_iterator begin() const { return subs_.begin(); }
  const_iterator end() const { return subs_.end(); }
  const_iterator cbegin() const { return subs_.begin(); }
  const_iterator cend() const { return subs_.end(); }

  // [re.results.form]
  template <class OutputIter>
  OutputIter format(OutputIter out, const char_type* fmt_first, const char_type* fmt_last,
                    regex_constants::match_flag_type flags = regex_constants::format_default) const {
    ::ycxx::detail::precondition(ready_, "match_results::format: not ready");
    auto put = [&out](const value_type& s) {
      if (s.matched)
        for (BidirectionalIterator i = s.first; i != s.second; ++i) {
          *out = *i;
          ++out;
        }
    };
    auto digit = [](char_type c) { return c >= char_type('0') && c <= char_type('9') ? int(c - char_type('0')) : -1; };
    for (const char_type* p = fmt_first; p != fmt_last; ++p) {
      const char_type c = *p;
      if (flags & regex_constants::format_sed) {
        // sed: & is the whole match, \n the n-th subexpression, \c the character c.
        if (c == char_type('&')) {
          put((*this)[0]);
          continue;
        }
        if (c == char_type('\\') && p + 1 != fmt_last) {
          ++p;
          if (digit(*p) >= 0) {
            put((*this)[static_cast<size_type>(digit(*p))]);
          } else {
            *out = *p;
            ++out;
          }
          continue;
        }
      } else if (c == char_type('$') && p + 1 != fmt_last) {
        // ECMA-262 String.prototype.replace: $$ $& $` $' $n $nn.
        const char_type d = p[1];
        if (d == char_type('$')) {
          ++p;
          *out = d;
          ++out;
          continue;
        }
        if (d == char_type('&')) {
          ++p;
          put((*this)[0]);
          continue;
        }
        if (d == char_type('`')) {
          ++p;
          put(prefix_);
          continue;
        }
        if (d == char_type('\'')) {
          ++p;
          put(suffix_);
          continue;
        }
        if (digit(d) >= 0) {
          size_type n = static_cast<size_type>(digit(d));
          ++p;
          if (p + 1 != fmt_last && digit(p[1]) >= 0) {
            const size_type nn = n * 10 + static_cast<size_type>(digit(p[1]));
            if (nn < size()) {
              n = nn;
              ++p;
            }
          }
          put((*this)[n]);
          continue;
        }
      }
      *out = c;
      ++out;
    }
    return out;
  }
  template <class OutputIter, class ST, class SA>
  OutputIter format(OutputIter out, const basic_string<char_type, ST, SA>& fmt,
                    regex_constants::match_flag_type flags = regex_constants::format_default) const {
    return format(out, fmt.data(), fmt.data() + fmt.size(), flags);
  }
  template <class ST, class SA>
  basic_string<char_type, ST, SA> format(const basic_string<char_type, ST, SA>& fmt,
                                         regex_constants::match_flag_type flags = regex_constants::format_default) const {
    basic_string<char_type, ST, SA> result;
    format(back_inserter(result), fmt, flags);
    return result;
  }
  string_type format(const char_type* fmt, regex_constants::match_flag_type flags = regex_constants::format_default) const {
    string_type result;
    format(back_inserter(result), fmt, fmt + char_traits<char_type>::length(fmt), flags);
    return result;
  }

  // [re.results.all]
  allocator_type get_allocator() const { return subs_.get_allocator(); }

  // [re.results.swap]
  void swap(match_results& that) {
    subs_.swap(that.subs_);
    prefix_.swap(that.prefix_);
    suffix_.swap(that.suffix_);
    unmatched_.swap(that.unmatched_);
    const BidirectionalIterator b = base_;
    base_ = that.base_;
    that.base_ = b;
    const bool r = ready_;
    ready_ = that.ready_;
    that.ready_ = r;
  }

private:
  friend ::ycxx::detail::regex_access;

  vector_type subs_;
  value_type prefix_, suffix_, unmatched_;
  BidirectionalIterator base_{};
  bool ready_ = false;
};

using cmatch = match_results<const char*>;
using wcmatch = match_results<const wchar_t*>;
using smatch = match_results<string::const_iterator>;
using wsmatch = match_results<wstring::const_iterator>;

// [re.results.nonmember]
template <class BidirectionalIterator, class Allocator>
bool operator==(const match_results<BidirectionalIterator, Allocator>& m1,
                const match_results<BidirectionalIterator, Allocator>& m2) {
  if (!m1.ready() || !m2.ready())
    return m1.ready() == m2.ready();
  if (m1.empty() || m2.empty())
    return m1.empty() && m2.empty();
  if (!(m1.prefix() == m2.prefix()) || m1.size() != m2.size() || !(m1.suffix() == m2.suffix()))
    return false;
  for (typename match_results<BidirectionalIterator, Allocator>::size_type i = 0; i < m1.size(); ++i)
    if (!(m1[i] == m2[i]))
      return false;
  return true;
}

// [re.results.swap]
template <class BidirectionalIterator, class Allocator>
void swap(match_results<BidirectionalIterator, Allocator>& m1, match_results<BidirectionalIterator, Allocator>& m2) {
  m1.swap(m2);
}

namespace pmr {
template <class BidirectionalIterator>
using match_results = std::match_results<BidirectionalIterator, polymorphic_allocator<sub_match<BidirectionalIterator>>>;
using cmatch = match_results<const char*>;
using wcmatch = match_results<const wchar_t*>;
using smatch = match_results<string::const_iterator>;
using wsmatch = match_results<wstring::const_iterator>;
} // namespace pmr

} // namespace std

namespace ycxx::detail {

struct regex_access {
  // Matches e against [first, last) (whole: regex_match) and fills m; positions count from base.
  template <class It, class Alloc, class charT, class traits>
  static bool run(It first, It last, std::match_results<It, Alloc>& m, const std::basic_regex<charT, traits>& e,
                  std::regex_constants::match_flag_type flags, bool whole, It base) {
    m.ready_ = true;
    m.base_ = base;
    m.subs_.clear();
    m.unmatched_.first = m.unmatched_.second = last;
    m.unmatched_.matched = false;
    m.prefix_ = m.unmatched_;
    m.suffix_ = m.unmatched_;
    if (!e.prog_)
      return false;
    std::vector<re_cap<It>> caps;
    if (!::ycxx::detail::re_execute(*e.prog_, e.traits_, first, last, flags, whole, caps))
      return false;
    const std::size_t n = (e.flags_ & std::regex_constants::nosubs) ? 1 : caps.size();
    m.subs_.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
      m.subs_[i].first = caps[i].first;
      m.subs_[i].second = caps[i].second;
      m.subs_[i].matched = caps[i].matched;
    }
    m.prefix_.first = first;
    m.prefix_.second = caps[0].first;
    m.prefix_.matched = m.prefix_.first != m.prefix_.second;
    m.suffix_.first = caps[0].second;
    m.suffix_.second = last;
    m.suffix_.matched = m.suffix_.first != m.suffix_.second;
    return true;
  }
  // regex_iterator: the prefix of a match found after an earlier one starts where that one ended.
  template <class It, class Alloc>
  static void set_prefix_first(std::match_results<It, Alloc>& m, It first) {
    m.prefix_.first = first;
    m.prefix_.matched = m.prefix_.first != m.prefix_.second;
  }
};

} // namespace ycxx::detail

namespace std {

// [re.alg.match]
template <class BidirectionalIterator, class Allocator, class charT, class traits>
bool regex_match(BidirectionalIterator first, BidirectionalIterator last, match_results<BidirectionalIterator, Allocator>& m,
                 const basic_regex<charT, traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::ycxx::detail::regex_access::run(first, last, m, e, flags, true, first);
}
template <class BidirectionalIterator, class charT, class traits>
bool regex_match(BidirectionalIterator first, BidirectionalIterator last, const basic_regex<charT, traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  match_results<BidirectionalIterator> what;
  return ::ycxx::detail::regex_access::run(first, last, what, e, flags, true, first);
}
template <class charT, class Allocator, class traits>
bool regex_match(const charT* str, match_results<const charT*, Allocator>& m, const basic_regex<charT, traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(str, str + char_traits<charT>::length(str), m, e, flags);
}
template <class ST, class SA, class Allocator, class charT, class traits>
bool regex_match(const basic_string<charT, ST, SA>& s,
                 match_results<typename basic_string<charT, ST, SA>::const_iterator, Allocator>& m,
                 const basic_regex<charT, traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(s.begin(), s.end(), m, e, flags);
}
template <class ST, class SA, class Allocator, class charT, class traits>
bool regex_match(const basic_string<charT, ST, SA>&&,
                 match_results<typename basic_string<charT, ST, SA>::const_iterator, Allocator>&,
                 const basic_regex<charT, traits>&, regex_constants::match_flag_type = regex_constants::match_default) = delete;
template <class charT, class traits>
bool regex_match(const charT* str, const basic_regex<charT, traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(str, str + char_traits<charT>::length(str), e, flags);
}
template <class ST, class SA, class charT, class traits>
bool regex_match(const basic_string<charT, ST, SA>& s, const basic_regex<charT, traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(s.begin(), s.end(), e, flags);
}

// [re.alg.search]
template <class BidirectionalIterator, class Allocator, class charT, class traits>
bool regex_search(BidirectionalIterator first, BidirectionalIterator last, match_results<BidirectionalIterator, Allocator>& m,
                  const basic_regex<charT, traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::ycxx::detail::regex_access::run(first, last, m, e, flags, false, first);
}
template <class BidirectionalIterator, class charT, class traits>
bool regex_search(BidirectionalIterator first, BidirectionalIterator last, const basic_regex<charT, traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  match_results<BidirectionalIterator> what;
  return ::ycxx::detail::regex_access::run(first, last, what, e, flags, false, first);
}
template <class charT, class Allocator, class traits>
bool regex_search(const charT* str, match_results<const charT*, Allocator>& m, const basic_regex<charT, traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(str, str + char_traits<charT>::length(str), m, e, flags);
}
template <class charT, class traits>
bool regex_search(const charT* str, const basic_regex<charT, traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(str, str + char_traits<charT>::length(str), e, flags);
}
template <class ST, class SA, class charT, class traits>
bool regex_search(const basic_string<charT, ST, SA>& s, const basic_regex<charT, traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(s.begin(), s.end(), e, flags);
}
template <class ST, class SA, class Allocator, class charT, class traits>
bool regex_search(const basic_string<charT, ST, SA>& s,
                  match_results<typename basic_string<charT, ST, SA>::const_iterator, Allocator>& m,
                  const basic_regex<charT, traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(s.begin(), s.end(), m, e, flags);
}
template <class ST, class SA, class Allocator, class charT, class traits>
bool regex_search(const basic_string<charT, ST, SA>&&,
                  match_results<typename basic_string<charT, ST, SA>::const_iterator, Allocator>&,
                  const basic_regex<charT, traits>&, regex_constants::match_flag_type = regex_constants::match_default) = delete;

// [re.regiter]
template <class BidirectionalIterator, class charT = typename iterator_traits<BidirectionalIterator>::value_type,
          class traits = regex_traits<charT>>
class regex_iterator {
public:
  using regex_type = basic_regex<charT, traits>;
  using iterator_category = forward_iterator_tag;
  using iterator_concept = input_iterator_tag;
  using value_type = match_results<BidirectionalIterator>;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  regex_iterator() = default;
  regex_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re,
                 regex_constants::match_flag_type m = regex_constants::match_default)
      : begin_(a), end_(b), pregex_(__builtin_addressof(re)), flags_(m) {
    if (!::ycxx::detail::regex_access::run(begin_, end_, match_, *pregex_, flags_, false, begin_))
      pregex_ = nullptr;
  }
  regex_iterator(BidirectionalIterator, BidirectionalIterator, const regex_type&&,
                 regex_constants::match_flag_type = regex_constants::match_default) = delete;
  regex_iterator(const regex_iterator&) = default;
  regex_iterator& operator=(const regex_iterator&) = default;

  bool operator==(const regex_iterator& right) const {
    if (pregex_ == nullptr || right.pregex_ == nullptr)
      return pregex_ == right.pregex_;
    return begin_ == right.begin_ && end_ == right.end_ && pregex_ == right.pregex_ && flags_ == right.flags_ &&
           match_[0] == right.match_[0];
  }
  bool operator==(default_sentinel_t) const noexcept { return pregex_ == nullptr; }
  const value_type& operator*() const { return match_; }
  const value_type* operator->() const { return __builtin_addressof(match_); }

  // [re.regiter.incr]
  regex_iterator& operator++() {
    namespace rc = regex_constants;
    using access = ::ycxx::detail::regex_access;
    BidirectionalIterator start = match_[0].second;
    const BidirectionalIterator prev_end = start;
    if (match_[0].first == match_[0].second) {
      if (start == end_) {
        *this = regex_iterator();
        return *this;
      }
      const rc::match_flag_type f =
          flags_ | rc::match_not_null | rc::match_continuous | (start != begin_ ? rc::match_prev_avail : rc::match_default);
      if (access::run(start, end_, match_, *pregex_, f, false, begin_)) {
        access::set_prefix_first(match_, prev_end);
        return *this;
      }
      ++start;
    }
    flags_ = flags_ | rc::match_prev_avail;
    if (!access::run(start, end_, match_, *pregex_, flags_, false, begin_)) {
      *this = regex_iterator();
      return *this;
    }
    access::set_prefix_first(match_, prev_end);
    return *this;
  }
  regex_iterator operator++(int) {
    regex_iterator t = *this;
    ++*this;
    return t;
  }

private:
  BidirectionalIterator begin_{}, end_{};
  const regex_type* pregex_ = nullptr;
  regex_constants::match_flag_type flags_{};
  match_results<BidirectionalIterator> match_;
};

using cregex_iterator = regex_iterator<const char*>;
using wcregex_iterator = regex_iterator<const wchar_t*>;
using sregex_iterator = regex_iterator<string::const_iterator>;
using wsregex_iterator = regex_iterator<wstring::const_iterator>;

// [re.tokiter]
template <class BidirectionalIterator, class charT = typename iterator_traits<BidirectionalIterator>::value_type,
          class traits = regex_traits<charT>>
class regex_token_iterator {
public:
  using regex_type = basic_regex<charT, traits>;
  using iterator_category = forward_iterator_tag;
  using iterator_concept = input_iterator_tag;
  using value_type = sub_match<BidirectionalIterator>;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  regex_token_iterator() = default;
  regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re, int submatch = 0,
                       regex_constants::match_flag_type m = regex_constants::match_default)
      : subs_(1, submatch) {
    init(a, b, re, m);
  }
  regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re,
                       const vector<int>& submatches, regex_constants::match_flag_type m = regex_constants::match_default)
      : subs_(submatches) {
    init(a, b, re, m);
  }
  regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re,
                       initializer_list<int> submatches, regex_constants::match_flag_type m = regex_constants::match_default)
      : subs_(submatches) {
    init(a, b, re, m);
  }
  template <size_t N>
  regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re, const int (&submatches)[N],
                       regex_constants::match_flag_type m = regex_constants::match_default)
      : subs_(submatches, submatches + N) {
    init(a, b, re, m);
  }
  regex_token_iterator(BidirectionalIterator, BidirectionalIterator, const regex_type&&, int = 0,
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;
  regex_token_iterator(BidirectionalIterator, BidirectionalIterator, const regex_type&&, const vector<int>&,
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;
  regex_token_iterator(BidirectionalIterator, BidirectionalIterator, const regex_type&&, initializer_list<int>,
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;
  template <size_t N>
  regex_token_iterator(BidirectionalIterator, BidirectionalIterator, const regex_type&&, const int (&)[N],
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;

  regex_token_iterator(const regex_token_iterator& o)
      : position_(o.position_), suffix_(o.suffix_), N_(o.N_), subs_(o.subs_) {
    rebind(o);
  }
  regex_token_iterator& operator=(const regex_token_iterator& o) {
    if (this != __builtin_addressof(o)) {
      position_ = o.position_;
      suffix_ = o.suffix_;
      N_ = o.N_;
      subs_ = o.subs_;
      rebind(o);
    }
    return *this;
  }

  // [re.tokiter.comp]
  bool operator==(const regex_token_iterator& right) const {
    if (result_ == nullptr || right.result_ == nullptr)
      return result_ == nullptr && right.result_ == nullptr;
    const bool s1 = is_suffix(), s2 = right.is_suffix();
    if (s1 || s2)
      return s1 && s2 && suffix_ == right.suffix_;
    return position_ == right.position_ && N_ == right.N_ && subs_ == right.subs_;
  }
  bool operator==(default_sentinel_t) const noexcept { return result_ == nullptr; }

  // [re.tokiter.deref]
  const value_type& operator*() const { return *result_; }
  const value_type* operator->() const { return result_; }

  // [re.tokiter.incr]
  regex_token_iterator& operator++() {
    const position_iterator prev = position_;
    if (is_suffix()) {
      result_ = nullptr;
      return *this;
    }
    if (N_ + 1 < subs_.size()) {
      ++N_;
      result_ = current();
      return *this;
    }
    N_ = 0;
    ++position_;
    if (position_ != position_iterator()) {
      result_ = current();
      return *this;
    }
    if (wants_suffix() && prev->suffix().length() != 0) {
      suffix_.first = prev->suffix().first;
      suffix_.second = prev->suffix().second;
      suffix_.matched = true;
      result_ = __builtin_addressof(suffix_);
      return *this;
    }
    result_ = nullptr;
    return *this;
  }
  regex_token_iterator operator++(int) {
    regex_token_iterator t = *this;
    ++*this;
    return t;
  }

private:
  using position_iterator = regex_iterator<BidirectionalIterator, charT, traits>;

  bool is_suffix() const { return result_ == __builtin_addressof(suffix_); }
  bool wants_suffix() const {
    for (int s : subs_)
      if (s == -1)
        return true;
    return false;
  }
  const value_type* current() const {
    const int s = subs_[N_];
    return s == -1 ? __builtin_addressof(position_->prefix())
                   : __builtin_addressof((*position_)[static_cast<typename match_results<BidirectionalIterator>::size_type>(s)]);
  }
  void init(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re, regex_constants::match_flag_type m) {
    for (int s : subs_)
      ::ycxx::detail::precondition(s >= -1, "regex_token_iterator: a submatch index below -1");
    position_ = position_iterator(a, b, re, m);
    N_ = 0;
    if (position_ != position_iterator()) {
      result_ = current();
    } else if (wants_suffix()) {
      suffix_.first = a;
      suffix_.second = b;
      suffix_.matched = true;
      result_ = __builtin_addressof(suffix_);
    }
  }
  // After a copy: point into this object's own position or suffix.
  void rebind(const regex_token_iterator& o) {
    if (o.result_ == nullptr)
      result_ = nullptr;
    else if (o.is_suffix())
      result_ = __builtin_addressof(suffix_);
    else
      result_ = current();
  }

  position_iterator position_;
  const value_type* result_ = nullptr;
  value_type suffix_;
  size_t N_ = 0;
  vector<int> subs_;
};

using cregex_token_iterator = regex_token_iterator<const char*>;
using wcregex_token_iterator = regex_token_iterator<const wchar_t*>;
using sregex_token_iterator = regex_token_iterator<string::const_iterator>;
using wsregex_token_iterator = regex_token_iterator<wstring::const_iterator>;

} // namespace std

namespace ycxx::detail {
// [re.alg.replace]/1
template <class OutputIterator, class BidirectionalIterator, class traits, class charT>
OutputIterator re_replace(OutputIterator out, BidirectionalIterator first, BidirectionalIterator last,
                          const std::basic_regex<charT, traits>& e, const charT* fmt_first, const charT* fmt_last,
                          std::regex_constants::match_flag_type flags) {
  std::regex_iterator<BidirectionalIterator, charT, traits> i(first, last, e, flags), end;
  const bool copy = !(flags & std::regex_constants::format_no_copy);
  auto put = [&out](BidirectionalIterator b, BidirectionalIterator e2) {
    for (; b != e2; ++b) {
      *out = *b;
      ++out;
    }
  };
  if (i == end) {
    if (copy)
      put(first, last);
    return out;
  }
  std::sub_match<BidirectionalIterator> tail;
  for (; i != end; ++i) {
    if (copy)
      put(i->prefix().first, i->prefix().second);
    out = i->format(out, fmt_first, fmt_last, flags);
    tail = i->suffix();
    if (flags & std::regex_constants::format_first_only)
      break;
  }
  if (copy)
    put(tail.first, tail.second);
  return out;
}
} // namespace ycxx::detail

namespace std {

// [re.alg.replace]
template <class OutputIterator, class BidirectionalIterator, class traits, class charT, class ST, class SA>
OutputIterator regex_replace(OutputIterator out, BidirectionalIterator first, BidirectionalIterator last,
                             const basic_regex<charT, traits>& e, const basic_string<charT, ST, SA>& fmt,
                             regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::ycxx::detail::re_replace(out, first, last, e, fmt.data(), fmt.data() + fmt.size(), flags);
}
template <class OutputIterator, class BidirectionalIterator, class traits, class charT>
OutputIterator regex_replace(OutputIterator out, BidirectionalIterator first, BidirectionalIterator last,
                             const basic_regex<charT, traits>& e, const charT* fmt,
                             regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::ycxx::detail::re_replace(out, first, last, e, fmt, fmt + char_traits<charT>::length(fmt), flags);
}
template <class traits, class charT, class ST, class SA, class FST, class FSA>
basic_string<charT, ST, SA> regex_replace(const basic_string<charT, ST, SA>& s, const basic_regex<charT, traits>& e,
                                          const basic_string<charT, FST, FSA>& fmt,
                                          regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<charT, ST, SA> result;
  std::regex_replace(back_inserter(result), s.begin(), s.end(), e, fmt, flags);
  return result;
}
template <class traits, class charT, class ST, class SA>
basic_string<charT, ST, SA> regex_replace(const basic_string<charT, ST, SA>& s, const basic_regex<charT, traits>& e,
                                          const charT* fmt,
                                          regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<charT, ST, SA> result;
  std::regex_replace(back_inserter(result), s.begin(), s.end(), e, fmt, flags);
  return result;
}
template <class traits, class charT, class ST, class SA>
basic_string<charT> regex_replace(const charT* s, const basic_regex<charT, traits>& e,
                                  const basic_string<charT, ST, SA>& fmt,
                                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<charT> result;
  std::regex_replace(back_inserter(result), s, s + char_traits<charT>::length(s), e, fmt, flags);
  return result;
}
template <class traits, class charT>
basic_string<charT> regex_replace(const charT* s, const basic_regex<charT, traits>& e, const charT* fmt,
                                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<charT> result;
  std::regex_replace(back_inserter(result), s, s + char_traits<charT>::length(s), e, fmt, flags);
  return result;
}

} // namespace std
