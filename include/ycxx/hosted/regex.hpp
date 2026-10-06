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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
struct __regex_access;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [re.regex]
template <class __charT, class __traits = regex_traits<__charT>>
class basic_regex {
public:
  using value_type = __charT;
  using traits_type = __traits;
  using string_type = typename __traits::string_type;
  using flag_type = regex_constants::syntax_option_type;
  using locale_type = typename __traits::locale_type;

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
  explicit basic_regex(const __charT* p, flag_type __f = regex_constants::ECMAScript) { assign(p, __f); }
  basic_regex(const __charT* p, size_t __len, flag_type __f = regex_constants::ECMAScript) { assign(p, __len, __f); }
  basic_regex(const basic_regex&) = default;
  basic_regex(basic_regex&& e) noexcept
      : __traits_(e.__traits_), __flags_(e.__flags_), __marks_(e.__marks_), __prog_(static_cast<shared_ptr<const __program>&&>(e.__prog_)) {}
  template <class _ST, class _SA>
  explicit basic_regex(const basic_string<__charT, _ST, _SA>& s, flag_type __f = regex_constants::ECMAScript) {
    assign(s, __f);
  }
  template <class _ForwardIterator>
  basic_regex(_ForwardIterator first, _ForwardIterator last, flag_type __f = regex_constants::ECMAScript) {
    assign(first, last, __f);
  }
  basic_regex(initializer_list<__charT> il, flag_type __f = regex_constants::ECMAScript) { assign(il.begin(), il.end(), __f); }
  ~basic_regex() = default;

  // [re.regex.assign]
  basic_regex& operator=(const basic_regex& e) = default;
  basic_regex& operator=(basic_regex&& e) noexcept {
    __traits_ = e.__traits_;
    __flags_ = e.__flags_;
    __marks_ = e.__marks_;
    __prog_ = static_cast<shared_ptr<const __program>&&>(e.__prog_);
    return *this;
  }
  basic_regex& operator=(const __charT* p) { return assign(p); }
  basic_regex& operator=(initializer_list<__charT> il) { return assign(il.begin(), il.end()); }
  template <class _ST, class _SA>
  basic_regex& operator=(const basic_string<__charT, _ST, _SA>& s) {
    return assign(s);
  }
  basic_regex& assign(const basic_regex& e) { return *this = e; }
  basic_regex& assign(basic_regex&& e) noexcept { return *this = static_cast<basic_regex&&>(e); }
  basic_regex& assign(const __charT* p, flag_type __f = regex_constants::ECMAScript) {
    return __compile(p, p + __traits::length(p), __f);
  }
  basic_regex& assign(const __charT* p, size_t __len, flag_type __f = regex_constants::ECMAScript) {
    return __compile(p, p + __len, __f);
  }
  template <class _ST, class _SA>
  basic_regex& assign(const basic_string<__charT, _ST, _SA>& s, flag_type __f = regex_constants::ECMAScript) {
    return __compile(s.data(), s.data() + s.size(), __f);
  }
  template <class _InputIterator>
  basic_regex& assign(_InputIterator first, _InputIterator last, flag_type __f = regex_constants::ECMAScript) {
    const string_type s(first, last);
    return __compile(s.data(), s.data() + s.size(), __f);
  }
  basic_regex& assign(initializer_list<__charT> il, flag_type __f = regex_constants::ECMAScript) {
    return __compile(il.begin(), il.end(), __f);
  }

  // [re.regex.operations]
  unsigned mark_count() const { return __marks_; }
  flag_type flags() const { return __flags_; }

  // [re.regex.locale]
  locale_type imbue(locale_type __loc) {
    locale_type __old = __traits_.imbue(__loc);
    __prog_.reset();
    __marks_ = 0;
    return __old;
  }
  locale_type getloc() const { return __traits_.getloc(); }

  // [re.regex.swap]
  void swap(basic_regex& e) {
    __traits t = __traits_;
    __traits_ = e.__traits_;
    e.__traits_ = t;
    const flag_type __f = __flags_;
    __flags_ = e.__flags_;
    e.__flags_ = __f;
    const unsigned m = __marks_;
    __marks_ = e.__marks_;
    e.__marks_ = m;
    __prog_.swap(e.__prog_);
  }

private:
  friend ::__ycxx::__detail::__regex_access;
  using __program = ::__ycxx::__detail::__re_program<__charT, __traits>;

  // Strong guarantee: *this changes only once the new program is built.
  basic_regex& __compile(const __charT* first, const __charT* last, flag_type __f) {
    shared_ptr<__program> p = std::make_shared<__program>();
    ::__ycxx::__detail::__re_compiler<__charT, __traits>(__traits_, *p).__compile(first, last, __f);
    __flags_ = __f;
    __marks_ = (__f & regex_constants::nosubs) ? 0u : static_cast<unsigned>(p->__groups);
    __prog_ = static_cast<shared_ptr<__program>&&>(p);
    return *this;
  }

