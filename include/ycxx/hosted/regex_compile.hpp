// libycxx hosted: the regular expression compiler. The six grammars of [re.synopt] are parsed
// into one syntax tree (re_node), which is then turned into a program (re_program) for one of the
// matchers of ycxx/hosted/regex_engine.hpp:
//
// - ECMAScript ([re.grammar]) and POSIX patterns with back-references compile for the
//   backtracking matcher. Quantified atoms keep a counter (loop_enter / loop_iter / loop_tail),
//   which gives ECMA-262's RepeatMatcher semantics: the captures inside the atom are cleared at
//   the start of every iteration, and an optional iteration that matches the empty string fails.
//   A quantified single character, set or '.' is one `rep` instruction that consumes greedily or
//   lazily without a frame per character.
// - The POSIX grammars without back-references compile to a Thompson NFA for the leftmost-longest
//   simulation: bounded repetitions are expanded in the tree first (x{2,3} is x x (x)?), so only
//   ? and * remain. Every node's code is a contiguous range [node_begin, node_end) left only by
//   reaching node_end, which the POSIX submatch resolver relies on.
//
// The translator calls only traits members for anything locale-dependent ([re.grammar]/2).
#pragma once

#include <ycxx/core/vector.hpp>
#include <ycxx/hosted/regex_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

enum class __re_kind : unsigned char {
  empty, __chr, any, set, __bol, __eol, __wordb, __nwordb, __backref, __group, __look, __nlook, concat, __alt, repeat
};

template <class __charT>
struct __re_node {
  __re_kind kind = __re_kind::empty;
  bool __greedy = true;
  bool __tail = false;      // POSIX expansion: a repetition that follows an iteration of its own
  bool __reset = false;     // POSIX expansion: a later copy of a repeated atom (its groups, [group_lo,
                            // group_hi), restart: they report the last iteration)
  __charT __ch{};
  int __val = 0;            // set index; group or back-reference number
  int min = 0, max = 0;   // repeat; max < 0: unbounded
  int __group_lo = 0, __group_hi = 0; // repeat and group: the groups numbered inside, [lo, hi)
  std::vector<int> __kids;
};

// The order of a character code: the unsigned value of an integral character type.
template <class __charT>
constexpr auto __re_ord(__charT c) noexcept {
  if constexpr (std::is_integral_v<__charT>)
    return static_cast<std::make_unsigned_t<__charT>>(c);
  else
    return c;
}
// Code units below 256 have precomputed answers (case folding, set membership, word class).
template <class __charT>
inline constexpr bool __re_cacheable = std::is_integral_v<__charT>;

template <class __charT>
constexpr bool __re_line_terminator(__charT c) noexcept {
  if (c == __charT('\n') || c == __charT('\r'))
    return true;
  if constexpr (std::is_integral_v<__charT> && sizeof(__charT) > 1)
    return __re_ord(c) == 0x2028u || __re_ord(c) == 0x2029u;
  else
    return false;
}

template <class __charT, class __traits>
struct __re_set {
  using string_type = typename __traits::string_type;
  using __class_type = typename __traits::char_class_type;
  struct range {
    __charT __lo, __hi;
  };
  bool negate = false;
  std::basic_string<__charT> __chars;  // translated
  std::vector<range> ranges;
  std::vector<string_type> __coll_lo, __coll_hi;  // collate: sort keys of the range ends
  __class_type __classes{};
  std::vector<__class_type> __neg_classes;        // \D, \S, \W inside brackets
  std::vector<string_type> __multi;               // multi-character collating elements (the
                                                // translator turns them into alternatives)
  std::vector<string_type> __equivs;            // primary sort keys
  unsigned char __cache[32] = {};               // membership of the code units below 256
  unsigned char __range_fold[32] = {};          // icase: the case-folded code units below 256 of
                                              // the characters of the ranges
};

enum class __re_op : unsigned char {
  __chr, any, set, split, __jmp, open, close, __bol, __eol, __wordb, __nwordb, __backref, __look, __look_end,
  __loop_enter, __loop_iter, __loop_tail, rep, __match
};

template <class __charT>
struct __re_inst {
  __re_op op;
  bool __flag = false; // any: stops at line terminators; look: negative
  __charT __ch{};        // chr: the translated character
  int a = 0, b = 0;  // split: preferred, other; jmp: target; set: index; open/close/backref: group;
                     // look: its look_end; loop_*, rep: loop index
};

struct __re_loop {
  int min = 0, max = 0; // max < 0: unbounded
  bool __greedy = true;
  int __iter_pc = 0, __exit_pc = 0;
  int __group_lo = 0, __group_hi = 0;
};

template <class __charT, class __traits>
struct __re_program {
  using __set_type = __re_set<__charT, __traits>;
  using __class_type = typename __traits::char_class_type;

  std::regex_constants::syntax_option_type flags{};
  bool icase = false, collate = false, multiline = false, __ecma = true;
  bool posix = false;   // leftmost-longest
  bool __nfa = false;     // compiled for the NFA simulation (POSIX without back-references)
  bool __has_backref = false;
  bool __needs_bt = false; // POSIX: a lookahead (a non-matching list with a multi-character
                           // collating element) keeps the program off the NFA
  bool __memo = false;    // the backtracker may remember failed (pc, position) pairs
  int __groups = 0;       // capturing groups (also counted under nosubs, for back-references)
  std::vector<__re_inst<__charT>> code;
  std::vector<__re_loop> __loops;
  std::vector<__set_type> __sets;
  // nfa: the expanded tree and each node's code range; the epsilon predecessors of each pc.
  std::vector<__re_node<__charT>> __nodes;
  int __root = -1;
  std::vector<int> __node_begin, __node_end;
  std::vector<std::vector<int>> __eps_pred;
  // POSIX backtracking programs (the subexpression search of regex_engine.hpp): per node of the
  // unexpanded tree, the shortest and longest string it can match (-1: unbounded), its flags,
  // and for a concatenation the bounds of each suffix of its elements (at suf_off[n] + i: the
  // elements i...; one past the last is 0).
  static constexpr unsigned char __nf_commit = 1; // no back-reference outside refers to a group inside
  static constexpr unsigned char __nf_pure = 2;   // no back-reference inside
  std::vector<std::ptrdiff_t> __nmin, __nmax;
  std::vector<unsigned char> __nflags;
  std::vector<int> __suf_off;
  std::vector<std::ptrdiff_t> __suf_min, __suf_max;
  // memo: the dense index of each pc where a failed (pc, position) pair is remembered, or -1.
  std::vector<int> __memo_index;
  int __memo_points = 0;
  std::vector<int> __memo_loops;        // the loops whose body is nullable (at most 4)
  std::vector<unsigned> __memo_mask;    // per memo point: bit k if inside an iteration of memo_loops[k]
  // Caches for the code units below 256.
  __charT __fold[256] = {};
  unsigned char __word[32] = {};
  __class_type __word_class{};

  static bool __bit(const unsigned char* bits, unsigned __u) noexcept { return (bits[__u >> 3] >> (__u & 7)) & 1; }

