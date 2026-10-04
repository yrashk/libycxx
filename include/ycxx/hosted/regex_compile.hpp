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

namespace ycxx::detail {

enum class re_kind : unsigned char {
  empty, chr, any, set, bol, eol, wordb, nwordb, backref, group, look, nlook, concat, alt, repeat
};

template <class charT>
struct re_node {
  re_kind kind = re_kind::empty;
  bool greedy = true;
  bool tail = false;      // POSIX expansion: a repetition that follows an iteration of its own
  charT ch{};
  int val = 0;            // set index; group or back-reference number
  int min = 0, max = 0;   // repeat; max < 0: unbounded
  int group_lo = 0, group_hi = 0; // repeat and group: the groups numbered inside, [lo, hi)
  std::vector<int> kids;
};

// The order of a character code: the unsigned value of an integral character type.
template <class charT>
constexpr auto re_ord(charT c) noexcept {
  if constexpr (std::is_integral_v<charT>)
    return static_cast<std::make_unsigned_t<charT>>(c);
  else
    return c;
}
// Code units below 256 have precomputed answers (case folding, set membership, word class).
template <class charT>
inline constexpr bool re_cacheable = std::is_integral_v<charT>;

template <class charT>
constexpr bool re_line_terminator(charT c) noexcept {
  if (c == charT('\n') || c == charT('\r'))
    return true;
  if constexpr (std::is_integral_v<charT> && sizeof(charT) > 1)
    return re_ord(c) == 0x2028u || re_ord(c) == 0x2029u;
  else
    return false;
}

template <class charT, class traits>
struct re_set {
  using string_type = typename traits::string_type;
  using class_type = typename traits::char_class_type;
  struct range {
    charT lo, hi;
  };
  bool negate = false;
  std::basic_string<charT> chars;  // translated
  std::vector<range> ranges;
  std::vector<string_type> coll_lo, coll_hi;  // collate: sort keys of the range ends
  class_type classes{};
  std::vector<class_type> neg_classes;        // \D, \S, \W inside brackets
  std::vector<string_type> equivs;            // primary sort keys
  unsigned char cache[32] = {};               // membership of the code units below 256
  unsigned char range_fold[32] = {};          // icase: the case-folded code units below 256 of
                                              // the characters of the ranges
};

enum class re_op : unsigned char {
  chr, any, set, split, jmp, open, close, bol, eol, wordb, nwordb, backref, look, look_end,
  loop_enter, loop_iter, loop_tail, rep, match
};

template <class charT>
struct re_inst {
  re_op op;
  bool flag = false; // any: stops at line terminators; look: negative
  charT ch{};        // chr: the translated character
  int a = 0, b = 0;  // split: preferred, other; jmp: target; set: index; open/close/backref: group;
                     // look: its look_end; loop_*, rep: loop index
};

struct re_loop {
  int min = 0, max = 0; // max < 0: unbounded
  bool greedy = true;
  int iter_pc = 0, exit_pc = 0;
  int group_lo = 0, group_hi = 0;
};

template <class charT, class traits>
struct re_program {
  using set_type = re_set<charT, traits>;
  using class_type = typename traits::char_class_type;

  std::regex_constants::syntax_option_type flags{};
  bool icase = false, collate = false, multiline = false, ecma = true;
  bool posix = false;   // leftmost-longest
  bool nfa = false;     // compiled for the NFA simulation (POSIX without back-references)
  bool has_backref = false;
  bool memo = false;    // the backtracker may remember failed (pc, position) pairs
  int groups = 0;       // capturing groups (also counted under nosubs, for back-references)
  std::vector<re_inst<charT>> code;
  std::vector<re_loop> loops;
  std::vector<set_type> sets;
  // nfa: the expanded tree and each node's code range; the epsilon predecessors of each pc.
  std::vector<re_node<charT>> nodes;
  int root = -1;
  std::vector<int> node_begin, node_end;
  std::vector<std::vector<int>> eps_pred;
  // memo: the dense index of each pc where a failed (pc, position) pair is remembered, or -1.
  std::vector<int> memo_index;
  int memo_points = 0;
  std::vector<int> memo_loops;        // the loops whose body is nullable (at most 4)
  std::vector<unsigned> memo_mask;    // per memo point: bit k if inside an iteration of memo_loops[k]
  // Caches for the code units below 256.
  charT fold[256] = {};
  unsigned char word[32] = {};
  class_type word_class{};

  static bool bit(const unsigned char* bits, unsigned u) noexcept { return (bits[u >> 3] >> (u & 7)) & 1; }