  __traits __traits_;
  flag_type __flags_ = regex_constants::ECMAScript;
  unsigned __marks_ = 0;
  shared_ptr<const __program> __prog_;
};

template <class _ForwardIterator>
basic_regex(_ForwardIterator, _ForwardIterator, regex_constants::syntax_option_type = regex_constants::ECMAScript)
    -> basic_regex<typename iterator_traits<_ForwardIterator>::value_type>;

using regex = basic_regex<char>;
using wregex = basic_regex<wchar_t>;

// [re.regex.nonmemb]
template <class __charT, class __traits>
void swap(basic_regex<__charT, __traits>& __e1, basic_regex<__charT, __traits>& __e2) {
  __e1.swap(__e2);
}

// [re.submatch]
template <class _BidirectionalIterator>
class sub_match : public pair<_BidirectionalIterator, _BidirectionalIterator> {
  using base = pair<_BidirectionalIterator, _BidirectionalIterator>;

public:
  using value_type = typename iterator_traits<_BidirectionalIterator>::value_type;
  using difference_type = typename iterator_traits<_BidirectionalIterator>::difference_type;
  using iterator = _BidirectionalIterator;
  using string_type = basic_string<value_type>;

  bool matched;

  constexpr sub_match() : base(), matched() {}

  difference_type length() const { return matched ? std::distance(this->first, this->second) : 0; }
  operator string_type() const { return str(); }
  string_type str() const { return matched ? string_type(this->first, this->second) : string_type(); }
  int compare(const sub_match& s) const { return str().compare(s.str()); }
  int compare(const string_type& s) const { return str().compare(s); }
  int compare(const value_type* s) const { return str().compare(s); }
  void swap(sub_match& s) noexcept(is_nothrow_swappable_v<_BidirectionalIterator>) {
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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// SM-CAT(I) of [re.submatch.op]
template <class _BiIter>
using __regex_sm_cat_t = std::compare_three_way_result_t<std::basic_string<typename std::iterator_traits<_BiIter>::value_type>>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [re.submatch.op]

template <class _BiIter>
bool operator==(const sub_match<_BiIter>& __lhs, const sub_match<_BiIter>& __rhs) {
  return __lhs.compare(__rhs) == 0;
}
template <class _BiIter>
auto operator<=>(const sub_match<_BiIter>& __lhs, const sub_match<_BiIter>& __rhs) {
  return static_cast<::__ycxx::__detail::__regex_sm_cat_t<_BiIter>>(__lhs.compare(__rhs) <=> 0);
}
template <class _BiIter, class _ST, class _SA>
bool operator==(const sub_match<_BiIter>& __lhs, const basic_string<typename iterator_traits<_BiIter>::value_type, _ST, _SA>& __rhs) {
  return __lhs.compare(typename sub_match<_BiIter>::string_type(__rhs.data(), __rhs.size())) == 0;
}
template <class _BiIter, class _ST, class _SA>
auto operator<=>(const sub_match<_BiIter>& __lhs, const basic_string<typename iterator_traits<_BiIter>::value_type, _ST, _SA>& __rhs) {
  return static_cast<::__ycxx::__detail::__regex_sm_cat_t<_BiIter>>(__lhs.compare(typename sub_match<_BiIter>::string_type(__rhs.data(), __rhs.size())) <=> 0);
}
template <class _BiIter>
bool operator==(const sub_match<_BiIter>& __lhs, const typename iterator_traits<_BiIter>::value_type* __rhs) {
  return __lhs.compare(__rhs) == 0;
}
template <class _BiIter>
auto operator<=>(const sub_match<_BiIter>& __lhs, const typename iterator_traits<_BiIter>::value_type* __rhs) {
  return static_cast<::__ycxx::__detail::__regex_sm_cat_t<_BiIter>>(__lhs.compare(__rhs) <=> 0);
}
template <class _BiIter>
bool operator==(const sub_match<_BiIter>& __lhs, const typename iterator_traits<_BiIter>::value_type& __rhs) {
  return __lhs.compare(typename sub_match<_BiIter>::string_type(1, __rhs)) == 0;
}
template <class _BiIter>
auto operator<=>(const sub_match<_BiIter>& __lhs, const typename iterator_traits<_BiIter>::value_type& __rhs) {
  return static_cast<::__ycxx::__detail::__regex_sm_cat_t<_BiIter>>(__lhs.compare(typename sub_match<_BiIter>::string_type(1, __rhs)) <=> 0);
}
template <class __charT, class _ST, class _BiIter>
basic_ostream<__charT, _ST>& operator<<(basic_ostream<__charT, _ST>& __os, const sub_match<_BiIter>& m) {
  return __os << m.str();
}

// [re.results]
template <class _BidirectionalIterator, class _Allocator = allocator<sub_match<_BidirectionalIterator>>>
class match_results;
template <class _BidirectionalIterator, class _Allocator>
class match_results {
  using __vector_type = vector<sub_match<_BidirectionalIterator>, _Allocator>;

public:
  using value_type = sub_match<_BidirectionalIterator>;
  using const_reference = const value_type&;
  using reference = value_type&;
  using const_iterator = typename __vector_type::const_iterator;
  using iterator = const_iterator;
  using difference_type = typename iterator_traits<_BidirectionalIterator>::difference_type;
  using size_type = typename allocator_traits<_Allocator>::size_type;
  using allocator_type = _Allocator;
  using char_type = typename iterator_traits<_BidirectionalIterator>::value_type;
  using string_type = basic_string<char_type>;

  // [re.results.const]
  match_results() : match_results(_Allocator()) {}
  explicit match_results(const _Allocator& a) : __subs_(a) {}
  match_results(const match_results& m) = default;
  match_results(const match_results& m, const _Allocator& a)
      : __subs_(m.__subs_, a), __prefix_(m.__prefix_), __suffix_(m.__suffix_), __unmatched_(m.__unmatched_), __base_(m.__base_), __ready_(m.__ready_) {}
  match_results(match_results&& m) noexcept
      : __subs_(static_cast<__vector_type&&>(m.__subs_)), __prefix_(m.__prefix_), __suffix_(m.__suffix_), __unmatched_(m.__unmatched_),
        __base_(m.__base_), __ready_(m.__ready_) {}
  match_results(match_results&& m, const _Allocator& a)
      : __subs_(static_cast<__vector_type&&>(m.__subs_), a), __prefix_(m.__prefix_), __suffix_(m.__suffix_), __unmatched_(m.__unmatched_),
        __base_(m.__base_), __ready_(m.__ready_) {}
  match_results& operator=(const match_results& m) = default;
  match_results& operator=(match_results&& m) = default;
  ~match_results() = default;

  // [re.results.state]
  bool ready() const { return __ready_; }

  // [re.results.size]
  size_type size() const { return __subs_.size(); }
  size_type max_size() const { return __subs_.max_size(); }
  bool empty() const { return __subs_.empty(); }

  // [re.results.acc]
  difference_type length(size_type __sub = 0) const { return (*this)[__sub].length(); }
  difference_type position(size_type __sub = 0) const {
    ::__ycxx::__detail::__precondition(__ready_, "match_results::position: not ready");
    return std::distance(__base_, (*this)[__sub].first);
  }
  string_type str(size_type __sub = 0) const { return string_type((*this)[__sub]); }
  const_reference operator[](size_type n) const {
    ::__ycxx::__detail::__precondition(__ready_, "match_results::operator[]: not ready");
    return n < __subs_.size() ? __subs_[n] : __unmatched_;
  }
  const_reference prefix() const {
    ::__ycxx::__detail::__precondition(__ready_, "match_results::prefix: not ready");
    return __prefix_;
  }
  const_reference suffix() const {
    ::__ycxx::__detail::__precondition(__ready_, "match_results::suffix: not ready");
    return __suffix_;
  }
  const_iterator begin() const { return __subs_.begin(); }
  const_iterator end() const { return __subs_.end(); }
  const_iterator cbegin() const { return __subs_.begin(); }
  const_iterator cend() const { return __subs_.end(); }

  // [re.results.form]
  template <class _OutputIter>
  _OutputIter format(_OutputIter out, const char_type* __fmt_first, const char_type* __fmt_last,
                    regex_constants::match_flag_type flags = regex_constants::format_default) const {
    ::__ycxx::__detail::__precondition(__ready_, "match_results::format: not ready");
    auto put = [&out](const value_type& s) {
      if (s.matched)
        for (_BidirectionalIterator i = s.first; i != s.second; ++i) {
          *out = *i;
          ++out;
        }
    };
    auto digit = [](char_type c) { return c >= char_type('0') && c <= char_type('9') ? int(c - char_type('0')) : -1; };
    for (const char_type* p = __fmt_first; p != __fmt_last; ++p) {
      const char_type c = *p;
      if (flags & regex_constants::format_sed) {
        // sed: & is the whole match, \n the n-th subexpression, \c the character c.
        if (c == char_type('&')) {
          put((*this)[0]);
          continue;
        }
        if (c == char_type('\\') && p + 1 != __fmt_last) {
          ++p;
          if (digit(*p) >= 0) {
            put((*this)[static_cast<size_type>(digit(*p))]);
          } else {
            *out = *p;
            ++out;
          }
          continue;
        }
      } else if (c == char_type('$') && p + 1 != __fmt_last) {
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
          put(__prefix_);
          continue;
        }
        if (d == char_type('\'')) {
          ++p;
          put(__suffix_);
          continue;
        }
        if (digit(d) >= 0) {
          size_type n = static_cast<size_type>(digit(d));
          ++p;
          if (p + 1 != __fmt_last && digit(p[1]) >= 0) {
            const size_type __nn = n * 10 + static_cast<size_type>(digit(p[1]));
            if (__nn < size()) {
              n = __nn;
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
  template <class _OutputIter, class _ST, class _SA>
  _OutputIter format(_OutputIter out, const basic_string<char_type, _ST, _SA>& __fmt,
                    regex_constants::match_flag_type flags = regex_constants::format_default) const {
    return format(out, __fmt.data(), __fmt.data() + __fmt.size(), flags);
  }
  template <class _ST, class _SA>
  basic_string<char_type, _ST, _SA> format(const basic_string<char_type, _ST, _SA>& __fmt,
                                         regex_constants::match_flag_type flags = regex_constants::format_default) const {
    basic_string<char_type, _ST, _SA> result;
    format(std::back_inserter(result), __fmt, flags);
    return result;
  }
  string_type format(const char_type* __fmt, regex_constants::match_flag_type flags = regex_constants::format_default) const {
    string_type result;
    format(std::back_inserter(result), __fmt, __fmt + char_traits<char_type>::length(__fmt), flags);
    return result;
  }

  // [re.results.all]
  allocator_type get_allocator() const { return __subs_.get_allocator(); }

  // [re.results.swap]
  void swap(match_results& __that) {
    __subs_.swap(__that.__subs_);
    __prefix_.swap(__that.__prefix_);
    __suffix_.swap(__that.__suffix_);
    __unmatched_.swap(__that.__unmatched_);
    const _BidirectionalIterator b = __base_;
    __base_ = __that.__base_;
    __that.__base_ = b;
    const bool r = __ready_;
    __ready_ = __that.__ready_;
    __that.__ready_ = r;
  }

private:
  friend ::__ycxx::__detail::__regex_access;

  __vector_type __subs_;
  value_type __prefix_, __suffix_, __unmatched_;
  _BidirectionalIterator __base_{};
  bool __ready_ = false;
};

using cmatch = match_results<const char*>;
using wcmatch = match_results<const wchar_t*>;
using smatch = match_results<string::const_iterator>;
using wsmatch = match_results<wstring::const_iterator>;

// [re.results.nonmember]
template <class _BidirectionalIterator, class _Allocator>
bool operator==(const match_results<_BidirectionalIterator, _Allocator>& __m1,
                const match_results<_BidirectionalIterator, _Allocator>& __m2) {
  if (!__m1.ready() || !__m2.ready())
    return __m1.ready() == __m2.ready();
  if (__m1.empty() || __m2.empty())
    return __m1.empty() && __m2.empty();
  if (!(__m1.prefix() == __m2.prefix()) || __m1.size() != __m2.size() || !(__m1.suffix() == __m2.suffix()))
    return false;
  for (typename match_results<_BidirectionalIterator, _Allocator>::size_type i = 0; i < __m1.size(); ++i)
    if (!(__m1[i] == __m2[i]))
      return false;
  return true;
}

// [re.results.swap]
template <class _BidirectionalIterator, class _Allocator>
void swap(match_results<_BidirectionalIterator, _Allocator>& __m1, match_results<_BidirectionalIterator, _Allocator>& __m2) {
  __m1.swap(__m2);
}

namespace pmr {
template <class _BidirectionalIterator>
using match_results = std::match_results<_BidirectionalIterator, polymorphic_allocator<sub_match<_BidirectionalIterator>>>;
using cmatch = match_results<const char*>;
using wcmatch = match_results<const wchar_t*>;
using smatch = match_results<string::const_iterator>;
using wsmatch = match_results<wstring::const_iterator>;
} // namespace pmr

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __regex_access {
  // Matches e against [first, last) (whole: regex_match) and fills m; positions count from base.
  template <class _It, class _Alloc, class __charT, class __traits>
  static bool run(_It first, _It last, std::match_results<_It, _Alloc>& m, const std::basic_regex<__charT, __traits>& e,
                  std::regex_constants::match_flag_type flags, bool __whole, _It base) {
    m.__ready_ = true;
    m.__base_ = base;
    m.__subs_.clear();
    m.__unmatched_.first = m.__unmatched_.second = last;
    m.__unmatched_.matched = false;
    m.__prefix_ = m.__unmatched_;
    m.__suffix_ = m.__unmatched_;
    if (!e.__prog_)
      return false;
    std::vector<__re_cap<_It>> __caps;
    if (!::__ycxx::__detail::__re_execute(*e.__prog_, e.__traits_, first, last, flags, __whole, __caps))
      return false;
    const std::size_t n = (e.__flags_ & std::regex_constants::nosubs) ? 1 : __caps.size();
    m.__subs_.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
      m.__subs_[i].first = __caps[i].first;
      m.__subs_[i].second = __caps[i].second;
      m.__subs_[i].matched = __caps[i].matched;
    }
    m.__prefix_.first = first;
    m.__prefix_.second = __caps[0].first;
    m.__prefix_.matched = m.__prefix_.first != m.__prefix_.second;
    m.__suffix_.first = __caps[0].second;
    m.__suffix_.second = last;
    m.__suffix_.matched = m.__suffix_.first != m.__suffix_.second;
    return true;
  }
  // regex_iterator: the prefix of a match found after an earlier one starts where that one ended.
  template <class _It, class _Alloc>
  static void __set_prefix_first(std::match_results<_It, _Alloc>& m, _It first) {
    m.__prefix_.first = first;
    m.__prefix_.matched = m.__prefix_.first != m.__prefix_.second;
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [re.alg.match]
template <class _BidirectionalIterator, class _Allocator, class __charT, class __traits>
bool regex_match(_BidirectionalIterator first, _BidirectionalIterator last, match_results<_BidirectionalIterator, _Allocator>& m,
                 const basic_regex<__charT, __traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::__ycxx::__detail::__regex_access::run(first, last, m, e, flags, true, first);
}
template <class _BidirectionalIterator, class __charT, class __traits>
bool regex_match(_BidirectionalIterator first, _BidirectionalIterator last, const basic_regex<__charT, __traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  match_results<_BidirectionalIterator> what;
  return ::__ycxx::__detail::__regex_access::run(first, last, what, e, flags, true, first);
}
template <class __charT, class _Allocator, class __traits>
bool regex_match(const __charT* str, match_results<const __charT*, _Allocator>& m, const basic_regex<__charT, __traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(str, str + char_traits<__charT>::length(str), m, e, flags);
}
template <class _ST, class _SA, class _Allocator, class __charT, class __traits>
bool regex_match(const basic_string<__charT, _ST, _SA>& s,
                 match_results<typename basic_string<__charT, _ST, _SA>::const_iterator, _Allocator>& m,
                 const basic_regex<__charT, __traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(s.begin(), s.end(), m, e, flags);
}
template <class _ST, class _SA, class _Allocator, class __charT, class __traits>
bool regex_match(const basic_string<__charT, _ST, _SA>&&,
                 match_results<typename basic_string<__charT, _ST, _SA>::const_iterator, _Allocator>&,
                 const basic_regex<__charT, __traits>&, regex_constants::match_flag_type = regex_constants::match_default) = delete;
template <class __charT, class __traits>
bool regex_match(const __charT* str, const basic_regex<__charT, __traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(str, str + char_traits<__charT>::length(str), e, flags);
}
template <class _ST, class _SA, class __charT, class __traits>
bool regex_match(const basic_string<__charT, _ST, _SA>& s, const basic_regex<__charT, __traits>& e,
                 regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_match(s.begin(), s.end(), e, flags);
}

// [re.alg.search]
template <class _BidirectionalIterator, class _Allocator, class __charT, class __traits>
bool regex_search(_BidirectionalIterator first, _BidirectionalIterator last, match_results<_BidirectionalIterator, _Allocator>& m,
                  const basic_regex<__charT, __traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::__ycxx::__detail::__regex_access::run(first, last, m, e, flags, false, first);
}
template <class _BidirectionalIterator, class __charT, class __traits>
bool regex_search(_BidirectionalIterator first, _BidirectionalIterator last, const basic_regex<__charT, __traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  match_results<_BidirectionalIterator> what;
  return ::__ycxx::__detail::__regex_access::run(first, last, what, e, flags, false, first);
}
template <class __charT, class _Allocator, class __traits>
bool regex_search(const __charT* str, match_results<const __charT*, _Allocator>& m, const basic_regex<__charT, __traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(str, str + char_traits<__charT>::length(str), m, e, flags);
}
template <class __charT, class __traits>
bool regex_search(const __charT* str, const basic_regex<__charT, __traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(str, str + char_traits<__charT>::length(str), e, flags);
}
template <class _ST, class _SA, class __charT, class __traits>
bool regex_search(const basic_string<__charT, _ST, _SA>& s, const basic_regex<__charT, __traits>& e,
                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(s.begin(), s.end(), e, flags);
}
template <class _ST, class _SA, class _Allocator, class __charT, class __traits>
bool regex_search(const basic_string<__charT, _ST, _SA>& s,
                  match_results<typename basic_string<__charT, _ST, _SA>::const_iterator, _Allocator>& m,
                  const basic_regex<__charT, __traits>& e, regex_constants::match_flag_type flags = regex_constants::match_default) {
  return std::regex_search(s.begin(), s.end(), m, e, flags);
}
template <class _ST, class _SA, class _Allocator, class __charT, class __traits>
bool regex_search(const basic_string<__charT, _ST, _SA>&&,
                  match_results<typename basic_string<__charT, _ST, _SA>::const_iterator, _Allocator>&,
                  const basic_regex<__charT, __traits>&, regex_constants::match_flag_type = regex_constants::match_default) = delete;

// [re.regiter]
template <class _BidirectionalIterator, class __charT = typename iterator_traits<_BidirectionalIterator>::value_type,
          class __traits = regex_traits<__charT>>
class regex_iterator {
public:
  using regex_type = basic_regex<__charT, __traits>;
  using iterator_category = forward_iterator_tag;
  using iterator_concept = input_iterator_tag;
  using value_type = match_results<_BidirectionalIterator>;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  regex_iterator() = default;
  regex_iterator(_BidirectionalIterator a, _BidirectionalIterator b, const regex_type& __re,
                 regex_constants::match_flag_type m = regex_constants::match_default)
      : __begin_(a), __end_(b), __pregex_(__builtin_addressof(__re)), __flags_(m) {
    if (!::__ycxx::__detail::__regex_access::run(__begin_, __end_, __match_, *__pregex_, __flags_, false, __begin_))
      __pregex_ = nullptr;
  }
  regex_iterator(_BidirectionalIterator, _BidirectionalIterator, const regex_type&&,
                 regex_constants::match_flag_type = regex_constants::match_default) = delete;
  regex_iterator(const regex_iterator&) = default;
  regex_iterator& operator=(const regex_iterator&) = default;

  bool operator==(const regex_iterator& right) const {
    if (__pregex_ == nullptr || right.__pregex_ == nullptr)
      return __pregex_ == right.__pregex_;
    return __begin_ == right.__begin_ && __end_ == right.__end_ && __pregex_ == right.__pregex_ && __flags_ == right.__flags_ &&
           __match_[0] == right.__match_[0];
  }
  bool operator==(default_sentinel_t) const noexcept { return __pregex_ == nullptr; }
  const value_type& operator*() const {
    ::__ycxx::__detail::__precondition(__pregex_ != nullptr, "regex_iterator: dereferencing the end-of-sequence iterator");
    return __match_;
  }
  const value_type* operator->() const {
    ::__ycxx::__detail::__precondition(__pregex_ != nullptr, "regex_iterator: dereferencing the end-of-sequence iterator");
    return __builtin_addressof(__match_);
  }

  // [re.regiter.incr]
  regex_iterator& operator++() {
    namespace __rc = regex_constants;
    using access = ::__ycxx::__detail::__regex_access;
    ::__ycxx::__detail::__precondition(__pregex_ != nullptr, "regex_iterator: incrementing the end-of-sequence iterator");
    _BidirectionalIterator start = __match_[0].second;
    const _BidirectionalIterator __prev_end = start;
    if (__match_[0].first == __match_[0].second) {
      if (start == __end_) {
        *this = regex_iterator();
        return *this;
      }
      const __rc::match_flag_type __f =
          __flags_ | __rc::match_not_null | __rc::match_continuous | (start != __begin_ ? __rc::match_prev_avail : __rc::match_default);
      if (access::run(start, __end_, __match_, *__pregex_, __f, false, __begin_)) {
        access::__set_prefix_first(__match_, __prev_end);
        return *this;
      }
      ++start;
    }
    __flags_ = __flags_ | __rc::match_prev_avail;
    if (!access::run(start, __end_, __match_, *__pregex_, __flags_, false, __begin_)) {
      *this = regex_iterator();
      return *this;
    }
    access::__set_prefix_first(__match_, __prev_end);
    return *this;
  }
  regex_iterator operator++(int) {
    regex_iterator t = *this;
    ++*this;
    return t;
  }

private:
  _BidirectionalIterator __begin_{}, __end_{};
  const regex_type* __pregex_ = nullptr;
  regex_constants::match_flag_type __flags_{};
  match_results<_BidirectionalIterator> __match_;
};

using cregex_iterator = regex_iterator<const char*>;
using wcregex_iterator = regex_iterator<const wchar_t*>;
using sregex_iterator = regex_iterator<string::const_iterator>;
using wsregex_iterator = regex_iterator<wstring::const_iterator>;

// [re.tokiter]
template <class _BidirectionalIterator, class __charT = typename iterator_traits<_BidirectionalIterator>::value_type,
          class __traits = regex_traits<__charT>>
class regex_token_iterator {
public:
  using regex_type = basic_regex<__charT, __traits>;
  using iterator_category = forward_iterator_tag;
  using iterator_concept = input_iterator_tag;
  using value_type = sub_match<_BidirectionalIterator>;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  regex_token_iterator() = default;
  regex_token_iterator(_BidirectionalIterator a, _BidirectionalIterator b, const regex_type& __re, int __submatch = 0,
                       regex_constants::match_flag_type m = regex_constants::match_default)
      : __subs_(1, __submatch) {
    init(a, b, __re, m);
  }
  regex_token_iterator(_BidirectionalIterator a, _BidirectionalIterator b, const regex_type& __re,
                       const vector<int>& __submatches, regex_constants::match_flag_type m = regex_constants::match_default)
      : __subs_(__submatches) {
    init(a, b, __re, m);
  }
  regex_token_iterator(_BidirectionalIterator a, _BidirectionalIterator b, const regex_type& __re,
                       initializer_list<int> __submatches, regex_constants::match_flag_type m = regex_constants::match_default)
      : __subs_(__submatches) {
    init(a, b, __re, m);
  }
  template <size_t _Np>
  regex_token_iterator(_BidirectionalIterator a, _BidirectionalIterator b, const regex_type& __re, const int (&__submatches)[_Np],
                       regex_constants::match_flag_type m = regex_constants::match_default)
      : __subs_(__submatches, __submatches + _Np) {
    init(a, b, __re, m);
  }
  regex_token_iterator(_BidirectionalIterator, _BidirectionalIterator, const regex_type&&, int = 0,
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;
  regex_token_iterator(_BidirectionalIterator, _BidirectionalIterator, const regex_type&&, const vector<int>&,
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;
  regex_token_iterator(_BidirectionalIterator, _BidirectionalIterator, const regex_type&&, initializer_list<int>,
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;
  template <size_t _Np>
  regex_token_iterator(_BidirectionalIterator, _BidirectionalIterator, const regex_type&&, const int (&)[_Np],
                       regex_constants::match_flag_type = regex_constants::match_default) = delete;

  regex_token_iterator(const regex_token_iterator& __o)
      : __position_(__o.__position_), __suffix_(__o.__suffix_), _N_(__o._N_), __subs_(__o.__subs_) {
    rebind(__o);
  }
  regex_token_iterator& operator=(const regex_token_iterator& __o) {
    if (this != __builtin_addressof(__o)) {
      __position_ = __o.__position_;
      __suffix_ = __o.__suffix_;
      _N_ = __o._N_;
      __subs_ = __o.__subs_;
      rebind(__o);
    }
    return *this;
  }

  // [re.tokiter.comp]
  bool operator==(const regex_token_iterator& right) const {
    if (__result_ == nullptr || right.__result_ == nullptr)
      return __result_ == nullptr && right.__result_ == nullptr;
    const bool __s1 = __is_suffix(), __s2 = right.__is_suffix();
    if (__s1 || __s2)
      return __s1 && __s2 && __suffix_ == right.__suffix_;
    return __position_ == right.__position_ && _N_ == right._N_ && __subs_ == right.__subs_;
  }
  bool operator==(default_sentinel_t) const noexcept { return __result_ == nullptr; }

  // [re.tokiter.deref]
  const value_type& operator*() const {
    ::__ycxx::__detail::__precondition(__result_ != nullptr, "regex_token_iterator: dereferencing the end-of-sequence iterator");
    return *__result_;
  }
  const value_type* operator->() const {
    ::__ycxx::__detail::__precondition(__result_ != nullptr, "regex_token_iterator: dereferencing the end-of-sequence iterator");
    return __result_;
  }

  // [re.tokiter.incr]
  regex_token_iterator& operator++() {
    ::__ycxx::__detail::__precondition(__result_ != nullptr, "regex_token_iterator: incrementing the end-of-sequence iterator");
    const __position_iterator prev = __position_;
    if (__is_suffix()) {
      __result_ = nullptr;
      return *this;
    }
    if (_N_ + 1 < __subs_.size()) {
      ++_N_;
      __result_ = current();
      return *this;
    }
    _N_ = 0;
    ++__position_;
    if (__position_ != __position_iterator()) {
      __result_ = current();
      return *this;
    }
    if (__wants_suffix() && prev->suffix().length() != 0) {
      __suffix_.first = prev->suffix().first;
      __suffix_.second = prev->suffix().second;
      __suffix_.matched = true;
      __result_ = __builtin_addressof(__suffix_);
      return *this;
    }
    __result_ = nullptr;
    return *this;
  }
  regex_token_iterator operator++(int) {
    regex_token_iterator t = *this;
    ++*this;
    return t;
  }

private:
  using __position_iterator = regex_iterator<_BidirectionalIterator, __charT, __traits>;

  bool __is_suffix() const { return __result_ == __builtin_addressof(__suffix_); }
  bool __wants_suffix() const {
    for (int s : __subs_)
      if (s == -1)
        return true;
    return false;
  }
  const value_type* current() const {
    const int s = __subs_[_N_];
    return s == -1 ? __builtin_addressof(__position_->prefix())
                   : __builtin_addressof((*__position_)[static_cast<typename match_results<_BidirectionalIterator>::size_type>(s)]);
  }
  void init(_BidirectionalIterator a, _BidirectionalIterator b, const regex_type& __re, regex_constants::match_flag_type m) {
    for (int s : __subs_)
      ::__ycxx::__detail::__precondition(s >= -1, "regex_token_iterator: a submatch index below -1");
    __position_ = __position_iterator(a, b, __re, m);
    _N_ = 0;
    if (__position_ != __position_iterator()) {
      __result_ = current();
    } else if (__wants_suffix()) {
      __suffix_.first = a;
      __suffix_.second = b;
      __suffix_.matched = true;
      __result_ = __builtin_addressof(__suffix_);
    }
  }
  // After a copy: point into this object's own position or suffix.
  void rebind(const regex_token_iterator& __o) {
    if (__o.__result_ == nullptr)
      __result_ = nullptr;
    else if (__o.__is_suffix())
      __result_ = __builtin_addressof(__suffix_);
    else
      __result_ = current();
  }

  __position_iterator __position_;
  const value_type* __result_ = nullptr;
  value_type __suffix_;
  size_t _N_ = 0;
  vector<int> __subs_;
};

using cregex_token_iterator = regex_token_iterator<const char*>;
using wcregex_token_iterator = regex_token_iterator<const wchar_t*>;
using sregex_token_iterator = regex_token_iterator<string::const_iterator>;
using wsregex_token_iterator = regex_token_iterator<wstring::const_iterator>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// [re.alg.replace]/1
template <class _OutputIterator, class _BidirectionalIterator, class __traits, class __charT>
_OutputIterator __re_replace(_OutputIterator out, _BidirectionalIterator first, _BidirectionalIterator last,
                          const std::basic_regex<__charT, __traits>& e, const __charT* __fmt_first, const __charT* __fmt_last,
                          std::regex_constants::match_flag_type flags) {
  std::regex_iterator<_BidirectionalIterator, __charT, __traits> i(first, last, e, flags), end;
  const bool copy = !(flags & std::regex_constants::format_no_copy);
  auto put = [&out](_BidirectionalIterator b, _BidirectionalIterator __e2) {
    for (; b != __e2; ++b) {
      *out = *b;
      ++out;
    }
  };
  if (i == end) {
    if (copy)
      put(first, last);
    return out;
  }
  std::sub_match<_BidirectionalIterator> __tail;
  for (; i != end; ++i) {
    if (copy)
      put(i->prefix().first, i->prefix().second);
    out = i->format(out, __fmt_first, __fmt_last, flags);
    __tail = i->suffix();
    if (flags & std::regex_constants::format_first_only)
      break;
  }
  if (copy)
    put(__tail.first, __tail.second);
  return out;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [re.alg.replace]
template <class _OutputIterator, class _BidirectionalIterator, class __traits, class __charT, class _ST, class _SA>
_OutputIterator regex_replace(_OutputIterator out, _BidirectionalIterator first, _BidirectionalIterator last,
                             const basic_regex<__charT, __traits>& e, const basic_string<__charT, _ST, _SA>& __fmt,
                             regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::__ycxx::__detail::__re_replace(out, first, last, e, __fmt.data(), __fmt.data() + __fmt.size(), flags);
}
template <class _OutputIterator, class _BidirectionalIterator, class __traits, class __charT>
_OutputIterator regex_replace(_OutputIterator out, _BidirectionalIterator first, _BidirectionalIterator last,
                             const basic_regex<__charT, __traits>& e, const __charT* __fmt,
                             regex_constants::match_flag_type flags = regex_constants::match_default) {
  return ::__ycxx::__detail::__re_replace(out, first, last, e, __fmt, __fmt + char_traits<__charT>::length(__fmt), flags);
}
template <class __traits, class __charT, class _ST, class _SA, class _FST, class _FSA>
basic_string<__charT, _ST, _SA> regex_replace(const basic_string<__charT, _ST, _SA>& s, const basic_regex<__charT, __traits>& e,
                                          const basic_string<__charT, _FST, _FSA>& __fmt,
                                          regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<__charT, _ST, _SA> result;
  std::regex_replace(std::back_inserter(result), s.begin(), s.end(), e, __fmt, flags);
  return result;
}
template <class __traits, class __charT, class _ST, class _SA>
basic_string<__charT, _ST, _SA> regex_replace(const basic_string<__charT, _ST, _SA>& s, const basic_regex<__charT, __traits>& e,
                                          const __charT* __fmt,
                                          regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<__charT, _ST, _SA> result;
  std::regex_replace(std::back_inserter(result), s.begin(), s.end(), e, __fmt, flags);
  return result;
}
template <class __traits, class __charT, class _ST, class _SA>
basic_string<__charT> regex_replace(const __charT* s, const basic_regex<__charT, __traits>& e,
                                  const basic_string<__charT, _ST, _SA>& __fmt,
                                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<__charT> result;
  std::regex_replace(std::back_inserter(result), s, s + char_traits<__charT>::length(s), e, __fmt, flags);
  return result;
}
template <class __traits, class __charT>
basic_string<__charT> regex_replace(const __charT* s, const basic_regex<__charT, __traits>& e, const __charT* __fmt,
                                  regex_constants::match_flag_type flags = regex_constants::match_default) {
  basic_string<__charT> result;
  std::regex_replace(std::back_inserter(result), s, s + char_traits<__charT>::length(s), e, __fmt, flags);
  return result;
}

} // namespace std