  // The character as compared: [re.grammar]/14.1.
  __charT __tx(const __traits& __tr, __charT c) const {
    if constexpr (__re_cacheable<__charT>) {
      if (__re_ord(c) < 256u)
        return __fold[__re_ord(c)];
    }
    return icase ? __tr.translate_nocase(c) : collate ? __tr.translate(c) : c;
  }
  bool __is_word(const __traits& __tr, __charT c) const {
    if constexpr (__re_cacheable<__charT>) {
      if (__re_ord(c) < 256u)
        return __bit(__word, unsigned(__re_ord(c)));
    }
    return __tr.isctype(c, __word_class);
  }
  bool __set_slow(const __traits& __tr, const __set_type& s, __charT c) const {
    const __charT t = icase ? __tr.translate_nocase(c) : collate ? __tr.translate(c) : c;
    bool in = s.__chars.find(t) != std::basic_string<__charT>::npos;
    for (std::size_t i = 0; !in && i < s.ranges.size(); ++i) {
      const auto& r = s.ranges[i];
      in = (__re_ord(r.__lo) <= __re_ord(c) && __re_ord(c) <= __re_ord(r.__hi)) ||
           (icase && __re_ord(r.__lo) <= __re_ord(t) && __re_ord(t) <= __re_ord(r.__hi));
    }
    // icase: c is in a range if some character of the range folds to what c folds to ([T-f]
    // holds 't', since it holds 'T'); exact for the folded values below 256.
    if constexpr (__re_cacheable<__charT>) {
      if (!in && icase && __re_ord(t) < 256u)
        in = __bit(s.__range_fold, unsigned(__re_ord(t)));
    }
    // [re.grammar]/14.2: collating ranges compare sort keys; without regard to case, the folded
    // character is tried too.
    for (int __pass = 0; !in && !s.__coll_lo.empty() && __pass < (icase ? 2 : 1); ++__pass) {
      const __charT k[1] = {__pass == 0 ? __tr.translate(c) : t};
      const auto key = __tr.transform(k, k + 1);
      for (std::size_t i = 0; !in && i < s.__coll_lo.size(); ++i)
        in = !(key < s.__coll_lo[i]) && !(s.__coll_hi[i] < key);
    }
    if (!in && s.__classes != __class_type{})
      in = __tr.isctype(c, s.__classes);
    for (std::size_t i = 0; !in && i < s.__neg_classes.size(); ++i)
      in = !__tr.isctype(c, s.__neg_classes[i]);
    if (!in && !s.__equivs.empty()) {
      const __charT k[1] = {c};
      const auto key = __tr.transform_primary(k, k + 1);
      for (std::size_t i = 0; !in && i < s.__equivs.size(); ++i)
        in = key == s.__equivs[i];
    }
    return in != s.negate;
  }
  bool __in_set(const __traits& __tr, int __idx, __charT c) const {
    const __set_type& s = __sets[static_cast<std::size_t>(__idx)];
    if constexpr (__re_cacheable<__charT>) {
      if (__re_ord(c) < 256u)
        return __bit(s.__cache, unsigned(__re_ord(c)));
    }
    return __set_slow(__tr, s, c);
  }
  // Does the single-character instruction at pc accept c?
  bool single(const __traits& __tr, int __pc, __charT c) const {
    const __re_inst<__charT>& in = code[static_cast<std::size_t>(__pc)];
    switch (in.op) {
    case __re_op::__chr:
      return __tx(__tr, c) == in.__ch;
    case __re_op::any:
      return !in.__flag || !::__ycxx::__detail::__re_line_terminator(c);
    default:
      return __in_set(__tr, in.a, c);
    }
  }
};

// ---- the translator ---------------------------------------------------------------------------
template <class __charT, class __traits>
class __re_compiler {
  using __rc_err = std::regex_constants::error_type;
  using string_type = typename __traits::string_type;
  using __class_type = typename __traits::char_class_type;
  using node = __re_node<__charT>;
  using __set_type = __re_set<__charT, __traits>;

  // Limits on the size of a translated expression (error_space beyond them): the tree and the
  // code, and the nesting of groups (the translator and the matchers recurse over the tree).
  static constexpr std::size_t __max_nodes = 1u << 20;
  static constexpr int __max_depth = 1000;
  static constexpr int __max_count = 0x7fffffff;
  // POSIX: bounded repetitions are expanded for the NFA only up to this many nodes in all and
  // this many copies of one atom; beyond, the program backtracks (keeping the longest match).
  static constexpr std::size_t __max_expanded = 1u << 16;
  static constexpr int __max_copies = 256;

  const __traits& __tr_;
  __re_program<__charT, __traits>& _P_;
  const __charT* __p_;
  const __charT* __end_;
  std::vector<node> __nodes_;
  std::vector<char> __closed_; // POSIX: closed_[n] once group n is complete (not vector<bool>, which <vector> specializes)
  int __max_backref_ = 0;
  int __depth_ = 0;
  enum __grammar { __ecma, __bre, __ere, __awk_g } __g_ = __ecma;

  [[noreturn]] static void fail(__rc_err e) { ::__ycxx::__detail::__throw_regex_error(e); }
  void __enter() {
    if (++__depth_ > __max_depth)
      fail(std::regex_constants::error_space);
  }

  bool at(char c) const { return __p_ != __end_ && *__p_ == static_cast<__charT>(c); }
  bool __at2(char c, char d) const { return at(c) && __p_ + 1 != __end_ && __p_[1] == static_cast<__charT>(d); }
  int digit(__charT c, int radix) const { return __tr_.value(c, radix); }

  int add(node n) {
    if (__nodes_.size() >= __max_nodes)
      fail(std::regex_constants::error_space);
    __nodes_.push_back(static_cast<node&&>(n));
    return static_cast<int>(__nodes_.size() - 1);
  }
  int __leaf(__re_kind k) {
    node n;
    n.kind = k;
    return add(static_cast<node&&>(n));
  }
  int literal(__charT c) {
    node n;
    n.kind = __re_kind::__chr;
    n.__ch = c;
    return add(static_cast<node&&>(n));
  }
  int list(__re_kind k, std::vector<int>& __kids) {
    if (__kids.empty())
      return __leaf(__re_kind::empty);
    if (__kids.size() == 1)
      return __kids[0];
    node n;
    n.kind = k;
    n.__kids = static_cast<std::vector<int>&&>(__kids);
    return add(static_cast<node&&>(n));
  }
  int repeat(int __atom, int __mn, int __mx, bool __greedy, int __g_lo) {
    node n;
    n.kind = __re_kind::repeat;
    n.min = __mn;
    n.max = __mx;
    n.__greedy = __greedy;
    n.__group_lo = __g_lo;
    n.__group_hi = _P_.__groups + 1;
    n.__kids.push_back(__atom);
    return add(static_cast<node&&>(n));
  }
  int __open_group() {
    ++_P_.__groups;
    __closed_.push_back(0);
    return _P_.__groups;
  }
  int __group(int num, int __inner) {
    node n;
    n.kind = __re_kind::__group;
    n.__val = num;
    n.__group_lo = num;
    n.__group_hi = _P_.__groups + 1;
    n.__kids.push_back(__inner);
    __closed_[static_cast<std::size_t>(num)] = 1;
    return add(static_cast<node&&>(n));
  }
  int __backref(int num) {
    node n;
    n.kind = __re_kind::__backref;
    n.__val = num;
    _P_.__has_backref = true;
    if (num > __max_backref_)
      __max_backref_ = num;
    return add(static_cast<node&&>(n));
  }
  int __new_set(__set_type&& s) {
    _P_.__sets.push_back(static_cast<__set_type&&>(s));
    node n;
    n.kind = __re_kind::set;
    n.__val = static_cast<int>(_P_.__sets.size() - 1);
    return add(static_cast<node&&>(n));
  }
  int __class_escape(__charT e) {
    // \d \D \s \S \w \W: [re.grammar]/7.
    const char name = static_cast<char>(__re_ord(e) | 0x20); // the lower-case letter
    const __charT __nm[1] = {static_cast<__charT>(name)};
    __set_type s;
    s.__classes = __tr_.lookup_classname(__nm, __nm + 1, _P_.icase);
    s.negate = e != static_cast<__charT>(name);
    return __new_set(static_cast<__set_type&&>(s));
  }
  __charT translate(__charT c) const {
    return _P_.icase ? __tr_.translate_nocase(c) : _P_.collate ? __tr_.translate(c) : c;
  }

  // A decimal number of a {} interval or a back-reference; error `e` on overflow.
  int __number(__rc_err e) {
    long long __v = 0;
    while (__p_ != __end_ && digit(*__p_, 10) >= 0) {
      __v = __v * 10 + digit(*__p_, 10);
      if (__v > __max_count)
        fail(e);
      ++__p_;
    }
    return static_cast<int>(__v);
  }
  // The contents of an interval after its opening brace: m, m, or m,n, then the closing brace
  // ("}" or, in a BRE, "\}").
  void __interval(int& __mn, int& __mx, bool __bre) {
    if (__p_ == __end_)
      fail(std::regex_constants::error_brace);
    if (digit(*__p_, 10) < 0)
      fail(std::regex_constants::error_badbrace);
    __mn = __number(std::regex_constants::error_badbrace);
    __mx = __mn;
    if (at(',')) {
      ++__p_;
      __mx = (__p_ != __end_ && digit(*__p_, 10) >= 0) ? __number(std::regex_constants::error_badbrace) : -1;
    }
    // Anything but the closing brace after the numbers (end of pattern, "{1,2,3}") leaves the
    // brace unmatched.
    if (!(__bre ? __at2('\\', '}') : at('}')))
      fail(std::regex_constants::error_brace);
    __p_ += __bre ? 2 : 1;
    if (__mx >= 0 && __mx < __mn)
      fail(std::regex_constants::error_badbrace);
  }