  // The character as compared: [re.grammar]/14.1.
  charT tx(const traits& tr, charT c) const {
    if constexpr (re_cacheable<charT>) {
      if (re_ord(c) < 256u)
        return fold[re_ord(c)];
    }
    return icase ? tr.translate_nocase(c) : collate ? tr.translate(c) : c;
  }
  bool is_word(const traits& tr, charT c) const {
    if constexpr (re_cacheable<charT>) {
      if (re_ord(c) < 256u)
        return bit(word, unsigned(re_ord(c)));
    }
    return tr.isctype(c, word_class);
  }
  bool set_slow(const traits& tr, const set_type& s, charT c) const {
    const charT t = icase ? tr.translate_nocase(c) : collate ? tr.translate(c) : c;
    bool in = s.chars.find(t) != std::basic_string<charT>::npos;
    for (std::size_t i = 0; !in && i < s.ranges.size(); ++i) {
      const auto& r = s.ranges[i];
      in = (re_ord(r.lo) <= re_ord(c) && re_ord(c) <= re_ord(r.hi)) ||
           (icase && re_ord(r.lo) <= re_ord(t) && re_ord(t) <= re_ord(r.hi));
    }
    // icase: c is in a range if some character of the range folds to what c folds to ([T-f]
    // holds 't', since it holds 'T'); exact for the folded values below 256.
    if constexpr (re_cacheable<charT>) {
      if (!in && icase && re_ord(t) < 256u)
        in = bit(s.range_fold, unsigned(re_ord(t)));
    }
    // [re.grammar]/14.2: collating ranges compare sort keys; without regard to case, the folded
    // character is tried too.
    for (int pass = 0; !in && !s.coll_lo.empty() && pass < (icase ? 2 : 1); ++pass) {
      const charT k[1] = {pass == 0 ? tr.translate(c) : t};
      const auto key = tr.transform(k, k + 1);
      for (std::size_t i = 0; !in && i < s.coll_lo.size(); ++i)
        in = !(key < s.coll_lo[i]) && !(s.coll_hi[i] < key);
    }
    if (!in && s.classes != class_type{})
      in = tr.isctype(c, s.classes);
    for (std::size_t i = 0; !in && i < s.neg_classes.size(); ++i)
      in = !tr.isctype(c, s.neg_classes[i]);
    if (!in && !s.equivs.empty()) {
      const charT k[1] = {c};
      const auto key = tr.transform_primary(k, k + 1);
      for (std::size_t i = 0; !in && i < s.equivs.size(); ++i)
        in = key == s.equivs[i];
    }
    return in != s.negate;
  }
  bool in_set(const traits& tr, int idx, charT c) const {
    const set_type& s = sets[static_cast<std::size_t>(idx)];
    if constexpr (re_cacheable<charT>) {
      if (re_ord(c) < 256u)
        return bit(s.cache, unsigned(re_ord(c)));
    }
    return set_slow(tr, s, c);
  }
  // Does the single-character instruction at pc accept c?
  bool single(const traits& tr, int pc, charT c) const {
    const re_inst<charT>& in = code[static_cast<std::size_t>(pc)];
    switch (in.op) {
    case re_op::chr:
      return tx(tr, c) == in.ch;
    case re_op::any:
      return !in.flag || !::ycxx::detail::re_line_terminator(c);
    default:
      return in_set(tr, in.a, c);
    }
  }
};

// ---- the translator ---------------------------------------------------------------------------
template <class charT, class traits>
class re_compiler {
  using rc_err = std::regex_constants::error_type;
  using string_type = typename traits::string_type;
  using class_type = typename traits::char_class_type;
  using node = re_node<charT>;
  using set_type = re_set<charT, traits>;

  // Limits on the size of a translated expression (error_space beyond them): the tree and the
  // code, and the nesting of groups (the translator and the matchers recurse over the tree).
  static constexpr std::size_t max_nodes = 1u << 20;
  static constexpr int max_depth = 1000;
  static constexpr int max_count = 0x7fffffff;
  // POSIX: bounded repetitions are expanded for the NFA only up to this many nodes in all and
  // this many copies of one atom; beyond, the program backtracks (keeping the longest match).
  static constexpr std::size_t max_expanded = 1u << 16;
  static constexpr int max_copies = 256;

  const traits& tr_;
  re_program<charT, traits>& P_;
  const charT* p_;
  const charT* end_;
  std::vector<node> nodes_;
  std::vector<char> closed_; // POSIX: closed_[n] once group n is complete (not vector<bool>, which <vector> specializes)
  int max_backref_ = 0;
  int depth_ = 0;
  enum grammar { ecma, bre, ere, awk_g } g_ = ecma;

  [[noreturn]] static void fail(rc_err e) { ::ycxx::detail::throw_regex_error(e); }
  void enter() {
    if (++depth_ > max_depth)
      fail(std::regex_constants::error_space);
  }

  bool at(char c) const { return p_ != end_ && *p_ == static_cast<charT>(c); }
  bool at2(char c, char d) const { return at(c) && p_ + 1 != end_ && p_[1] == static_cast<charT>(d); }
  int digit(charT c, int radix) const { return tr_.value(c, radix); }

  int add(node n) {
    if (nodes_.size() >= max_nodes)
      fail(std::regex_constants::error_space);
    nodes_.push_back(static_cast<node&&>(n));
    return static_cast<int>(nodes_.size() - 1);
  }
  int leaf(re_kind k) {
    node n;
    n.kind = k;
    return add(static_cast<node&&>(n));
  }
  int literal(charT c) {
    node n;
    n.kind = re_kind::chr;
    n.ch = c;
    return add(static_cast<node&&>(n));
  }
  int list(re_kind k, std::vector<int>& kids) {
    if (kids.empty())
      return leaf(re_kind::empty);
    if (kids.size() == 1)
      return kids[0];
    node n;
    n.kind = k;
    n.kids = static_cast<std::vector<int>&&>(kids);
    return add(static_cast<node&&>(n));
  }
  int repeat(int atom, int mn, int mx, bool greedy, int g_lo) {
    node n;
    n.kind = re_kind::repeat;
    n.min = mn;
    n.max = mx;
    n.greedy = greedy;
    n.group_lo = g_lo;
    n.group_hi = P_.groups + 1;
    n.kids.push_back(atom);
    return add(static_cast<node&&>(n));
  }
  int open_group() {
    ++P_.groups;
    closed_.push_back(0);
    return P_.groups;
  }
  int group(int num, int inner) {
    node n;
    n.kind = re_kind::group;
    n.val = num;
    n.group_lo = num;
    n.group_hi = P_.groups + 1;
    n.kids.push_back(inner);
    closed_[static_cast<std::size_t>(num)] = 1;
    return add(static_cast<node&&>(n));
  }
  int backref(int num) {
    node n;
    n.kind = re_kind::backref;
    n.val = num;
    P_.has_backref = true;
    if (num > max_backref_)
      max_backref_ = num;
    return add(static_cast<node&&>(n));
  }
  int new_set(set_type&& s) {
    P_.sets.push_back(static_cast<set_type&&>(s));
    node n;
    n.kind = re_kind::set;
    n.val = static_cast<int>(P_.sets.size() - 1);
    return add(static_cast<node&&>(n));
  }
  int class_escape(charT e) {
    // \d \D \s \S \w \W: [re.grammar]/7.
    const char name = static_cast<char>(re_ord(e) | 0x20); // the lower-case letter
    const charT nm[1] = {static_cast<charT>(name)};
    set_type s;
    s.classes = tr_.lookup_classname(nm, nm + 1, P_.icase);
    s.negate = e != static_cast<charT>(name);
    return new_set(static_cast<set_type&&>(s));
  }
  charT translate(charT c) const {
    return P_.icase ? tr_.translate_nocase(c) : P_.collate ? tr_.translate(c) : c;
  }

  // A decimal number of a {} interval or a back-reference; error `e` on overflow.
  int number(rc_err e) {
    long long v = 0;
    while (p_ != end_ && digit(*p_, 10) >= 0) {
      v = v * 10 + digit(*p_, 10);
      if (v > max_count)
        fail(e);
      ++p_;
    }
    return static_cast<int>(v);
  }
  // The contents of an interval after its opening brace: m, m, or m,n, then the closing brace
  // ("}" or, in a BRE, "\}").
  void interval(int& mn, int& mx, bool bre) {
    if (p_ == end_)
      fail(std::regex_constants::error_brace);
    if (digit(*p_, 10) < 0)
      fail(std::regex_constants::error_badbrace);
    mn = number(std::regex_constants::error_badbrace);
    mx = mn;
    if (at(',')) {
      ++p_;
      mx = (p_ != end_ && digit(*p_, 10) >= 0) ? number(std::regex_constants::error_badbrace) : -1;
    }
    // Anything but the closing brace after the numbers (end of pattern, "{1,2,3}") leaves the
    // brace unmatched.
    if (!(bre ? at2('\\', '}') : at('}')))
      fail(std::regex_constants::error_brace);
    p_ += bre ? 2 : 1;
    if (mx >= 0 && mx < mn)
      fail(std::regex_constants::error_badbrace);
  }

  // ---- bracket expressions --------------------------------------------------------------------
  struct class_atom {
    enum { chr, cls, neg_cls, equiv } kind = chr;
    charT c{};
    class_type m{};
    string_type key;
  };
  // [:name:], [.name.] or [=name=], at "[" followed by one of ":.=".
  class_atom bracket_special() {
    const charT delim = p_[1];
    p_ += 2;
    const charT* q = p_;
    while (q != end_ && !(*q == delim && q + 1 != end_ && q[1] == static_cast<charT>(']')))
      ++q;
    if (q == end_)
      fail(std::regex_constants::error_brack);
    const charT* nb = p_;
    p_ = q + 2;
    class_atom a;
    if (delim == static_cast<charT>(':')) {
      a.kind = class_atom::cls;
      a.m = tr_.lookup_classname(nb, q, P_.icase);
      if (a.m == class_type{})
        fail(std::regex_constants::error_ctype);
      return a;
    }
    string_type name = tr_.lookup_collatename(nb, q);
    if (name.empty())
      fail(std::regex_constants::error_collate);
    if (delim == static_cast<charT>('.')) {
      if (name.size() != 1)
        fail(std::regex_constants::error_collate);
      a.c = name[0];
      return a;
    }
    a.kind = class_atom::equiv;
    a.key = tr_.transform_primary(name.begin(), name.end());
    if (a.key.empty())
      fail(std::regex_constants::error_collate);
    return a;
  }
  void add_atom(set_type& s, class_atom& a) {
    switch (a.kind) {
    case class_atom::chr:
      s.chars.push_back(translate(a.c));
      break;
    case class_atom::cls:
      s.classes |= a.m;
      break;
    case class_atom::neg_cls:
      s.neg_classes.push_back(a.m);
      break;
    case class_atom::equiv:
      s.equivs.push_back(static_cast<string_type&&>(a.key));
      break;
    }
  }
  void add_range(set_type& s, const class_atom& a, const class_atom& b) {
    if (a.kind != class_atom::chr || b.kind != class_atom::chr)
      fail(std::regex_constants::error_range);
    string_type lo, hi;
    if (P_.collate) {
      const charT ka[1] = {tr_.translate(a.c)};
      const charT kb[1] = {tr_.translate(b.c)};
      lo = tr_.transform(ka, ka + 1);
      hi = tr_.transform(kb, kb + 1);
      if (hi < lo)
        fail(std::regex_constants::error_range);
    } else {
      if (re_ord(b.c) < re_ord(a.c))
        fail(std::regex_constants::error_range);
      s.ranges.push_back({a.c, b.c});
    }
    // icase: the folded values of the range's characters below 256.
    if constexpr (re_cacheable<charT>) {
      if (P_.icase)
        for (unsigned u = 0; u < 256u; ++u) {
          const charT c = static_cast<charT>(u);
          bool in;
          if (P_.collate) {
            const charT k[1] = {tr_.translate(c)};
            const string_type key = tr_.transform(k, k + 1);
            in = !(key < lo) && !(hi < key);
          } else {
            in = re_ord(a.c) <= u && u <= re_ord(b.c);
          }
          const auto f = re_ord(tr_.translate_nocase(c));
          if (in && f < 256u)
            s.range_fold[f >> 3] |= static_cast<unsigned char>(1u << (f & 7));
        }
    }
    if (P_.collate) {
      s.coll_lo.push_back(static_cast<string_type&&>(lo));
      s.coll_hi.push_back(static_cast<string_type&&>(hi));
    }
  }
  // ECMAScript ClassAtom (with the [re.grammar]/3 additions).
  class_atom ecma_class_atom() {
    class_atom a;
    if (at('\\')) {
      ++p_;
      if (p_ == end_)
        fail(std::regex_constants::error_escape);
      const charT e = *p_;
      switch (static_cast<char>(re_ord(e) < 128u ? re_ord(e) : 0)) {
      case 'b':
        ++p_;
        a.c = charT(8);
        return a;
      case 'd': case 's': case 'w': case 'D': case 'S': case 'W': {
        ++p_;
        const char name = static_cast<char>(re_ord(e) | 0x20);
        const charT nm[1] = {static_cast<charT>(name)};
        a.m = tr_.lookup_classname(nm, nm + 1, P_.icase);
        a.kind = e == static_cast<charT>(name) ? class_atom::cls : class_atom::neg_cls;
        return a;
      }
      default:
        if (digit(e, 10) > 0) // a back-reference is not a character ([re.grammar], ES5 15.10.2.19)
          fail(std::regex_constants::error_escape);
        a.c = ecma_char_escape();
        return a;
      }
    }
    if (at('[') && p_ + 1 != end_ &&
        (p_[1] == static_cast<charT>(':') || p_[1] == static_cast<charT>('.') || p_[1] == static_cast<charT>('=')))
      return bracket_special();
    a.c = *p_++;
    return a;
  }
  // A POSIX bracket expression element ([[:class:]], [.coll.], [=equiv=], an awk escape).
  class_atom posix_class_atom() {
    if (at('[') && p_ + 1 != end_ &&
        (p_[1] == static_cast<charT>(':') || p_[1] == static_cast<charT>('.') || p_[1] == static_cast<charT>('=')))
      return bracket_special();
    class_atom a;
    if (g_ == awk_g && at('\\')) {
      ++p_;
      a.c = awk_escape();
      return a;
    }
    a.c = *p_++;
    return a;
  }
  int bracket() { // after '['
    set_type s;
    if (at('^')) {
      ++p_;
      s.negate = true;
    }
    for (bool first = true;; first = false) {
      if (p_ == end_)
        fail(std::regex_constants::error_brack);
      if (at(']') && (g_ == ecma || !first)) {
        ++p_;
        break;
      }
      class_atom a = g_ == ecma ? ecma_class_atom() : posix_class_atom();
      if (at('-') && p_ + 1 != end_ && p_[1] != static_cast<charT>(']')) {
        ++p_;
        class_atom b = g_ == ecma ? ecma_class_atom() : posix_class_atom();
        add_range(s, a, b);
      } else {
        add_atom(s, a);
      }
    }
    return new_set(static_cast<set_type&&>(s));
  }