  // ---- bracket expressions --------------------------------------------------------------------
  struct __class_atom {
    enum { __chr, __cls, __neg_cls, __equiv, __multi } kind = __chr;
    __charT c{};
    __class_type m{};
    string_type key;
    string_type str; // __multi, and __equiv of a multi-character element: the element
  };
  // [:name:], [.name.] or [=name=], at "[" followed by one of ":.=".
  __class_atom __bracket_special() {
    const __charT __delim = __p_[1];
    __p_ += 2;
    const __charT* __q = __p_;
    while (__q != __end_ && !(*__q == __delim && __q + 1 != __end_ && __q[1] == static_cast<__charT>(']')))
      ++__q;
    if (__q == __end_)
      fail(std::regex_constants::error_brack);
    const __charT* __nb = __p_;
    __p_ = __q + 2;
    __class_atom a;
    if (__delim == static_cast<__charT>(':')) {
      a.kind = __class_atom::__cls;
      a.m = __tr_.lookup_classname(__nb, __q, _P_.icase);
      if (a.m == __class_type{})
        fail(std::regex_constants::error_ctype);
      return a;
    }
    string_type name = __tr_.lookup_collatename(__nb, __q);
    if (name.empty())
      fail(std::regex_constants::error_collate);
    if (__delim == static_cast<__charT>('.')) {
      if (name.size() == 1) {
        a.c = name[0];
      } else { // a multi-character collating element ([re.traits]/8, XBD 9.3.5)
        a.kind = __class_atom::__multi;
        a.str = static_cast<string_type&&>(name);
      }
      return a;
    }
    // [re.grammar]/10: invalid if the primary key is empty.
    a.kind = __class_atom::__equiv;
    a.key = __tr_.transform_primary(name.begin(), name.end());
    if (a.key.empty())
      fail(std::regex_constants::error_collate);
    if (name.size() > 1)
      a.str = static_cast<string_type&&>(name);
    return a;
  }
  void __add_atom(__set_type& s, __class_atom& a) {
    switch (a.kind) {
    case __class_atom::__chr:
      s.__chars.push_back(translate(a.c));
      break;
    case __class_atom::__cls:
      s.__classes |= a.m;
      break;
    case __class_atom::__neg_cls:
      s.__neg_classes.push_back(a.m);
      break;
    case __class_atom::__equiv:
      s.__equivs.push_back(static_cast<string_type&&>(a.key));
      if (!a.str.empty())
        s.__multi.push_back(static_cast<string_type&&>(a.str));
      break;
    case __class_atom::__multi:
      s.__multi.push_back(static_cast<string_type&&>(a.str));
      break;
    }
  }
  // The sort key of a range end ([re.grammar]/14.2): its translated character, or the whole
  // multi-character element.
  string_type __range_key(const __class_atom& a) const {
    if (a.kind == __class_atom::__multi)
      return __tr_.transform(a.str.begin(), a.str.end());
    const __charT k[1] = {__tr_.translate(a.c)};
    return __tr_.transform(k, k + 1);
  }
  void __add_range(__set_type& s, const __class_atom& a, const __class_atom& b) {
    const bool __multi_end = a.kind == __class_atom::__multi || b.kind == __class_atom::__multi;
    if ((a.kind != __class_atom::__chr && a.kind != __class_atom::__multi) ||
        (b.kind != __class_atom::__chr && b.kind != __class_atom::__multi) || (__multi_end && !_P_.collate))
      fail(std::regex_constants::error_range); // a multi-character end has a sort key, no code
    string_type __lo, __hi;
    if (_P_.collate) {
      __lo = __range_key(a);
      __hi = __range_key(b);
      if (__hi < __lo)
        fail(std::regex_constants::error_range);
    } else {
      if (__re_ord(b.c) < __re_ord(a.c))
        fail(std::regex_constants::error_range);
      s.ranges.push_back({a.c, b.c});
    }
    // icase: the folded values of the range's characters below 256.
    if constexpr (__re_cacheable<__charT>) {
      if (_P_.icase)
        for (unsigned __u = 0; __u < 256u; ++__u) {
          const __charT c = static_cast<__charT>(__u);
          bool in;
          if (_P_.collate) {
            const __charT k[1] = {__tr_.translate(c)};
            const string_type key = __tr_.transform(k, k + 1);
            in = !(key < __lo) && !(__hi < key);
          } else {
            in = __re_ord(a.c) <= __u && __u <= __re_ord(b.c);
          }
          const auto __f = __re_ord(__tr_.translate_nocase(c));
          if (in && __f < 256u)
            s.__range_fold[__f >> 3] |= static_cast<unsigned char>(1u << (__f & 7));
        }
    }
    if (_P_.collate) {
      s.__coll_lo.push_back(static_cast<string_type&&>(__lo));
      s.__coll_hi.push_back(static_cast<string_type&&>(__hi));
    }
  }
  // ECMAScript ClassAtom (with the [re.grammar]/3 additions).
  __class_atom __ecma_class_atom() {
    __class_atom a;
    if (at('\\')) {
      ++__p_;
      if (__p_ == __end_)
        fail(std::regex_constants::error_escape);
      const __charT e = *__p_;
      switch (static_cast<char>(__re_ord(e) < 128u ? __re_ord(e) : 0)) {
      case 'b':
        ++__p_;
        a.c = __charT(8);
        return a;
      case 'd': case 's': case 'w': case 'D': case 'S': case 'W': {
        ++__p_;
        const char name = static_cast<char>(__re_ord(e) | 0x20);
        const __charT __nm[1] = {static_cast<__charT>(name)};
        a.m = __tr_.lookup_classname(__nm, __nm + 1, _P_.icase);
        a.kind = e == static_cast<__charT>(name) ? __class_atom::__cls : __class_atom::__neg_cls;
        return a;
      }
      default:
        if (digit(e, 10) > 0) // a back-reference is not a character ([re.grammar], ES5 15.10.2.19)
          fail(std::regex_constants::error_escape);
        a.c = __ecma_char_escape();
        return a;
      }
    }
    if (at('[') && __p_ + 1 != __end_ &&
        (__p_[1] == static_cast<__charT>(':') || __p_[1] == static_cast<__charT>('.') || __p_[1] == static_cast<__charT>('=')))
      return __bracket_special();
    a.c = *__p_++;
    return a;
  }
  // A POSIX bracket expression element ([[:class:]], [.coll.], [=equiv=], an awk escape).
  __class_atom __posix_class_atom() {
    if (at('[') && __p_ + 1 != __end_ &&
        (__p_[1] == static_cast<__charT>(':') || __p_[1] == static_cast<__charT>('.') || __p_[1] == static_cast<__charT>('=')))
      return __bracket_special();
    __class_atom a;
    if (__g_ == __awk_g && at('\\')) {
      ++__p_;
      a.c = __awk_escape();
      return a;
    }
    a.c = *__p_++;
    return a;
  }
  int __bracket() { // after '['
    __set_type s;
    if (at('^')) {
      ++__p_;
      s.negate = true;
    }
    for (bool first = true;; first = false) {
      if (__p_ == __end_)
        fail(std::regex_constants::error_brack);
      if (at(']') && (__g_ == __ecma || !first)) {
        ++__p_;
        break;
      }
      __class_atom a = __g_ == __ecma ? __ecma_class_atom() : __posix_class_atom();
      if (at('-') && __p_ + 1 != __end_ && __p_[1] != static_cast<__charT>(']')) {
        ++__p_;
        __class_atom b = __g_ == __ecma ? __ecma_class_atom() : __posix_class_atom();
        __add_range(s, a, b);
      } else {
        __add_atom(s, a);
      }
    }
    if (s.__multi.empty())
      return __new_set(static_cast<__set_type&&>(s));
    // Multi-character collating elements: each is an alternative of its own, longest first.
    std::vector<string_type> __elems = static_cast<std::vector<string_type>&&>(s.__multi);
    s.__multi.clear();
    for (std::size_t i = 1; i < __elems.size(); ++i) // insertion sort: longest first, stable
      for (std::size_t __j = i; __j > 0 && __elems[__j - 1].size() < __elems[__j].size(); --__j) {
        string_type __tmp = static_cast<string_type&&>(__elems[__j]);
        __elems[__j] = static_cast<string_type&&>(__elems[__j - 1]);
        __elems[__j - 1] = static_cast<string_type&&>(__tmp);
      }
    const bool __neg = s.negate;
    std::vector<int> __alts;
    for (const string_type& __e : __elems) {
      std::vector<int> __lits;
      for (__charT c : __e)
        __lits.push_back(literal(c));
      __alts.push_back(list(__re_kind::concat, __lits));
    }
    const int __single = __new_set(static_cast<__set_type&&>(s));
    if (!__neg) { // a matching list: one of the elements, or one character of the set
      __alts.push_back(__single);
      return list(__re_kind::__alt, __alts);
    }
    // A non-matching list: one character of the set, where none of the elements begins.
    node __x;
    __x.kind = __re_kind::__nlook;
    __x.__kids.push_back(list(__re_kind::__alt, __alts));
    std::vector<int> __seq{add(static_cast<node&&>(__x)), __single};
    _P_.__needs_bt = true;
    return list(__re_kind::concat, __seq);
  }

  // ---- ECMAScript ---------------------------------------------------------------------------
  // CharacterEscape after the backslash (at *p_): ControlEscape, \cX, \xHH, \uHHHH, \0 and
  // IdentityEscape ("SourceCharacter but not c", [re.grammar]/3).
  __charT __ecma_char_escape() {
    const __charT e = *__p_++;
    const auto __u = __re_ord(e);
    if (__u < 128u) {
      switch (static_cast<char>(__u)) {
      case 'f': return __charT(0x0C);
      case 'n': return __charT(0x0A);
      case 'r': return __charT(0x0D);
      case 't': return __charT(0x09);
      case 'v': return __charT(0x0B);
      case '0':
        if (__p_ != __end_ && digit(*__p_, 10) >= 0)
          fail(std::regex_constants::error_escape);
        return __charT(0);
      case 'c': {
        if (__p_ == __end_)
          fail(std::regex_constants::error_escape);
        const auto __l = __re_ord(*__p_);
        if (!((__l >= 'a' && __l <= 'z') || (__l >= 'A' && __l <= 'Z')))
          fail(std::regex_constants::error_escape);
        ++__p_;
        return __charT(__l % 32);
      }
      case 'x':
      case 'u': {
        const int n = __u == 'x' ? 2 : 4;
        unsigned long __v = 0;
        for (int i = 0; i < n; ++i) {
          const int d = __p_ != __end_ ? digit(*__p_, 16) : -1;
          if (d < 0)
            fail(std::regex_constants::error_escape);
          __v = __v * 16 + static_cast<unsigned long>(d);
          ++__p_;
        }
        // [re.grammar]/12: a value that does not fit in charT is an error.
        if constexpr (std::is_integral_v<__charT>) {
          if (__v > static_cast<unsigned long>(static_cast<std::make_unsigned_t<__charT>>(-1)))
            fail(std::regex_constants::error_escape);
        }
        return static_cast<__charT>(__v);
      }
      default:
        break;
      }
    }
    return e;
  }
  void __no_quantifier() {
    if (at('*') || at('+') || at('?') || at('{'))
      fail(std::regex_constants::error_badrepeat);
  }
  int __ecma_disjunction() {
    __enter();
    std::vector<int> __alts;
    __alts.push_back(__ecma_alternative());
    while (at('|')) {
      ++__p_;
      __alts.push_back(__ecma_alternative());
    }
    --__depth_;
    return list(__re_kind::__alt, __alts);
  }
  int __ecma_alternative() {
    std::vector<int> __terms;
    while (__p_ != __end_ && !at('|') && !at(')'))
      __terms.push_back(__ecma_term());
    return list(__re_kind::concat, __terms);
  }
  int __ecma_term() {
    int n;
    if (at('^') || at('$')) {
      n = __leaf(at('^') ? __re_kind::__bol : __re_kind::__eol);
      ++__p_;
      __no_quantifier();
      return n;
    }
    if (__at2('\\', 'b') || __at2('\\', 'B')) {
      n = __leaf(__p_[1] == static_cast<__charT>('b') ? __re_kind::__wordb : __re_kind::__nwordb);
      __p_ += 2;
      __no_quantifier();
      return n;
    }
    if (__at2('(', '?') && __p_ + 2 != __end_ && (__p_[2] == static_cast<__charT>('=') || __p_[2] == static_cast<__charT>('!'))) {
      const bool __neg = __p_[2] == static_cast<__charT>('!');
      __p_ += 3;
      const int __inner = __ecma_disjunction();
      if (!at(')'))
        fail(std::regex_constants::error_paren);
      ++__p_;
      node __x;
      __x.kind = __neg ? __re_kind::__nlook : __re_kind::__look;
      __x.__kids.push_back(__inner);
      n = add(static_cast<node&&>(__x));
      __no_quantifier();
      return n;
    }
    const int __g0 = _P_.__groups + 1;
    return __quantifier(__ecma_atom(), __g0);
  }
  int __ecma_atom() {
    const __charT c = *__p_;
    const auto __u = __re_ord(c);
    switch (static_cast<char>(__u < 128u ? __u : 0)) {
    case '.':
      ++__p_;
      return __leaf(__re_kind::any);
    case '(': {
      ++__p_;
      if (__at2('?', ':')) {
        __p_ += 2;
        const int __inner = __ecma_disjunction();
        if (!at(')'))
          fail(std::regex_constants::error_paren);
        ++__p_;
        return __inner;
      }
      if (at('?'))
        fail(std::regex_constants::error_badrepeat);
      const int num = __open_group();
      const int __inner = __ecma_disjunction();
      if (!at(')'))
        fail(std::regex_constants::error_paren);
      ++__p_;
      return __group(num, __inner);
    }
    case '[':
      ++__p_;
      return __bracket();
    case '\\': {
      ++__p_;
      if (__p_ == __end_)
        fail(std::regex_constants::error_escape);
      const __charT e = *__p_;
      const auto __eu = __re_ord(e);
      if (__eu < 128u) {
        switch (static_cast<char>(__eu)) {
        case 'd': case 'D': case 's': case 'S': case 'w': case 'W':
          ++__p_;
          return __class_escape(e);
        default:
          break;
        }
      }
      if (digit(e, 10) > 0)
        return __backref(__number(std::regex_constants::error_backref));
      return literal(__ecma_char_escape());
    }
    case '*': case '+': case '?': case '{':
      fail(std::regex_constants::error_badrepeat);
    default:
      ++__p_;
      return literal(c);
    }
  }
  int __quantifier(int __atom, int __g0) {
    if (__p_ == __end_)
      return __atom;
    int __mn, __mx;
    if (at('*')) {
      __mn = 0, __mx = -1;
      ++__p_;
    } else if (at('+')) {
      __mn = 1, __mx = -1;
      ++__p_;
    } else if (at('?')) {
      __mn = 0, __mx = 1;
      ++__p_;
    } else if (at('{')) {
      ++__p_;
      __interval(__mn, __mx, false);
    } else {
      return __atom;
    }
    bool __greedy = true;
    if (at('?')) {
      ++__p_;
      __greedy = false;
    }
    __no_quantifier();
    return repeat(__atom, __mn, __mx, __greedy, __g0);
  }