  // ---- ECMAScript ---------------------------------------------------------------------------
  // CharacterEscape after the backslash (at *p_): ControlEscape, \cX, \xHH, \uHHHH, \0 and
  // IdentityEscape ("SourceCharacter but not c", [re.grammar]/3).
  charT ecma_char_escape() {
    const charT e = *p_++;
    const auto u = re_ord(e);
    if (u < 128u) {
      switch (static_cast<char>(u)) {
      case 'f': return charT(0x0C);
      case 'n': return charT(0x0A);
      case 'r': return charT(0x0D);
      case 't': return charT(0x09);
      case 'v': return charT(0x0B);
      case '0':
        if (p_ != end_ && digit(*p_, 10) >= 0)
          fail(std::regex_constants::error_escape);
        return charT(0);
      case 'c': {
        if (p_ == end_)
          fail(std::regex_constants::error_escape);
        const auto l = re_ord(*p_);
        if (!((l >= 'a' && l <= 'z') || (l >= 'A' && l <= 'Z')))
          fail(std::regex_constants::error_escape);
        ++p_;
        return charT(l % 32);
      }
      case 'x':
      case 'u': {
        const int n = u == 'x' ? 2 : 4;
        unsigned long v = 0;
        for (int i = 0; i < n; ++i) {
          const int d = p_ != end_ ? digit(*p_, 16) : -1;
          if (d < 0)
            fail(std::regex_constants::error_escape);
          v = v * 16 + static_cast<unsigned long>(d);
          ++p_;
        }
        // [re.grammar]/12: a value that does not fit in charT is an error.
        if constexpr (std::is_integral_v<charT>) {
          if (v > static_cast<unsigned long>(static_cast<std::make_unsigned_t<charT>>(-1)))
            fail(std::regex_constants::error_escape);
        }
        return static_cast<charT>(v);
      }
      default:
        break;
      }
    }
    return e;
  }
  void no_quantifier() {
    if (at('*') || at('+') || at('?') || at('{'))
      fail(std::regex_constants::error_badrepeat);
  }
  int ecma_disjunction() {
    enter();
    std::vector<int> alts;
    alts.push_back(ecma_alternative());
    while (at('|')) {
      ++p_;
      alts.push_back(ecma_alternative());
    }
    --depth_;
    return list(re_kind::alt, alts);
  }
  int ecma_alternative() {
    std::vector<int> terms;
    while (p_ != end_ && !at('|') && !at(')'))
      terms.push_back(ecma_term());
    return list(re_kind::concat, terms);
  }
  int ecma_term() {
    int n;
    if (at('^') || at('$')) {
      n = leaf(at('^') ? re_kind::bol : re_kind::eol);
      ++p_;
      no_quantifier();
      return n;
    }
    if (at2('\\', 'b') || at2('\\', 'B')) {
      n = leaf(p_[1] == static_cast<charT>('b') ? re_kind::wordb : re_kind::nwordb);
      p_ += 2;
      no_quantifier();
      return n;
    }
    if (at2('(', '?') && p_ + 2 != end_ && (p_[2] == static_cast<charT>('=') || p_[2] == static_cast<charT>('!'))) {
      const bool neg = p_[2] == static_cast<charT>('!');
      p_ += 3;
      const int inner = ecma_disjunction();
      if (!at(')'))
        fail(std::regex_constants::error_paren);
      ++p_;
      node x;
      x.kind = neg ? re_kind::nlook : re_kind::look;
      x.kids.push_back(inner);
      n = add(static_cast<node&&>(x));
      no_quantifier();
      return n;
    }
    const int g0 = P_.groups + 1;
    return quantifier(ecma_atom(), g0);
  }
  int ecma_atom() {
    const charT c = *p_;
    const auto u = re_ord(c);
    switch (static_cast<char>(u < 128u ? u : 0)) {
    case '.':
      ++p_;
      return leaf(re_kind::any);
    case '(': {
      ++p_;
      if (at2('?', ':')) {
        p_ += 2;
        const int inner = ecma_disjunction();
        if (!at(')'))
          fail(std::regex_constants::error_paren);
        ++p_;
        return inner;
      }
      if (at('?'))
        fail(std::regex_constants::error_badrepeat);
      const int num = open_group();
      const int inner = ecma_disjunction();
      if (!at(')'))
        fail(std::regex_constants::error_paren);
      ++p_;
      return group(num, inner);
    }
    case '[':
      ++p_;
      return bracket();
    case '\\': {
      ++p_;
      if (p_ == end_)
        fail(std::regex_constants::error_escape);
      const charT e = *p_;
      const auto eu = re_ord(e);
      if (eu < 128u) {
        switch (static_cast<char>(eu)) {
        case 'd': case 'D': case 's': case 'S': case 'w': case 'W':
          ++p_;
          return class_escape(e);
        default:
          break;
        }
      }
      if (digit(e, 10) > 0)
        return backref(number(std::regex_constants::error_backref));
      return literal(ecma_char_escape());
    }
    case '*': case '+': case '?': case '{':
      fail(std::regex_constants::error_badrepeat);
    default:
      ++p_;
      return literal(c);
    }
  }
  int quantifier(int atom, int g0) {
    if (p_ == end_)
      return atom;
    int mn, mx;
    if (at('*')) {
      mn = 0, mx = -1;
      ++p_;
    } else if (at('+')) {
      mn = 1, mx = -1;
      ++p_;
    } else if (at('?')) {
      mn = 0, mx = 1;
      ++p_;
    } else if (at('{')) {
      ++p_;
      interval(mn, mx, false);
    } else {
      return atom;
    }
    bool greedy = true;
    if (at('?')) {
      ++p_;
      greedy = false;
    }
    no_quantifier();
    return repeat(atom, mn, mx, greedy, g0);
  }