  // ---- POSIX ----------------------------------------------------------------------------------
  // An awk escape after the backslash ([re.synopt] awk: the escapes of POSIX awk).
  __charT __awk_escape() {
    if (__p_ == __end_)
      fail(std::regex_constants::error_escape);
    const __charT e = *__p_++;
    const auto __u = __re_ord(e);
    if (__u < 128u) {
      switch (static_cast<char>(__u)) {
      case 'a': return __charT(7);
      case 'b': return __charT(8);
      case 'f': return __charT(12);
      case 'n': return __charT(10);
      case 'r': return __charT(13);
      case 't': return __charT(9);
      case 'v': return __charT(11);
      case '"': case '/': case '\\': case '^': case '.': case '[': case ']': case '$': case '(': case ')':
      case '|': case '*': case '+': case '?': case '{': case '}':
        return e;
      default:
        break;
      }
    }
    if (digit(e, 8) >= 0) {
      unsigned __v = static_cast<unsigned>(digit(e, 8));
      for (int i = 0; i < 2 && __p_ != __end_ && digit(*__p_, 8) >= 0; ++i)
        __v = __v * 8 + static_cast<unsigned>(digit(*__p_++, 8));
      return static_cast<__charT>(__v);
    }
    fail(std::regex_constants::error_escape);
  }
  int __posix_backref(__charT e) {
    const int num = digit(e, 10);
    if (num > _P_.__groups || !__closed_[static_cast<std::size_t>(num)])
      fail(std::regex_constants::error_backref);
    return __backref(num);
  }
  static bool __posix_special(__charT e) {
    const auto __u = __re_ord(e);
    if (__u >= 128u)
      return false;
    switch (static_cast<char>(__u)) {
    case '^': case '.': case '[': case ']': case '$': case '(': case ')': case '|': case '*': case '+':
    case '?': case '{': case '}': case '\\':
      return true;
    default:
      return false;
    }
  }
  // A basic regular expression (basic, grep), up to the end or "\)".
  int __bre_expr() {
    __enter();
    std::vector<int> __terms;
    if (at('^')) {
      ++__p_;
      __terms.push_back(__leaf(__re_kind::__bol));
    }
    bool start = true;
    while (__p_ != __end_ && !__at2('\\', ')')) {
      if (at('$') && (__p_ + 1 == __end_ || (__p_[1] == static_cast<__charT>('\\') && __p_ + 2 != __end_ && __p_[2] == static_cast<__charT>(')')))) {
        ++__p_;
        __terms.push_back(__leaf(__re_kind::__eol));
        continue;
      }
      const int __g0 = _P_.__groups + 1;
      int __atom;
      if (start && at('*')) { // a leading '*' is an ordinary character
        ++__p_;
        __atom = literal(__charT('*'));
      } else {
        __atom = __bre_atom();
      }
      start = false;
      for (;;) {
        int __mn, __mx;
        if (at('*')) {
          ++__p_;
          __mn = 0, __mx = -1;
        } else if (__at2('\\', '{')) {
          __p_ += 2;
          __interval(__mn, __mx, true);
        } else {
          break;
        }
        __atom = repeat(__atom, __mn, __mx, true, __g0);
      }
      __terms.push_back(__atom);
    }
    --__depth_;
    return list(__re_kind::concat, __terms);
  }
  int __bre_atom() {
    const __charT c = *__p_;
    if (c == static_cast<__charT>('.')) {
      ++__p_;
      return __leaf(__re_kind::any);
    }
    if (c == static_cast<__charT>('[')) {
      ++__p_;
      return __bracket();
    }
    if (c != static_cast<__charT>('\\')) {
      ++__p_;
      return literal(c);
    }
    if (__p_ + 1 == __end_)
      fail(std::regex_constants::error_escape);
    const __charT e = __p_[1];
    __p_ += 2;
    if (e == static_cast<__charT>('(')) {
      const int num = __open_group();
      const int __inner = __bre_expr();
      if (!__at2('\\', ')'))
        fail(std::regex_constants::error_paren);
      __p_ += 2;
      return __group(num, __inner);
    }
    if (e == static_cast<__charT>('{'))
      fail(std::regex_constants::error_badrepeat);
    if (e == static_cast<__charT>('}'))
      fail(std::regex_constants::error_brace);
    if (digit(e, 10) > 0)
      return __posix_backref(e);
    const auto __u = __re_ord(e);
    if (__u < 128u && (static_cast<char>(__u) == '.' || static_cast<char>(__u) == '[' || static_cast<char>(__u) == ']' ||
                     static_cast<char>(__u) == '\\' || static_cast<char>(__u) == '*' || static_cast<char>(__u) == '^' ||
                     static_cast<char>(__u) == '$'))
      return literal(e);
    fail(std::regex_constants::error_escape);
  }
  // An extended regular expression (extended, egrep, awk).
  int __ere_alt() {
    __enter();
    std::vector<int> __alts;
    __alts.push_back(__ere_branch());
    while (at('|')) {
      ++__p_;
      __alts.push_back(__ere_branch());
    }
    --__depth_;
    return list(__re_kind::__alt, __alts);
  }
  int __ere_branch() {
    std::vector<int> __terms;
    while (__p_ != __end_ && !at('|') && !at(')'))
      __terms.push_back(__ere_expr());
    return list(__re_kind::concat, __terms);
  }
  int __ere_expr() {
    const __charT c = *__p_;
    const auto __u = __re_ord(c);
    const int __g0 = _P_.__groups + 1;
    int __atom;
    switch (static_cast<char>(__u < 128u ? __u : 0)) {
    case '^':
    case '$':
      ++__p_;
      __atom = __leaf(c == static_cast<__charT>('^') ? __re_kind::__bol : __re_kind::__eol);
      __no_quantifier();
      return __atom;
    case '*': case '+': case '?': case '{':
      fail(std::regex_constants::error_badrepeat);
    case '(': {
      ++__p_;
      const int num = __open_group();
      const int __inner = __ere_alt();
      if (!at(')'))
        fail(std::regex_constants::error_paren);
      ++__p_;
      __atom = __group(num, __inner);
      break;
    }
    case '.':
      ++__p_;
      __atom = __leaf(__re_kind::any);
      break;
    case '[':
      ++__p_;
      __atom = __bracket();
      break;
    case '\\': {
      ++__p_;
      if (__g_ == __awk_g) {
        __atom = literal(__awk_escape());
        break;
      }
      if (__p_ == __end_)
        fail(std::regex_constants::error_escape);
      const __charT e = *__p_++;
      if (digit(e, 10) > 0)
        __atom = __posix_backref(e);
      else if (__posix_special(e))
        __atom = literal(e);
      else
        fail(std::regex_constants::error_escape);
      break;
    }
    default:
      ++__p_;
      __atom = literal(c);
      break;
    }
    for (;;) {
      int __mn, __mx;
      if (at('*')) {
        __mn = 0, __mx = -1;
      } else if (at('+')) {
        __mn = 1, __mx = -1;
      } else if (at('?')) {
        __mn = 0, __mx = 1;
      } else if (at('{')) {
        ++__p_;
        __interval(__mn, __mx, false);
        __atom = repeat(__atom, __mn, __mx, true, __g0);
        continue;
      } else {
        break;
      }
      ++__p_;
      __atom = repeat(__atom, __mn, __mx, true, __g0);
    }
    return __atom;
  }
  // grep and egrep: the lines of the pattern are alternatives.
  int __lines(bool __bre) {
    std::vector<int> __alts;
    const __charT* __all_end = __end_;
    for (;;) {
      const __charT* __nl = __p_;
      while (__nl != __all_end && *__nl != static_cast<__charT>('\n'))
        ++__nl;
      __end_ = __nl;
      __alts.push_back(__bre ? __bre_expr() : __ere_alt());
      if (__p_ != __end_)
        fail(std::regex_constants::error_paren);
      __end_ = __all_end;
      if (__nl == __all_end)
        break;
      __p_ = __nl + 1;
    }
    return list(__re_kind::__alt, __alts);
  }

  // ---- analysis and code generation -------------------------------------------------------------
  bool __y_nullable(int n) const {
    const node& __x = __nodes_[static_cast<std::size_t>(n)];
    switch (__x.kind) {
    case __re_kind::__chr:
    case __re_kind::any:
    case __re_kind::set:
      return false;
    case __re_kind::__group:
      return __y_nullable(__x.__kids[0]);
    case __re_kind::concat:
      for (int k : __x.__kids)
        if (!__y_nullable(k))
          return false;
      return true;
    case __re_kind::__alt:
      for (int k : __x.__kids)
        if (__y_nullable(k))
          return true;
      return false;
    case __re_kind::repeat:
      return __x.min == 0 || __y_nullable(__x.__kids[0]);
    default: // empty, assertions, lookahead, back-reference
      return true;
    }
  }
  bool __single_char(int n) const {
    const __re_kind k = __nodes_[static_cast<std::size_t>(n)].kind;
    return k == __re_kind::__chr || k == __re_kind::any || k == __re_kind::set;
  }
  // The number of nodes expand() makes of n, saturated at max_expanded + 1; also too large when
  // one atom would be copied more than max_copies times.
  std::size_t __expanded_size(int n) const {
    const node& __x = __nodes_[static_cast<std::size_t>(n)];
    std::size_t s = 1;
    for (int k : __x.__kids)
      s += __expanded_size(k);
    if (__x.kind == __re_kind::repeat) {
      const int __copies = __x.max < 0 ? __x.min + 1 : __x.max;
      if (__copies > __max_copies)
        return __max_expanded + 1;
      s *= static_cast<std::size_t>(__copies < 1 ? 1 : __copies) * 2;
    }
    return s > __max_expanded ? __max_expanded + 1 : s;
  }
  // Copies the subtree n (POSIX expansion of bounded repetitions).
  int __clone(int n) {
    node __x = __nodes_[static_cast<std::size_t>(n)];
    for (int& k : __x.__kids)
      k = __clone(k);
    return add(static_cast<node&&>(__x));
  }
  // A copy of the body of repetition r that is not its first iteration.
  int __copy(int __body, const node& r) {
    const int c = __clone(__body);
    node& __y = __nodes_[static_cast<std::size_t>(c)];
    if (r.__group_hi > r.__group_lo) {
      __y.__reset = true;
      __y.__group_lo = r.__group_lo;
      __y.__group_hi = r.__group_hi;
    }
    return c;
  }
  // Rewrites every repetition into x?, x* and concatenations (nfa programs).
  int expand(int n) {
    const std::size_t __nk = __nodes_[static_cast<std::size_t>(n)].__kids.size();
    for (std::size_t i = 0; i < __nk; ++i) {
      const int k = expand(__nodes_[static_cast<std::size_t>(n)].__kids[i]);
      __nodes_[static_cast<std::size_t>(n)].__kids[i] = k;
    }
    const node __x = __nodes_[static_cast<std::size_t>(n)];
    if (__x.kind != __re_kind::repeat || (__x.min == 0 && (__x.max == 1 || __x.max < 0)))
      return n;
    if (__x.max == 0)
      return __leaf(__re_kind::empty);
    const int __body = __x.__kids[0];
    if (__x.min == 1 && __x.max == 1)
      return __body;
    // A tail follows another iteration of the same repetition: it makes no empty iteration.
    auto __opt = [&](int __kid, int __mx, bool __tail) {
      node r;
      r.kind = __re_kind::repeat;
      r.min = 0;
      r.max = __mx;
      r.__tail = __tail;
      r.__group_lo = __x.__group_lo;
      r.__group_hi = __x.__group_hi;
      r.__kids.push_back(__kid);
      return add(static_cast<node&&>(r));
    };
    std::vector<int> seq;
    for (int i = 0; i < __x.min; ++i)
      seq.push_back(i == 0 ? __body : __copy(__body, __x));
    if (__x.max < 0) {
      seq.push_back(__opt(__x.min == 0 ? __body : __clone(__body), -1, __x.min > 0));
    } else if (__x.max > __x.min) { // x{0,3} is (x(x(x)?)?)?
      int __tail = __opt(__x.min == 0 ? __body : __clone(__body), 1, __x.min > 0 || __x.max - __x.min > 1);
      for (int i = __x.min + 1; i < __x.max; ++i) {
        std::vector<int> pair{__clone(__body), __tail};
        __tail = __opt(list(__re_kind::concat, pair), 1, __x.min > 0 || i + 1 < __x.max);
      }
      seq.push_back(__tail);
    }
    return list(__re_kind::concat, seq);
  }

  // POSIX backtracking programs: the per-node facts the subexpression search uses (re_program).
  static std::ptrdiff_t __len_add(std::ptrdiff_t a, std::ptrdiff_t b) {
    constexpr std::ptrdiff_t __cap = std::ptrdiff_t(1) << 40;
    if (a < 0 || b < 0)
      return -1;
    return a + b > __cap ? __cap : a + b;
  }
  static std::ptrdiff_t __len_mul(std::ptrdiff_t a, std::ptrdiff_t __k) { // k < 0: unbounded
    constexpr std::ptrdiff_t __cap = std::ptrdiff_t(1) << 40;
    if (a == 0 || __k == 0)
      return 0;
    if (a < 0 || __k < 0)
      return -1;
    return a > __cap / __k ? __cap : a * __k;
  }
  struct __an_state {
    std::vector<int> __pre, __post, __glo, __ghi;
    std::vector<std::ptrdiff_t> __gmax; // per group: the longest string its node matches
    int __maxref[10];                  // per back-reference number: the last preorder index
    int __counter = 0;
  };
  void __an_visit(int n, __an_state& __st) {
    using __prog = __re_program<__charT, __traits>;
    auto& _Pp = _P_;
    const auto __un = static_cast<std::size_t>(n);
    __st.__pre[__un] = __st.__counter++;
    const node& __x = __nodes_[__un];
    std::ptrdiff_t __mn = 0, __mx = 0;
    bool __pure = true;
    int __glo = 1 << 30, __ghi = -1;
    for (int k : __x.__kids) {
      __an_visit(k, __st);
      const auto __uk = static_cast<std::size_t>(k);
      __pure = __pure && (_Pp.__nflags[__uk] & __prog::__nf_pure) != 0;
      if (__st.__glo[__uk] < __glo)
        __glo = __st.__glo[__uk];
      if (__st.__ghi[__uk] > __ghi)
        __ghi = __st.__ghi[__uk];
    }
    switch (__x.kind) {
    case __re_kind::__chr:
    case __re_kind::any:
    case __re_kind::set:
      __mn = __mx = 1;
      break;
    case __re_kind::__backref:
      __pure = false;
      __mx = __st.__gmax[static_cast<std::size_t>(__x.__val)];
      if (__x.__val < 10 && __st.__pre[__un] > __st.__maxref[__x.__val])
        __st.__maxref[__x.__val] = __st.__pre[__un];
      break;
    case __re_kind::__group: {
      const auto __uk = static_cast<std::size_t>(__x.__kids[0]);
      __mn = _Pp.__nmin[__uk];
      __mx = _Pp.__nmax[__uk];
      __st.__gmax[static_cast<std::size_t>(__x.__val)] = __mx;
      if (__x.__val < __glo)
        __glo = __x.__val;
      if (__x.__val + 1 > __ghi)
        __ghi = __x.__val + 1;
      break;
    }
    case __re_kind::concat: {
      const std::size_t m = __x.__kids.size();
      _Pp.__suf_off[__un] = static_cast<int>(_Pp.__suf_min.size());
      _Pp.__suf_min.resize(_Pp.__suf_min.size() + m + 1, 0);
      _Pp.__suf_max.resize(_Pp.__suf_max.size() + m + 1, 0);
      const auto __o = static_cast<std::size_t>(_Pp.__suf_off[__un]);
      for (std::size_t i = m; i-- > 0;) {
        const auto __uk = static_cast<std::size_t>(__x.__kids[i]);
        _Pp.__suf_min[__o + i] = __len_add(_Pp.__suf_min[__o + i + 1], _Pp.__nmin[__uk]);
        _Pp.__suf_max[__o + i] = __len_add(_Pp.__suf_max[__o + i + 1], _Pp.__nmax[__uk]);
      }
      __mn = _Pp.__suf_min[__o];
      __mx = _Pp.__suf_max[__o];
      break;
    }
    case __re_kind::__alt:
      __mn = -1;
      for (int k : __x.__kids) {
        const auto __uk = static_cast<std::size_t>(k);
        if (__mn < 0 || _Pp.__nmin[__uk] < __mn)
          __mn = _Pp.__nmin[__uk];
        if (__mx >= 0 && (_Pp.__nmax[__uk] < 0 || _Pp.__nmax[__uk] > __mx))
          __mx = _Pp.__nmax[__uk];
      }
      break;
    case __re_kind::repeat: {
      const auto __uk = static_cast<std::size_t>(__x.__kids[0]);
      __mn = __len_mul(_Pp.__nmin[__uk], __x.min);
      __mx = __x.max == 0 ? 0 : __len_mul(_Pp.__nmax[__uk], __x.max);
      break;
    }
    default: // empty, assertions, lookahead: zero-width
      break;
    }
    _Pp.__nmin[__un] = __mn;
    _Pp.__nmax[__un] = __mx;
    __st.__glo[__un] = __glo;
    __st.__ghi[__un] = __ghi;
    __st.__post[__un] = __st.__counter - 1;
    if (__pure)
      _Pp.__nflags[__un] |= __prog::__nf_pure;
  }
  void __analyze(int __root) {
    using __prog = __re_program<__charT, __traits>;
    auto& _Pp = _P_;
    const std::size_t __nn = __nodes_.size();
    _Pp.__nmin.assign(__nn, 0);
    _Pp.__nmax.assign(__nn, 0);
    _Pp.__nflags.assign(__nn, 0);
    _Pp.__suf_off.assign(__nn, -1);
    __an_state __st;
    __st.__pre.assign(__nn, 0);
    __st.__post.assign(__nn, 0);
    __st.__glo.assign(__nn, 1 << 30);
    __st.__ghi.assign(__nn, -1);
    __st.__gmax.assign(static_cast<std::size_t>(_Pp.__groups) + 1, -1);
    for (int& r : __st.__maxref)
      r = -1;
    __an_visit(__root, __st);
    // A node commits to its first way of matching a span unless a back-reference after it refers
    // to a group inside it (POSIX back-references are \1 to \9 and follow their group).
    for (std::size_t n = 0; n < __nn; ++n) {
      bool __ok = true;
      for (int __g = __st.__glo[n] < 1 ? 1 : __st.__glo[n]; __ok && __g < __st.__ghi[n] && __g < 10; ++__g)
        if (__st.__maxref[__g] > __st.__post[n])
          __ok = false;
      if (__ok)
        _Pp.__nflags[n] |= __prog::__nf_commit;
    }
  }