  // ---- POSIX ----------------------------------------------------------------------------------
  // An awk escape after the backslash ([re.synopt] awk: the escapes of POSIX awk).
  charT awk_escape() {
    if (p_ == end_)
      fail(std::regex_constants::error_escape);
    const charT e = *p_++;
    const auto u = re_ord(e);
    if (u < 128u) {
      switch (static_cast<char>(u)) {
      case 'a': return charT(7);
      case 'b': return charT(8);
      case 'f': return charT(12);
      case 'n': return charT(10);
      case 'r': return charT(13);
      case 't': return charT(9);
      case 'v': return charT(11);
      case '"': case '/': case '\\': case '^': case '.': case '[': case ']': case '$': case '(': case ')':
      case '|': case '*': case '+': case '?': case '{': case '}':
        return e;
      default:
        break;
      }
    }
    if (digit(e, 8) >= 0) {
      unsigned v = static_cast<unsigned>(digit(e, 8));
      for (int i = 0; i < 2 && p_ != end_ && digit(*p_, 8) >= 0; ++i)
        v = v * 8 + static_cast<unsigned>(digit(*p_++, 8));
      return static_cast<charT>(v);
    }
    fail(std::regex_constants::error_escape);
  }
  int posix_backref(charT e) {
    const int num = digit(e, 10);
    if (num > P_.groups || !closed_[static_cast<std::size_t>(num)])
      fail(std::regex_constants::error_backref);
    return backref(num);
  }
  static bool posix_special(charT e) {
    const auto u = re_ord(e);
    if (u >= 128u)
      return false;
    switch (static_cast<char>(u)) {
    case '^': case '.': case '[': case ']': case '$': case '(': case ')': case '|': case '*': case '+':
    case '?': case '{': case '}': case '\\':
      return true;
    default:
      return false;
    }
  }
  // A basic regular expression (basic, grep), up to the end or "\)".
  int bre_expr() {
    enter();
    std::vector<int> terms;
    if (at('^')) {
      ++p_;
      terms.push_back(leaf(re_kind::bol));
    }
    bool start = true;
    while (p_ != end_ && !at2('\\', ')')) {
      if (at('$') && (p_ + 1 == end_ || (p_[1] == static_cast<charT>('\\') && p_ + 2 != end_ && p_[2] == static_cast<charT>(')')))) {
        ++p_;
        terms.push_back(leaf(re_kind::eol));
        continue;
      }
      const int g0 = P_.groups + 1;
      int atom;
      if (start && at('*')) { // a leading '*' is an ordinary character
        ++p_;
        atom = literal(charT('*'));
      } else {
        atom = bre_atom();
      }
      start = false;
      for (;;) {
        int mn, mx;
        if (at('*')) {
          ++p_;
          mn = 0, mx = -1;
        } else if (at2('\\', '{')) {
          p_ += 2;
          interval(mn, mx, true);
        } else {
          break;
        }
        atom = repeat(atom, mn, mx, true, g0);
      }
      terms.push_back(atom);
    }
    --depth_;
    return list(re_kind::concat, terms);
  }
  int bre_atom() {
    const charT c = *p_;
    if (c == static_cast<charT>('.')) {
      ++p_;
      return leaf(re_kind::any);
    }
    if (c == static_cast<charT>('[')) {
      ++p_;
      return bracket();
    }
    if (c != static_cast<charT>('\\')) {
      ++p_;
      return literal(c);
    }
    if (p_ + 1 == end_)
      fail(std::regex_constants::error_escape);
    const charT e = p_[1];
    p_ += 2;
    if (e == static_cast<charT>('(')) {
      const int num = open_group();
      const int inner = bre_expr();
      if (!at2('\\', ')'))
        fail(std::regex_constants::error_paren);
      p_ += 2;
      return group(num, inner);
    }
    if (e == static_cast<charT>('{'))
      fail(std::regex_constants::error_badrepeat);
    if (e == static_cast<charT>('}'))
      fail(std::regex_constants::error_brace);
    if (digit(e, 10) > 0)
      return posix_backref(e);
    const auto u = re_ord(e);
    if (u < 128u && (static_cast<char>(u) == '.' || static_cast<char>(u) == '[' || static_cast<char>(u) == ']' ||
                     static_cast<char>(u) == '\\' || static_cast<char>(u) == '*' || static_cast<char>(u) == '^' ||
                     static_cast<char>(u) == '$'))
      return literal(e);
    fail(std::regex_constants::error_escape);
  }
  // An extended regular expression (extended, egrep, awk).
  int ere_alt() {
    enter();
    std::vector<int> alts;
    alts.push_back(ere_branch());
    while (at('|')) {
      ++p_;
      alts.push_back(ere_branch());
    }
    --depth_;
    return list(re_kind::alt, alts);
  }
  int ere_branch() {
    std::vector<int> terms;
    while (p_ != end_ && !at('|') && !at(')'))
      terms.push_back(ere_expr());
    return list(re_kind::concat, terms);
  }
  int ere_expr() {
    const charT c = *p_;
    const auto u = re_ord(c);
    const int g0 = P_.groups + 1;
    int atom;
    switch (static_cast<char>(u < 128u ? u : 0)) {
    case '^':
    case '$':
      ++p_;
      atom = leaf(c == static_cast<charT>('^') ? re_kind::bol : re_kind::eol);
      no_quantifier();
      return atom;
    case '*': case '+': case '?': case '{':
      fail(std::regex_constants::error_badrepeat);
    case '(': {
      ++p_;
      const int num = open_group();
      const int inner = ere_alt();
      if (!at(')'))
        fail(std::regex_constants::error_paren);
      ++p_;
      atom = group(num, inner);
      break;
    }
    case '.':
      ++p_;
      atom = leaf(re_kind::any);
      break;
    case '[':
      ++p_;
      atom = bracket();
      break;
    case '\\': {
      ++p_;
      if (g_ == awk_g) {
        atom = literal(awk_escape());
        break;
      }
      if (p_ == end_)
        fail(std::regex_constants::error_escape);
      const charT e = *p_++;
      if (digit(e, 10) > 0)
        atom = posix_backref(e);
      else if (posix_special(e))
        atom = literal(e);
      else
        fail(std::regex_constants::error_escape);
      break;
    }
    default:
      ++p_;
      atom = literal(c);
      break;
    }
    for (;;) {
      int mn, mx;
      if (at('*')) {
        mn = 0, mx = -1;
      } else if (at('+')) {
        mn = 1, mx = -1;
      } else if (at('?')) {
        mn = 0, mx = 1;
      } else if (at('{')) {
        ++p_;
        interval(mn, mx, false);
        atom = repeat(atom, mn, mx, true, g0);
        continue;
      } else {
        break;
      }
      ++p_;
      atom = repeat(atom, mn, mx, true, g0);
    }
    return atom;
  }
  // grep and egrep: the lines of the pattern are alternatives.
  int lines(bool bre) {
    std::vector<int> alts;
    const charT* all_end = end_;
    for (;;) {
      const charT* nl = p_;
      while (nl != all_end && *nl != static_cast<charT>('\n'))
        ++nl;
      end_ = nl;
      alts.push_back(bre ? bre_expr() : ere_alt());
      if (p_ != end_)
        fail(std::regex_constants::error_paren);
      end_ = all_end;
      if (nl == all_end)
        break;
      p_ = nl + 1;
    }
    return list(re_kind::alt, alts);
  }

  // ---- analysis and code generation -------------------------------------------------------------
  bool nullable(int n) const {
    const node& x = nodes_[static_cast<std::size_t>(n)];
    switch (x.kind) {
    case re_kind::chr:
    case re_kind::any:
    case re_kind::set:
      return false;
    case re_kind::group:
      return nullable(x.kids[0]);
    case re_kind::concat:
      for (int k : x.kids)
        if (!nullable(k))
          return false;
      return true;
    case re_kind::alt:
      for (int k : x.kids)
        if (nullable(k))
          return true;
      return false;
    case re_kind::repeat:
      return x.min == 0 || nullable(x.kids[0]);
    default: // empty, assertions, lookahead, back-reference
      return true;
    }
  }
  bool single_char(int n) const {
    const re_kind k = nodes_[static_cast<std::size_t>(n)].kind;
    return k == re_kind::chr || k == re_kind::any || k == re_kind::set;
  }
  // The number of nodes expand() makes of n, saturated at max_expanded + 1; also too large when
  // one atom would be copied more than max_copies times.
  std::size_t expanded_size(int n) const {
    const node& x = nodes_[static_cast<std::size_t>(n)];
    std::size_t s = 1;
    for (int k : x.kids)
      s += expanded_size(k);
    if (x.kind == re_kind::repeat) {
      const int copies = x.max < 0 ? x.min + 1 : x.max;
      if (copies > max_copies)
        return max_expanded + 1;
      s *= static_cast<std::size_t>(copies < 1 ? 1 : copies) * 2;
    }
    return s > max_expanded ? max_expanded + 1 : s;
  }
  // Copies the subtree n (POSIX expansion of bounded repetitions).
  int clone(int n) {
    node x = nodes_[static_cast<std::size_t>(n)];
    for (int& k : x.kids)
      k = clone(k);
    return add(static_cast<node&&>(x));
  }
  // Rewrites every repetition into x?, x* and concatenations (nfa programs).
  int expand(int n) {
    const std::size_t nk = nodes_[static_cast<std::size_t>(n)].kids.size();
    for (std::size_t i = 0; i < nk; ++i) {
      const int k = expand(nodes_[static_cast<std::size_t>(n)].kids[i]);
      nodes_[static_cast<std::size_t>(n)].kids[i] = k;
    }
    const node x = nodes_[static_cast<std::size_t>(n)];
    if (x.kind != re_kind::repeat || (x.min == 0 && (x.max == 1 || x.max < 0)))
      return n;
    if (x.max == 0)
      return leaf(re_kind::empty);
    const int body = x.kids[0];
    if (x.min == 1 && x.max == 1)
      return body;
    // A tail follows another iteration of the same repetition: it makes no empty iteration.
    auto opt = [&](int kid, int mx, bool tail) {
      node r;
      r.kind = re_kind::repeat;
      r.min = 0;
      r.max = mx;
      r.tail = tail;
      r.group_lo = x.group_lo;
      r.group_hi = x.group_hi;
      r.kids.push_back(kid);
      return add(static_cast<node&&>(r));
    };
    std::vector<int> seq;
    for (int i = 0; i < x.min; ++i)
      seq.push_back(i == 0 ? body : clone(body));
    if (x.max < 0) {
      seq.push_back(opt(x.min == 0 ? body : clone(body), -1, x.min > 0));
    } else if (x.max > x.min) { // x{0,3} is (x(x(x)?)?)?
      int tail = opt(x.min == 0 ? body : clone(body), 1, x.min > 0 || x.max - x.min > 1);
      for (int i = x.min + 1; i < x.max; ++i) {
        std::vector<int> pair{clone(body), tail};
        tail = opt(list(re_kind::concat, pair), 1, x.min > 0 || i + 1 < x.max);
      }
      seq.push_back(tail);
    }
    return list(re_kind::concat, seq);
  }