  int emit(__re_op op, int a = 0, bool __flag = false) {
    if (_P_.code.size() >= __max_nodes)
      fail(std::regex_constants::error_space);
    __re_inst<__charT> in;
    in.op = op;
    in.a = a;
    in.__flag = __flag;
    _P_.code.push_back(in);
    return static_cast<int>(_P_.code.size() - 1);
  }
  int __pc() const { return static_cast<int>(_P_.code.size()); }
  __re_inst<__charT>& __at_pc(int i) { return _P_.code[static_cast<std::size_t>(i)]; }

  void __gen(int n) {
    const node __x = __nodes_[static_cast<std::size_t>(n)]; // gen never adds nodes, but keep a copy
    if (_P_.posix)
      _P_.__node_begin[static_cast<std::size_t>(n)] = __pc();
    switch (__x.kind) {
    case __re_kind::empty:
      break;
    case __re_kind::__chr:
      __at_pc(emit(__re_op::__chr)).__ch = translate(__x.__ch);
      break;
    case __re_kind::any:
      emit(__re_op::any, 0, _P_.__ecma);
      break;
    case __re_kind::set:
      emit(__re_op::set, __x.__val);
      break;
    case __re_kind::__bol:
      emit(__re_op::__bol);
      break;
    case __re_kind::__eol:
      emit(__re_op::__eol);
      break;
    case __re_kind::__wordb:
      emit(__re_op::__wordb);
      break;
    case __re_kind::__nwordb:
      emit(__re_op::__nwordb);
      break;
    case __re_kind::__backref:
      emit(__re_op::__backref, __x.__val);
      break;
    case __re_kind::__group:
      emit(__re_op::open, __x.__val);
      __gen(__x.__kids[0]);
      emit(__re_op::close, __x.__val);
      break;
    case __re_kind::__look:
    case __re_kind::__nlook: {
      const int __l = emit(__re_op::__look, 0, __x.kind == __re_kind::__nlook);
      __gen(__x.__kids[0]);
      __at_pc(__l).a = emit(__re_op::__look_end);
      break;
    }
    case __re_kind::concat:
      for (int k : __x.__kids)
        __gen(k);
      break;
    case __re_kind::__alt: {
      std::vector<int> __jumps;
      for (std::size_t i = 0; i < __x.__kids.size(); ++i) {
        if (i + 1 < __x.__kids.size()) {
          const int s = emit(__re_op::split);
          __at_pc(s).a = s + 1;
          __gen(__x.__kids[i]);
          __jumps.push_back(emit(__re_op::__jmp));
          __at_pc(s).b = __pc();
        } else {
          __gen(__x.__kids[i]);
        }
      }
      for (int __j : __jumps)
        __at_pc(__j).a = __pc();
      break;
    }
    case __re_kind::repeat:
      __gen_repeat(__x);
      break;
    }
    if (_P_.posix)
      _P_.__node_end[static_cast<std::size_t>(n)] = __pc();
  }
  void __gen_repeat(const node& __x) {
    if (_P_.__nfa) { // only x? and x* remain after expand()
      const int s = emit(__re_op::split);
      __at_pc(s).a = s + 1;
      __gen(__x.__kids[0]);
      if (__x.max < 0)
        emit(__re_op::__jmp, s);
      __at_pc(s).b = __pc();
      return;
    }
    if (__x.max == 0)
      return;
    if (__x.min == 1 && __x.max == 1) {
      __gen(__x.__kids[0]);
      return;
    }
    __re_loop __lp;
    __lp.min = __x.min;
    __lp.max = __x.max;
    __lp.__greedy = __x.__greedy;
    __lp.__group_lo = __x.__group_lo;
    __lp.__group_hi = __x.__group_hi;
    const int __li = static_cast<int>(_P_.__loops.size());
    _P_.__loops.push_back(__lp);
    if (__single_char(__x.__kids[0])) {
      emit(__re_op::rep, __li);
      __gen(__x.__kids[0]);
      _P_.__loops[static_cast<std::size_t>(__li)].__exit_pc = __pc();
      return;
    }
    emit(__re_op::__loop_enter, __li);
    const int __it = emit(__re_op::__loop_iter, __li);
    __gen(__x.__kids[0]);
    emit(__re_op::__loop_tail, __li);
    _P_.__loops[static_cast<std::size_t>(__li)].__iter_pc = __it;
    _P_.__loops[static_cast<std::size_t>(__li)].__exit_pc = __pc();
    // Whether the backtracker may remember failures: what follows a pc must not depend on the
    // loop's counter, so max is 1 or unbounded, and min at most 1. An iteration that can match
    // the empty string adds one bit of state, whether it began at the current position (the
    // empty check), which joins the memo key; only for min == 0, and for at most 4 such loops.
    const bool __counts_ok = __x.max == 1 || (__x.max < 0 && __x.min <= 1);
    if (!__counts_ok)
      _P_.__memo = false;
    else if (__y_nullable(__x.__kids[0])) {
      if (__x.min != 0 || _P_.__memo_loops.size() == 4)
        _P_.__memo = false;
      else
        _P_.__memo_loops.push_back(__li);
    }
  }