  int emit(re_op op, int a = 0, bool flag = false) {
    if (P_.code.size() >= max_nodes)
      fail(std::regex_constants::error_space);
    re_inst<charT> in;
    in.op = op;
    in.a = a;
    in.flag = flag;
    P_.code.push_back(in);
    return static_cast<int>(P_.code.size() - 1);
  }
  int pc() const { return static_cast<int>(P_.code.size()); }
  re_inst<charT>& at_pc(int i) { return P_.code[static_cast<std::size_t>(i)]; }

  void gen(int n) {
    const node x = nodes_[static_cast<std::size_t>(n)]; // gen never adds nodes, but keep a copy
    if (P_.nfa)
      P_.node_begin[static_cast<std::size_t>(n)] = pc();
    switch (x.kind) {
    case re_kind::empty:
      break;
    case re_kind::chr:
      at_pc(emit(re_op::chr)).ch = translate(x.ch);
      break;
    case re_kind::any:
      emit(re_op::any, 0, P_.ecma);
      break;
    case re_kind::set:
      emit(re_op::set, x.val);
      break;
    case re_kind::bol:
      emit(re_op::bol);
      break;
    case re_kind::eol:
      emit(re_op::eol);
      break;
    case re_kind::wordb:
      emit(re_op::wordb);
      break;
    case re_kind::nwordb:
      emit(re_op::nwordb);
      break;
    case re_kind::backref:
      emit(re_op::backref, x.val);
      break;
    case re_kind::group:
      emit(re_op::open, x.val);
      gen(x.kids[0]);
      emit(re_op::close, x.val);
      break;
    case re_kind::look:
    case re_kind::nlook: {
      const int l = emit(re_op::look, 0, x.kind == re_kind::nlook);
      gen(x.kids[0]);
      at_pc(l).a = emit(re_op::look_end);
      break;
    }
    case re_kind::concat:
      for (int k : x.kids)
        gen(k);
      break;
    case re_kind::alt: {
      std::vector<int> jumps;
      for (std::size_t i = 0; i < x.kids.size(); ++i) {
        if (i + 1 < x.kids.size()) {
          const int s = emit(re_op::split);
          at_pc(s).a = s + 1;
          gen(x.kids[i]);
          jumps.push_back(emit(re_op::jmp));
          at_pc(s).b = pc();
        } else {
          gen(x.kids[i]);
        }
      }
      for (int j : jumps)
        at_pc(j).a = pc();
      break;
    }
    case re_kind::repeat:
      gen_repeat(x);
      break;
    }
    if (P_.nfa)
      P_.node_end[static_cast<std::size_t>(n)] = pc();
  }
  void gen_repeat(const node& x) {
    if (P_.nfa) { // only x? and x* remain after expand()
      const int s = emit(re_op::split);
      at_pc(s).a = s + 1;
      gen(x.kids[0]);
      if (x.max < 0)
        emit(re_op::jmp, s);
      at_pc(s).b = pc();
      return;
    }
    if (x.max == 0)
      return;
    if (x.min == 1 && x.max == 1) {
      gen(x.kids[0]);
      return;
    }
    re_loop lp;
    lp.min = x.min;
    lp.max = x.max;
    lp.greedy = x.greedy;
    lp.group_lo = x.group_lo;
    lp.group_hi = x.group_hi;
    const int li = static_cast<int>(P_.loops.size());
    P_.loops.push_back(lp);
    if (single_char(x.kids[0])) {
      emit(re_op::rep, li);
      gen(x.kids[0]);
      P_.loops[static_cast<std::size_t>(li)].exit_pc = pc();
      return;
    }
    emit(re_op::loop_enter, li);
    const int it = emit(re_op::loop_iter, li);
    gen(x.kids[0]);
    emit(re_op::loop_tail, li);
    P_.loops[static_cast<std::size_t>(li)].iter_pc = it;
    P_.loops[static_cast<std::size_t>(li)].exit_pc = pc();
    // Whether the backtracker may remember failures: what follows a pc must not depend on the
    // loop's counter, so max is 1 or unbounded, and min at most 1. An iteration that can match
    // the empty string adds one bit of state, whether it began at the current position (the
    // empty check), which joins the memo key; only for min == 0, and for at most 4 such loops.
    const bool counts_ok = x.max == 1 || (x.max < 0 && x.min <= 1);
    if (!counts_ok)
      P_.memo = false;
    else if (nullable(x.kids[0])) {
      if (x.min != 0 || P_.memo_loops.size() == 4)
        P_.memo = false;
      else
        P_.memo_loops.push_back(li);
    }
  }

  void finish() {
    auto& P = P_;
    // The memo points: where the backtracker resumes or branches.
    if (P.memo) {
      P.memo_index.assign(P.code.size(), -1);
      auto mark = [&](int i) {
        if (P.memo_index[static_cast<std::size_t>(i)] < 0)
          P.memo_index[static_cast<std::size_t>(i)] = P.memo_points++;
      };
      for (std::size_t i = 0; i < P.code.size(); ++i) {
        const auto& in = P.code[i];
        if (in.op == re_op::split) {
          mark(in.a);
          mark(in.b);
        } else if (in.op == re_op::look) {
          mark(static_cast<int>(i));
        }
      }
      for (const re_loop& lp : P.loops) {
        if (lp.iter_pc != 0)
          mark(lp.iter_pc);
        mark(lp.exit_pc);
      }
      // A lookahead's body succeeds by reaching its look_end without ending the search, so a
      // (pc, position) pair visited there may have succeeded: no memo inside lookaheads (the
      // look instruction itself is a memo point: what follows it depends only on the position).
      for (std::size_t i = 0; i < P.code.size(); ++i)
        if (P.code[i].op == re_op::look)
          for (int j = static_cast<int>(i) + 1; j <= P.code[i].a; ++j)
            P.memo_index[static_cast<std::size_t>(j)] = -1;
      // For each memo point, the nullable-body loops whose iteration it lies in.
      P.memo_mask.assign(static_cast<std::size_t>(P.memo_points), 0);
      for (std::size_t i = 0; i < P.code.size(); ++i) {
        const int mp = P.memo_index[i];
        if (mp < 0)
          continue;
        for (std::size_t k = 0; k < P.memo_loops.size(); ++k) {
          const re_loop& lp = P.loops[static_cast<std::size_t>(P.memo_loops[k])];
          if (static_cast<int>(i) > lp.iter_pc && static_cast<int>(i) < lp.exit_pc)
            P.memo_mask[static_cast<std::size_t>(mp)] |= 1u << k;
        }
      }
    }
    if (P.nfa) {
      P.eps_pred.assign(P.code.size() + 1, {});
      for (std::size_t i = 0; i < P.code.size(); ++i) {
        const auto& in = P.code[i];
        const int ii = static_cast<int>(i);
        switch (in.op) {
        case re_op::split:
          P.eps_pred[static_cast<std::size_t>(in.a)].push_back(ii);
          P.eps_pred[static_cast<std::size_t>(in.b)].push_back(ii);
          break;
        case re_op::jmp:
          P.eps_pred[static_cast<std::size_t>(in.a)].push_back(ii);
          break;
        case re_op::open: case re_op::close: case re_op::bol: case re_op::eol: case re_op::wordb: case re_op::nwordb:
          P.eps_pred[i + 1].push_back(ii);
          break;
        default:
          break;
        }
      }
    }
    // Caches for the code units below 256.
    {
      const char w[1] = {'w'};
      charT wn[1] = {static_cast<charT>(w[0])};
      P.word_class = tr_.lookup_classname(wn, wn + 1, false);
    }
    if constexpr (re_cacheable<charT>) {
      for (unsigned u = 0; u < 256; ++u) {
        const charT c = static_cast<charT>(u);
        P.fold[u] = P.icase ? tr_.translate_nocase(c) : P.collate ? tr_.translate(c) : c;
        if (tr_.isctype(c, P.word_class))
          P.word[u >> 3] |= static_cast<unsigned char>(1u << (u & 7));
        for (auto& s : P.sets)
          if (P.set_slow(tr_, s, c))
            s.cache[u >> 3] |= static_cast<unsigned char>(1u << (u & 7));
      }
    }
  }

public:
  re_compiler(const traits& tr, re_program<charT, traits>& P) : tr_(tr), P_(P) {}

  void compile(const charT* first, const charT* last, std::regex_constants::syntax_option_type f) {
    namespace rc = std::regex_constants;
    auto& P = P_;
    P.flags = f;
    P.icase = (f & rc::icase) != 0;
    P.collate = (f & rc::collate) != 0;
    // [re.synopt]/1: a valid value has at most one grammar element. error_type has no code for
    // this; error_complexity says the expression cannot be handled.
    const unsigned grammars = unsigned(f & (rc::ECMAScript | rc::basic | rc::extended | rc::awk | rc::grep | rc::egrep));
    if ((grammars & (grammars - 1)) != 0)
      fail(rc::error_complexity);
    if ((f & rc::basic) || (f & rc::grep))
      g_ = bre;
    else if ((f & rc::extended) || (f & rc::egrep))
      g_ = ere;
    else if (f & rc::awk)
      g_ = awk_g;
    else
      g_ = ecma;
    P.ecma = g_ == ecma;
    P.posix = !P.ecma;
    P.multiline = P.ecma && (f & rc::multiline) != 0;
    P.memo = true;
    closed_.push_back(1); // group 0
    p_ = first;
    end_ = last;
    int root;
    if ((f & rc::grep) || (f & rc::egrep)) {
      root = lines(g_ == bre);
    } else {
      root = g_ == ecma ? ecma_disjunction() : g_ == bre ? bre_expr() : ere_alt();
      if (p_ != end_)
        fail(rc::error_paren);
    }
    if (max_backref_ > P.groups)
      fail(rc::error_backref);
    if (P.has_backref)
      P.memo = false;
    P.nfa = P.posix && !P.has_backref && expanded_size(root) <= max_expanded;
    if (P.nfa) {
      root = expand(root);
      P.node_begin.assign(nodes_.size(), 0);
      P.node_end.assign(nodes_.size(), 0);
      P.memo = false;
    }
    gen(root);
    emit(re_op::match);
    if (P.nfa) {
      P.nodes = static_cast<std::vector<node>&&>(nodes_);
      P.root = root;
    }
    finish();
  }
};

} // namespace ycxx::detail