  void finish() {
    auto& _Pp = _P_;
    // The memo points: where the backtracker resumes or branches.
    if (_Pp.__memo) {
      _Pp.__memo_index.assign(_Pp.code.size(), -1);
      auto __mark = [&](int i) {
        if (_Pp.__memo_index[static_cast<std::size_t>(i)] < 0)
          _Pp.__memo_index[static_cast<std::size_t>(i)] = _Pp.__memo_points++;
      };
      for (std::size_t i = 0; i < _Pp.code.size(); ++i) {
        const auto& in = _Pp.code[i];
        if (in.op == __re_op::split) {
          __mark(in.a);
          __mark(in.b);
        } else if (in.op == __re_op::__look) {
          __mark(static_cast<int>(i));
        }
      }
      for (const __re_loop& __lp : _Pp.__loops) {
        if (__lp.__iter_pc != 0)
          __mark(__lp.__iter_pc);
        __mark(__lp.__exit_pc);
      }
      // A lookahead's body succeeds by reaching its look_end without ending the search, so a
      // (pc, position) pair visited there may have succeeded: no memo inside lookaheads (the
      // look instruction itself is a memo point: what follows it depends only on the position).
      for (std::size_t i = 0; i < _Pp.code.size(); ++i)
        if (_Pp.code[i].op == __re_op::__look)
          for (int __j = static_cast<int>(i) + 1; __j <= _Pp.code[i].a; ++__j)
            _Pp.__memo_index[static_cast<std::size_t>(__j)] = -1;
      // For each memo point, the nullable-body loops whose iteration it lies in.
      _Pp.__memo_mask.assign(static_cast<std::size_t>(_Pp.__memo_points), 0);
      for (std::size_t i = 0; i < _Pp.code.size(); ++i) {
        const int __mp = _Pp.__memo_index[i];
        if (__mp < 0)
          continue;
        for (std::size_t k = 0; k < _Pp.__memo_loops.size(); ++k) {
          const __re_loop& __lp = _Pp.__loops[static_cast<std::size_t>(_Pp.__memo_loops[k])];
          if (static_cast<int>(i) > __lp.__iter_pc && static_cast<int>(i) < __lp.__exit_pc)
            _Pp.__memo_mask[static_cast<std::size_t>(__mp)] |= 1u << k;
        }
      }
    }
    if (_Pp.__nfa) {
      _Pp.__eps_pred.assign(_Pp.code.size() + 1, {});
      for (std::size_t i = 0; i < _Pp.code.size(); ++i) {
        const auto& in = _Pp.code[i];
        const int __ii = static_cast<int>(i);
        switch (in.op) {
        case __re_op::split:
          _Pp.__eps_pred[static_cast<std::size_t>(in.a)].push_back(__ii);
          _Pp.__eps_pred[static_cast<std::size_t>(in.b)].push_back(__ii);
          break;
        case __re_op::__jmp:
          _Pp.__eps_pred[static_cast<std::size_t>(in.a)].push_back(__ii);
          break;
        case __re_op::open: case __re_op::close: case __re_op::__bol: case __re_op::__eol: case __re_op::__wordb: case __re_op::__nwordb:
          _Pp.__eps_pred[i + 1].push_back(__ii);
          break;
        default:
          break;
        }
      }
    }
    // Caches for the code units below 256.
    {
      const char __w[1] = {'w'};
      __charT __wn[1] = {static_cast<__charT>(__w[0])};
      _Pp.__word_class = __tr_.lookup_classname(__wn, __wn + 1, false);
    }
    if constexpr (__re_cacheable<__charT>) {
      auto __set_bit = [](unsigned char* bits, unsigned __u) { bits[__u >> 3] |= static_cast<unsigned char>(1u << (__u & 7)); };
      // The code units below 256 a class matches, computed once per distinct class.
      struct __class_entry {
        __class_type __f;
        unsigned char b[32];
      };
      std::vector<__class_entry> __class_bits;
      auto __bits_of = [&](__class_type __f) -> const unsigned char* {
        for (auto& e : __class_bits)
          if (e.__f == __f)
            return e.b;
        __class_entry& e = __class_bits.emplace_back(__class_entry{__f, {}});
        for (unsigned __u = 0; __u < 256; ++__u)
          if (__tr_.isctype(static_cast<__charT>(__u), __f))
            __set_bit(e.b, __u);
        return e.b;
      };
      for (unsigned __u = 0; __u < 256; ++__u) {
        const __charT c = static_cast<__charT>(__u);
        _Pp.__fold[__u] = _Pp.icase ? __tr_.translate_nocase(c) : _Pp.collate ? __tr_.translate(c) : c;
      }
      const unsigned char* __word = __bits_of(_Pp.__word_class);
      for (unsigned i = 0; i < 32; ++i)
        _Pp.__word[i] = __word[i];
      for (auto& s : _Pp.__sets) {
        if (_Pp.icase || _Pp.collate || !s.__coll_lo.empty() || !s.__equivs.empty()) {
          for (unsigned __u = 0; __u < 256; ++__u)
            if (_Pp.__set_slow(__tr_, s, static_cast<__charT>(__u)))
              __set_bit(s.__cache, __u);
          continue;
        }
        // What set_slow decides for c below 256 when nothing is translated, folded or collated:
        // the listed characters, the ranges by code, the classes, the negated classes.
        for (__charT __ch : s.__chars)
          if (__re_ord(__ch) < 256u)
            __set_bit(s.__cache, unsigned(__re_ord(__ch)));
        for (const auto& r : s.ranges)
          for (unsigned long long __u = __re_ord(r.__lo); __u <= __re_ord(r.__hi) && __u < 256u; ++__u)
            __set_bit(s.__cache, unsigned(__u));
        if (s.__classes != __class_type{}) {
          const unsigned char* b = __bits_of(s.__classes);
          for (unsigned i = 0; i < 32; ++i)
            s.__cache[i] |= b[i];
        }
        for (const __class_type __f : s.__neg_classes) {
          const unsigned char* b = __bits_of(__f);
          for (unsigned i = 0; i < 32; ++i)
            s.__cache[i] |= static_cast<unsigned char>(~b[i]);
        }
        if (s.negate)
          for (unsigned i = 0; i < 32; ++i)
            s.__cache[i] = static_cast<unsigned char>(~s.__cache[i]);
      }
    }
  }

public:
  __re_compiler(const __traits& __tr, __re_program<__charT, __traits>& _Pp) : __tr_(__tr), _P_(_Pp) {}

  void __compile(const __charT* first, const __charT* last, std::regex_constants::syntax_option_type __f) {
    namespace __rc = std::regex_constants;
    auto& _Pp = _P_;
    _Pp.flags = __f;
    _Pp.icase = (__f & __rc::icase) != 0;
    _Pp.collate = (__f & __rc::collate) != 0;
    // [re.synopt]/1: a valid value has at most one grammar element. error_type has no code for
    // this; error_complexity says the expression cannot be handled.
    const unsigned __grammars = unsigned(__f & (__rc::ECMAScript | __rc::basic | __rc::extended | __rc::awk | __rc::grep | __rc::egrep));
    if ((__grammars & (__grammars - 1)) != 0)
      fail(__rc::error_complexity);
    if ((__f & __rc::basic) || (__f & __rc::grep))
      __g_ = __bre;
    else if ((__f & __rc::extended) || (__f & __rc::egrep))
      __g_ = __ere;
    else if (__f & __rc::awk)
      __g_ = __awk_g;
    else
      __g_ = __ecma;
    _Pp.__ecma = __g_ == __ecma;
    _Pp.posix = !_Pp.__ecma;
    _Pp.multiline = _Pp.__ecma && (__f & __rc::multiline) != 0;
    _Pp.__memo = true;
    __closed_.push_back(1); // group 0
    __p_ = first;
    __end_ = last;
    int __root;
    if ((__f & __rc::grep) || (__f & __rc::egrep)) {
      __root = __lines(__g_ == __bre);
    } else {
      __root = __g_ == __ecma ? __ecma_disjunction() : __g_ == __bre ? __bre_expr() : __ere_alt();
      if (__p_ != __end_)
        fail(__rc::error_paren);
    }
    if (__max_backref_ > _Pp.__groups)
      fail(__rc::error_backref);
    if (_Pp.__has_backref)
      _Pp.__memo = false;
    _Pp.__nfa = _Pp.posix && !_Pp.__has_backref && !_Pp.__needs_bt && __expanded_size(__root) <= __max_expanded;
    if (_Pp.__nfa) {
      __root = expand(__root);
      _Pp.__memo = false;
    }
    if (_Pp.posix) {
      _Pp.__node_begin.assign(__nodes_.size(), 0);
      _Pp.__node_end.assign(__nodes_.size(), 0);
    }
    __gen(__root);
    emit(__re_op::__match);
    if (_Pp.posix) {
      if (!_Pp.__nfa)
        __analyze(__root);
      _Pp.__nodes = static_cast<std::vector<node>&&>(__nodes_);
      _Pp.__root = __root;
    }
    finish();
  }
};

}} // namespace __ycxx::__detail
