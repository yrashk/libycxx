// libycxx hosted: the regular expression matchers.
//
// - re_backtracker runs ECMAScript programs (first match in ECMA-262's priority order) and POSIX
//   programs with back-references (every path is explored and the longest match kept). It
//   backtracks on an explicit stack of choice points and undo records, never by recursion
//   (only a lookahead runs as a nested call, so the depth is bounded by the pattern).
//   Programs without back-references whose loops cannot repeat an empty iteration remember the
//   (pc, position) pairs from which matching already failed, which bounds the work by
//   (program size) x (input length) per search; others count their steps and throw
//   regex_error(error_complexity) beyond a limit, and every program throws error_stack when the
//   backtracking stack grows beyond a limit.
// - re_nfa finds the leftmost-longest match of a POSIX program by simulating its Thompson NFA
//   (threads ordered by start position), in time (program size) x (input length).
// - re_posix_sub then assigns the subexpressions inside that match by POSIX's rule: from left to
//   right, each subpattern matches the longest string that still lets the whole pattern match
//   ([re.synopt] basic/extended, POSIX Base Definitions 9.1/9.3.6). It walks the syntax tree with
//   the set of ends a node can reach (forward NFA simulation) and the set of positions from which
//   the rest of the enclosing node can still reach its end (backward simulation).
#pragma once

#include <ycxx/hosted/regex_compile.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class It>
struct re_cap {
  It first{}, second{};
  bool matched = false;
};

// The zero-width assertions ([re.matchflag], [re.grammar]).
template <class charT, class traits>
bool re_assert(const re_program<charT, traits>& P, const traits& tr, re_op op, std::regex_constants::match_flag_type f,
               bool at_first, bool has_prev, charT prev, bool at_end, charT cur) {
  namespace rc = std::regex_constants;
  const bool prev_avail = (f & rc::match_prev_avail) != 0;
  switch (op) {
  case re_op::bol:
    if (at_first && !prev_avail)
      return (f & rc::match_not_bol) == 0;
    return has_prev && P.multiline && ::ycxx::detail::re_line_terminator(prev);
  case re_op::eol:
    if (at_end)
      return (f & rc::match_not_eol) == 0;
    return P.multiline && ::ycxx::detail::re_line_terminator(cur);
  default: {
    bool b;
    if (at_first && !prev_avail && (f & rc::match_not_bow) != 0)
      b = false;
    else if (at_end && (f & rc::match_not_eow) != 0)
      b = false;
    else
      b = (has_prev && P.is_word(tr, prev)) != (!at_end && P.is_word(tr, cur));
    return op == re_op::wordb ? b : !b;
  }
  }
}

// A set of pcs with constant-time insert, test and clear (Briggs and Torczon).
struct re_pc_set {
  std::vector<int> dense, sparse;
  std::size_t n = 0;
  void init(std::size_t size) {
    dense.assign(size, 0);
    sparse.assign(size, 0);
    n = 0;
  }
  bool contains(int pc) const {
    const auto s = static_cast<std::size_t>(sparse[static_cast<std::size_t>(pc)]);
    return s < n && dense[s] == pc;
  }
  void insert(int pc) {
    sparse[static_cast<std::size_t>(pc)] = static_cast<int>(n);
    dense[n++] = pc;
  }
  void clear() { n = 0; }
};

[[noreturn]] [[gnu::cold]] inline void re_throw_complexity() {
  ::ycxx::detail::throw_regex_error(std::regex_constants::error_complexity);
}
[[noreturn]] [[gnu::cold]] inline void re_throw_stack() {
  ::ycxx::detail::throw_regex_error(std::regex_constants::error_stack);
}

// ---- backtracking ------------------------------------------------------------------------------
template <class It, class charT, class traits>
class re_backtracker {
  using prog = re_program<charT, traits>;
  using flag_t = std::regex_constants::match_flag_type;

  struct slot {
    It it{};
    bool set = false;
  };
  struct loop_state {
    std::ptrdiff_t count = 0;
    It start{};
    std::ptrdiff_t start_idx = -1;
  };
  enum kind : unsigned char { k_choice, k_slot, k_loop, k_greedy, k_lazy };
  struct frame {
    kind k;
    bool flag;
    int a;
    std::ptrdiff_t n;
    It it;
    std::ptrdiff_t idx;
  };

  static constexpr std::size_t max_frames = std::size_t(1) << 22;
  static constexpr std::size_t max_memo_words = std::size_t(1) << 22;

  const prog& P_;
  const traits& tr_;
  It first_, last_;
  flag_t flags_;
  bool whole_ = false, longest_ = false, memo_on_ = false;
  std::ptrdiff_t start_idx_ = 0, max_idx_ = 0;
  std::vector<slot> caps_;
  std::vector<loop_state> ls_;
  std::vector<frame> stack_;
  std::vector<unsigned long long> memo_;
  std::size_t memo_wpp_ = 0;
  std::size_t steps_ = 0, next_check_ = 0;
  // longest_: the best match so far
  std::ptrdiff_t best_len_ = -1;
  It best_end_{};
  std::vector<slot> best_caps_;

  void push(kind k, int a, std::ptrdiff_t n, const It& it, std::ptrdiff_t idx, bool flag = false) {
    if (stack_.size() >= max_frames)
      ::ycxx::detail::re_throw_stack();
    stack_.push_back(frame{k, flag, a, n, it, idx});
  }
  void set_slot(int i, const It& it, bool set) {
    slot& s = caps_[static_cast<std::size_t>(i)];
    push(k_slot, i, 0, s.it, 0, s.set);
    s.it = it;
    s.set = set;
  }
  void save_loop(int l) {
    const loop_state& s = ls_[static_cast<std::size_t>(l)];
    push(k_loop, l, s.count, s.start, s.start_idx);
  }
  // Counts steps; past a limit that grows with the input reached, the match is too complex.
  void tick(std::ptrdiff_t idx) {
    if (idx > max_idx_)
      max_idx_ = idx;
    if (++steps_ < next_check_)
      return;
    next_check_ = steps_ + (std::size_t(1) << 16);
    const std::size_t limit = std::size_t(20000000) + std::size_t(32) * static_cast<std::size_t>(max_idx_ + 1) * P_.code.size();
    if (steps_ > limit)
      ::ycxx::detail::re_throw_complexity();
  }
  // The memo point mp, refined by whether each enclosing nullable-body loop began its current
  // iteration at idx (the only state, besides pc and idx, that the rest of the match depends on).
  int memo_key(int mp, std::ptrdiff_t idx) const {
    const unsigned mask = P_.memo_mask[static_cast<std::size_t>(mp)];
    unsigned v = 0;
    for (std::size_t k = 0; k < P_.memo_loops.size(); ++k)
      if ((mask >> k) & 1u)
        if (ls_[static_cast<std::size_t>(P_.memo_loops[k])].start_idx == idx)
          v |= 1u << k;
    return mp << P_.memo_loops.size() | static_cast<int>(v);
  }
  // false if (memo key mp, idx) was visited before; marks it.
  bool visit(int mp, std::ptrdiff_t idx) {
    const std::size_t w = static_cast<std::size_t>(idx) * memo_wpp_ + static_cast<std::size_t>(mp) / 64;
    if (w >= memo_.size()) {
      if (w >= max_memo_words)
        return true;
      std::size_t n = memo_.size() * 2;
      if (n <= w)
        n = w + 1 + memo_wpp_ * 64;
      memo_.resize(n < max_memo_words ? n : max_memo_words, 0);
    }
    const unsigned long long b = 1ull << (static_cast<unsigned>(mp) % 64);
    if (memo_[w] & b)
      return false;
    memo_[w] |= b;
    return true;
  }
  bool check(re_op op, const It& cur) const {
    const bool at_first = cur == first_;
    const bool has_prev = !at_first || (flags_ & std::regex_constants::match_prev_avail) != 0;
    charT prev{}, c{};
    if (has_prev) {
      It q = cur;
      --q;
      prev = *q;
    }
    const bool at_end = cur == last_;
    if (!at_end)
      c = *cur;
    return ::ycxx::detail::re_assert(P_, tr_, op, flags_, at_first, has_prev, prev, at_end, c);
  }
  bool accept(const It& cur, std::ptrdiff_t idx) const {
    if (whole_ && cur != last_)
      return false;
    return (flags_ & std::regex_constants::match_not_null) == 0 || idx != start_idx_;
  }
  void decide(int l, int& pc, const It& cur, std::ptrdiff_t idx) {
    const re_loop& lp = P_.loops[static_cast<std::size_t>(l)];
    const std::ptrdiff_t c = ls_[static_cast<std::size_t>(l)].count;
    if (c < lp.min) {
      pc = lp.iter_pc;
    } else if (lp.max >= 0 && c >= lp.max) {
      pc = lp.exit_pc;
    } else if (lp.greedy) {
      push(k_choice, lp.exit_pc, 0, cur, idx);
      pc = lp.iter_pc;
    } else {
      push(k_choice, lp.iter_pc, 0, cur, idx);
      pc = lp.exit_pc;
    }
  }
  // Pops frames down to base, restoring the state they recorded, until one says where to resume.
  bool backtrack(std::size_t base, int& pc, It& cur, std::ptrdiff_t& idx) {
    while (stack_.size() > base) {
      frame& f = stack_.back();
      switch (f.k) {
      case k_slot:
        caps_[static_cast<std::size_t>(f.a)] = slot{f.it, f.flag};
        stack_.pop_back();
        break;
      case k_loop:
        ls_[static_cast<std::size_t>(f.a)] = loop_state{f.n, f.it, f.idx};
        stack_.pop_back();
        break;
      case k_choice:
        pc = f.a;
        cur = f.it;
        idx = f.idx;
        stack_.pop_back();
        return true;
      case k_greedy: { // give back one character
        const re_loop& lp = P_.loops[static_cast<std::size_t>(P_.code[static_cast<std::size_t>(f.a)].a)];
        --f.it;
        --f.idx;
        --f.n;
        pc = lp.exit_pc;
        cur = f.it;
        idx = f.idx;
        if (f.n <= lp.min)
          stack_.pop_back();
        return true;
      }
      case k_lazy: { // take one more character
        const re_loop& lp = P_.loops[static_cast<std::size_t>(P_.code[static_cast<std::size_t>(f.a)].a)];
        if ((lp.max < 0 || f.n < lp.max) && f.it != last_ && P_.single(tr_, f.a + 1, *f.it)) {
          ++f.it;
          ++f.idx;
          ++f.n;
          pc = lp.exit_pc;
          cur = f.it;
          idx = f.idx;
          if (lp.max >= 0 && f.n >= lp.max)
            stack_.pop_back();
          return true;
        }
        stack_.pop_back();
        break;
      }
      }
    }
    return false;
  }
  // Undoes and drops every frame above base (a negative lookahead that matched).
  void unwind(std::size_t base) {
    int pc;
    It cur;
    std::ptrdiff_t idx;
    while (stack_.size() > base) {
      const kind k = stack_.back().k;
      if (k == k_slot || k == k_loop) {
        backtrack(stack_.size() - 1, pc, cur, idx);
      } else {
        stack_.pop_back();
      }
    }
  }
  // A positive lookahead matched: its choice points are discarded (it is atomic), its undo
  // records kept, so that backtracking past it restores the captures it set.
  void keep_undos(std::size_t base) {
    std::size_t w = base;
    for (std::size_t r = base; r < stack_.size(); ++r)
      if (stack_[r].k == k_slot || stack_[r].k == k_loop)
        stack_[w++] = stack_[r];
    stack_.resize(w, frame{k_choice, false, 0, 0, It{}, 0});
  }

  // Runs from pc until the match instruction accepts (stop < 0) or the look_end `stop` is
  // reached; returns false when every alternative failed (the stack is then back to its size at
  // entry). On success *end is the position reached.
  bool run(int pc, It cur, std::ptrdiff_t idx, int stop, It* end) {
    const std::size_t base = stack_.size();
    for (;;) {
      tick(idx);
      if (memo_on_) {
        const int mp = P_.memo_index[static_cast<std::size_t>(pc)];
        if (mp >= 0 && !visit(memo_key(mp, idx), idx))
          goto fail;
      }
      {
        const re_inst<charT>& in = P_.code[static_cast<std::size_t>(pc)];
        switch (in.op) {
        case re_op::chr:
        case re_op::any:
        case re_op::set:
          if (cur == last_ || !P_.single(tr_, pc, *cur))
            goto fail;
          ++cur;
          ++idx;
          ++pc;
          continue;
        case re_op::split:
          push(k_choice, in.b, 0, cur, idx);
          pc = in.a;
          continue;
        case re_op::jmp:
          pc = in.a;
          continue;
        case re_op::open:
          set_slot(2 * in.a, cur, true);
          ++pc;
          continue;
        case re_op::close:
          set_slot(2 * in.a + 1, cur, true);
          ++pc;
          continue;
        case re_op::bol:
        case re_op::eol:
        case re_op::wordb:
        case re_op::nwordb:
          if (!check(in.op, cur))
            goto fail;
          ++pc;
          continue;
        case re_op::backref: {
          const auto g = static_cast<std::size_t>(in.a);
          if (caps_[2 * g + 1].set) {
            It s = caps_[2 * g].it;
            const It e = caps_[2 * g + 1].it;
            for (; s != e; ++s) {
              if (cur == last_ || P_.tx(tr_, *s) != P_.tx(tr_, *cur))
                goto fail;
              ++cur;
              ++idx;
            }
          }
          ++pc;
          continue;
        }
        case re_op::look: {
          const std::size_t b2 = stack_.size();
          const bool ok = run(pc + 1, cur, idx, in.a, nullptr);
          if (ok) {
            if (in.flag) {
              unwind(b2);
              goto fail;
            }
            keep_undos(b2);
          } else if (!in.flag) {
            goto fail;
          }
          pc = in.a + 1;
          continue;
        }
        case re_op::look_end:
          if (pc == stop)
            return true;
          goto fail;
        case re_op::loop_enter:
          save_loop(in.a);
          ls_[static_cast<std::size_t>(in.a)].count = 0;
          decide(in.a, pc, cur, idx);
          continue;
        case re_op::loop_iter: {
          const re_loop& lp = P_.loops[static_cast<std::size_t>(in.a)];
          save_loop(in.a);
          loop_state& s = ls_[static_cast<std::size_t>(in.a)];
          ++s.count;
          s.start = cur;
          s.start_idx = idx;
          // ECMA-262 RepeatMatcher step 4: the captures inside the atom are cleared.
          for (int g = lp.group_lo; g < lp.group_hi; ++g)
            if (caps_[static_cast<std::size_t>(2 * g + 1)].set)
              set_slot(2 * g + 1, It{}, false);
          ++pc;
          continue;
        }
        case re_op::loop_tail: {
          const re_loop& lp = P_.loops[static_cast<std::size_t>(in.a)];
          const loop_state& s = ls_[static_cast<std::size_t>(in.a)];
          // An optional iteration that matched the empty string fails.
          if (s.count > lp.min && idx == s.start_idx)
            goto fail;
          decide(in.a, pc, cur, idx);
          continue;
        }
        case re_op::rep: {
          const re_loop& lp = P_.loops[static_cast<std::size_t>(in.a)];
          std::ptrdiff_t n = 0;
          if (lp.greedy) {
            while ((lp.max < 0 || n < lp.max) && cur != last_ && P_.single(tr_, pc + 1, *cur)) {
              ++cur;
              ++idx;
              ++n;
            }
            if (n < lp.min)
              goto fail;
            if (n > lp.min)
              push(k_greedy, pc, n, cur, idx);
          } else {
            for (; n < lp.min; ++n) {
              if (cur == last_ || !P_.single(tr_, pc + 1, *cur))
                goto fail;
              ++cur;
              ++idx;
            }
            if (lp.max < 0 || n < lp.max)
              push(k_lazy, pc, n, cur, idx);
          }
          if (idx > max_idx_)
            max_idx_ = idx;
          pc = lp.exit_pc;
          continue;
        }
        case re_op::match:
          if (stop < 0 && accept(cur, idx)) {
            if (!longest_) {
              *end = cur;
              return true;
            }
            if (idx - start_idx_ > best_len_) {
              best_len_ = idx - start_idx_;
              best_end_ = cur;
              best_caps_ = caps_;
            }
          }
          goto fail;
        }
      }
    fail:
      if (!backtrack(base, pc, cur, idx))
        return false;
    }
  }

public:
  re_backtracker(const prog& P, const traits& tr, It first, It last, flag_t f)
      : P_(P), tr_(tr), first_(first), last_(last), flags_(f) {}

  // Finds the first match (ECMAScript) or the leftmost-longest one (POSIX with
  // back-references) in [first, last); whole: it must span [first, last).
  bool search(bool whole, std::vector<re_cap<It>>& out) {
    whole_ = whole;
    longest_ = P_.posix;
    memo_on_ = P_.memo && !longest_;
    memo_wpp_ = ((static_cast<std::size_t>(P_.memo_points) << P_.memo_loops.size()) + 63) / 64;
    const bool continuous = whole || (flags_ & std::regex_constants::match_continuous) != 0;
    ls_.assign(P_.loops.size(), loop_state{});
    It start = first_;
    for (start_idx_ = 0;; ++start_idx_) {
      caps_.assign(2 * static_cast<std::size_t>(P_.groups) + 2, slot{});
      It end{};
      bool ok;
      if (longest_) {
        best_len_ = -1;
        run(0, start, start_idx_, -1, nullptr);
        ok = best_len_ >= 0;
        if (ok) {
          end = best_end_;
          caps_ = best_caps_;
        }
      } else {
        ok = run(0, start, start_idx_, -1, &end);
      }
      if (ok) {
        stack_.clear();
        out.assign(static_cast<std::size_t>(P_.groups) + 1, re_cap<It>{last_, last_, false});
        out[0] = re_cap<It>{start, end, true};
        for (std::size_t g = 1; g <= static_cast<std::size_t>(P_.groups); ++g)
          if (caps_[2 * g + 1].set)
            out[g] = re_cap<It>{caps_[2 * g].it, caps_[2 * g + 1].it, true};
        return true;
      }
      if (continuous || start == last_)
        return false;
      ++start;
    }
  }
};

// ---- POSIX: the leftmost-longest match --------------------------------------------------------
template <class It, class charT, class traits>
class re_nfa {
  using prog = re_program<charT, traits>;
  using flag_t = std::regex_constants::match_flag_type;
  struct pos_ctx {
    bool at_first = false, has_prev = false, at_end = false;
    charT prev{}, cur{};
  };
  struct threads {
    re_pc_set pcs;
    std::vector<std::ptrdiff_t> sidx; // per pc: the start of the thread that holds it
    std::vector<It> sit;
    void init(std::size_t n) {
      pcs.init(n);
      sidx.assign(n, 0);
      sit.assign(n, It{});
    }
  };

  const prog& P_;
  const traits& tr_;
  It first_, last_;
  flag_t flags_;
  std::vector<int> stk_;

  // Adds pc0 and everything reachable from it by epsilon moves at position c.
  void closure(threads& L, int pc0, std::ptrdiff_t si, const It& sit, const pos_ctx& c) {
    stk_.clear();
    stk_.push_back(pc0);
    while (!stk_.empty()) {
      const int pc = stk_.back();
      stk_.pop_back();
      if (L.pcs.contains(pc))
        continue;
      L.pcs.insert(pc);
      L.sidx[static_cast<std::size_t>(pc)] = si;
      L.sit[static_cast<std::size_t>(pc)] = sit;
      const re_inst<charT>& in = P_.code[static_cast<std::size_t>(pc)];
      switch (in.op) {
      case re_op::split:
        stk_.push_back(in.b);
        stk_.push_back(in.a);
        break;
      case re_op::jmp:
        stk_.push_back(in.a);
        break;
      case re_op::open:
      case re_op::close:
        stk_.push_back(pc + 1);
        break;
      case re_op::bol:
      case re_op::eol:
      case re_op::wordb:
      case re_op::nwordb:
        if (::ycxx::detail::re_assert(P_, tr_, in.op, flags_, c.at_first, c.has_prev, c.prev, c.at_end, c.cur))
          stk_.push_back(pc + 1);
        break;
      default:
        break;
      }
    }
  }

public:
  re_nfa(const prog& P, const traits& tr, It first, It last, flag_t f) : P_(P), tr_(tr), first_(first), last_(last), flags_(f) {}

  // The leftmost-longest match: [bs, be) as offsets from first and as iterators.
  bool find(bool whole, std::ptrdiff_t& bs, It& bsit, std::ptrdiff_t& be, It& beit) {
    namespace rc = std::regex_constants;
    const bool continuous = whole || (flags_ & rc::match_continuous) != 0;
    const bool not_null = (flags_ & rc::match_not_null) != 0;
    threads cl, nl;
    cl.init(P_.code.size());
    nl.init(P_.code.size());
    It cur = first_;
    std::ptrdiff_t i = 0;
    pos_ctx c;
    c.at_first = true;
    c.has_prev = (flags_ & rc::match_prev_avail) != 0;
    if (c.has_prev) {
      It q = first_;
      --q;
      c.prev = *q;
    }
    c.at_end = cur == last_;
    if (!c.at_end)
      c.cur = *cur;
    bool found = false;
    for (;;) {
      if (!found && (i == 0 || !continuous))
        closure(cl, 0, i, cur, c); // the new thread has the lowest priority
      for (std::size_t k = 0; k < cl.pcs.n; ++k) {
        const int pc = cl.pcs.dense[k];
        if (P_.code[static_cast<std::size_t>(pc)].op != re_op::match)
          continue;
        const std::ptrdiff_t si = cl.sidx[static_cast<std::size_t>(pc)];
        if ((whole && !c.at_end) || (not_null && si == i))
          continue;
        if (!found || si < bs || (si == bs && i > be)) {
          found = true;
          bs = si;
          bsit = cl.sit[static_cast<std::size_t>(pc)];
          be = i;
          beit = cur;
        }
      }
      if (c.at_end || (cl.pcs.n == 0 && (found || continuous)))
        break;
      nl.pcs.clear();
      const charT ch = c.cur;
      It nx = cur;
      ++nx;
      pos_ctx nc;
      nc.has_prev = true;
      nc.prev = ch;
      nc.at_end = nx == last_;
      if (!nc.at_end)
        nc.cur = *nx;
      for (std::size_t k = 0; k < cl.pcs.n; ++k) {
        const int pc = cl.pcs.dense[k];
        const re_op op = P_.code[static_cast<std::size_t>(pc)].op;
        if (op != re_op::chr && op != re_op::any && op != re_op::set)
          continue;
        const std::ptrdiff_t si = cl.sidx[static_cast<std::size_t>(pc)];
        if (found && si > bs)
          continue;
        if (P_.single(tr_, pc, ch))
          closure(nl, pc + 1, si, cl.sit[static_cast<std::size_t>(pc)], nc);
      }
      using std::swap;
      swap(cl, nl);
      cur = nx;
      ++i;
      c = nc;
    }
    return found;
  }
};

// ---- POSIX: the subexpressions of a match -----------------------------------------------------
template <class It, class charT, class traits>
class re_posix_sub {
  using prog = re_program<charT, traits>;
  using flag_t = std::regex_constants::match_flag_type;
  using node = re_node<charT>;

  const prog& P_;
  const traits& tr_;
  flag_t flags_;
  std::vector<It> its_;     // the positions of the match, [0, L]
  std::vector<charT> text_; // its characters
  std::ptrdiff_t L_ = 0;
  bool at_first0_ = false, has_prev0_ = false, end_is_last_ = false;
  charT prev0_{}, after_{};
  std::vector<std::ptrdiff_t> caps_; // per group: start, end (relative); -1 unset
  re_pc_set a_, b_;
  std::vector<int> stk_;

  bool assert_at(re_op op, std::ptrdiff_t p) const {
    const bool has_prev = p > 0 || has_prev0_;
    const charT prev = p > 0 ? text_[static_cast<std::size_t>(p - 1)] : prev0_;
    const bool at_end = p == L_ && end_is_last_;
    const charT cur = p < L_ ? text_[static_cast<std::size_t>(p)] : after_;
    return ::ycxx::detail::re_assert(P_, tr_, op, flags_, p == 0 && at_first0_, has_prev, prev, at_end, cur);
  }
  static bool consuming(re_op op) { return op == re_op::chr || op == re_op::any || op == re_op::set; }

  // Forward: the closure of pc0 at position p inside [B, E); reaching E adds p to ends (in
  // increasing order, once).
  void fclosure(re_pc_set& S, int pc0, std::ptrdiff_t p, int B, int E, std::vector<std::ptrdiff_t>& ends) {
    stk_.clear();
    stk_.push_back(pc0);
    while (!stk_.empty()) {
      const int pc = stk_.back();
      stk_.pop_back();
      if (pc == E) {
        if (ends.empty() || ends.back() != p)
          ends.push_back(p);
        continue;
      }
      if (pc < B || pc > E || S.contains(pc))
        continue;
      S.insert(pc);
      const re_inst<charT>& in = P_.code[static_cast<std::size_t>(pc)];
      switch (in.op) {
      case re_op::split:
        stk_.push_back(in.b);
        stk_.push_back(in.a);
        break;
      case re_op::jmp:
        stk_.push_back(in.a);
        break;
      case re_op::open:
      case re_op::close:
        stk_.push_back(pc + 1);
        break;
      case re_op::bol:
      case re_op::eol:
      case re_op::wordb:
      case re_op::nwordb:
        if (assert_at(in.op, p))
          stk_.push_back(pc + 1);
        break;
      default:
        break;
      }
    }
  }
  // The positions e in [a, lim] such that node n matches [a, e), in increasing order. The work is
  // proportional to how far the node can match, not to lim - a.
  std::vector<std::ptrdiff_t> ends(int n, std::ptrdiff_t a, std::ptrdiff_t lim) {
    const int B = P_.node_begin[static_cast<std::size_t>(n)], E = P_.node_end[static_cast<std::size_t>(n)];
    std::vector<std::ptrdiff_t> res;
    re_pc_set* cl = &a_;
    re_pc_set* nl = &b_;
    cl->clear();
    fclosure(*cl, B, a, B, E, res);
    for (std::ptrdiff_t p = a; p < lim && cl->n != 0; ++p) {
      nl->clear();
      const charT ch = text_[static_cast<std::size_t>(p)];
      for (std::size_t k = 0; k < cl->n; ++k) {
        const int pc = cl->dense[k];
        if (consuming(P_.code[static_cast<std::size_t>(pc)].op) && P_.single(tr_, pc, ch))
          fclosure(*nl, pc + 1, p + 1, B, E, res);
      }
      re_pc_set* t = cl;
      cl = nl;
      nl = t;
    }
    return res;
  }
  // Backward: the closure of q0 at position p inside [B, E) over epsilon predecessors.
  void bclosure(re_pc_set& S, int q0, std::ptrdiff_t p, int B, int E) {
    stk_.clear();
    stk_.push_back(q0);
    while (!stk_.empty()) {
      const int q = stk_.back();
      stk_.pop_back();
      if (S.contains(q))
        continue;
      S.insert(q);
      for (int e : P_.eps_pred[static_cast<std::size_t>(q)]) {
        if (e < B || e >= E)
          continue;
        const re_op op = P_.code[static_cast<std::size_t>(e)].op;
        if ((op == re_op::bol || op == re_op::eol || op == re_op::wordb || op == re_op::nwordb) && !assert_at(op, p))
          continue;
        stk_.push_back(e);
      }
    }
  }
  // For each pc of `at`: the positions p in [s, t] such that (pc, p) reaches (end of n, t).
  std::vector<std::vector<char>> reach(int n, std::ptrdiff_t s, std::ptrdiff_t t, const std::vector<int>& at) {
    const int B = P_.node_begin[static_cast<std::size_t>(n)], E = P_.node_end[static_cast<std::size_t>(n)];
    std::vector<std::vector<char>> res(at.size(), std::vector<char>(static_cast<std::size_t>(t - s + 1), 0));
    re_pc_set* cl = &a_;
    re_pc_set* nl = &b_;
    cl->clear();
    bclosure(*cl, E, t, B, E);
    for (std::ptrdiff_t p = t;; --p) {
      for (std::size_t k = 0; k < at.size(); ++k)
        res[k][static_cast<std::size_t>(p - s)] = cl->contains(at[k]) ? 1 : 0;
      if (p == s || cl->n == 0)
        break;
      nl->clear();
      const charT ch = text_[static_cast<std::size_t>(p - 1)];
      for (std::size_t k = 0; k < cl->n; ++k) {
        const int c = cl->dense[k] - 1;
        if (c >= B && c < E && consuming(P_.code[static_cast<std::size_t>(c)].op) && P_.single(tr_, c, ch))
          bclosure(*nl, c, p - 1, B, E);
      }
      re_pc_set* tmp = cl;
      cl = nl;
      nl = tmp;
    }
    return res;
  }
  // The largest end e >= lo in E from which the rest can still finish (R[e - s]), or -1.
  static std::ptrdiff_t longest(const std::vector<std::ptrdiff_t>& E, const std::vector<char>& R, std::ptrdiff_t s,
                                std::ptrdiff_t lo) {
    for (std::size_t k = E.size(); k-- > 0;)
      if (E[k] >= lo && R[static_cast<std::size_t>(E[k] - s)])
        return E[k];
    return -1;
  }
  void reset_groups(const node& x) {
    for (int g = x.group_lo; g < x.group_hi; ++g) {
      caps_[2 * static_cast<std::size_t>(g)] = -1;
      caps_[2 * static_cast<std::size_t>(g) + 1] = -1;
    }
  }
  // Assigns the subexpressions of node n, which matches [s, t).
  void span(int n, std::ptrdiff_t s, std::ptrdiff_t t) {
    const node& x = P_.nodes[static_cast<std::size_t>(n)];
    switch (x.kind) {
    case re_kind::group:
      caps_[2 * static_cast<std::size_t>(x.val)] = s;
      caps_[2 * static_cast<std::size_t>(x.val) + 1] = t;
      span(x.kids[0], s, t);
      return;
    case re_kind::alt:
      for (int k : x.kids) {
        const auto E = ends(k, s, t);
        if (!E.empty() && E.back() == t) {
          span(k, s, t);
          return;
        }
      }
      return;
    case re_kind::concat: {
      std::vector<int> at;
      for (std::size_t i = 1; i < x.kids.size(); ++i)
        at.push_back(P_.node_begin[static_cast<std::size_t>(x.kids[i])]);
      const auto R = reach(n, s, t, at);
      std::ptrdiff_t cur = s;
      for (std::size_t i = 0; i + 1 < x.kids.size(); ++i) {
        const std::ptrdiff_t b = longest(ends(x.kids[i], cur, t), R[i], s, cur);
        if (b < 0)
          return; // cannot happen when [s, t) is a match of n
        span(x.kids[i], cur, b);
        cur = b;
      }
      span(x.kids.back(), cur, t);
      return;
    }
    case re_kind::repeat: {
      const int body = x.kids[0];
      if (s == t) { // one empty iteration if the body can match it and no iteration preceded
        if (!x.tail && !ends(body, s, s).empty()) {
          reset_groups(x);
          span(body, s, s);
        }
        return;
      }
      if (x.max == 1) {
        reset_groups(x);
        span(body, s, t);
        return;
      }
      const std::vector<int> at{P_.node_begin[static_cast<std::size_t>(n)]};
      const auto R = reach(n, s, t, at);
      for (std::ptrdiff_t cur = s; cur < t;) {
        const std::ptrdiff_t b = longest(ends(body, cur, t), R[0], s, cur + 1);
        if (b < 0)
          return; // cannot happen when [s, t) is a match of n
        reset_groups(x);
        span(body, cur, b);
        cur = b;
      }
      return;
    }
    default:
      return;
    }
  }

public:
  re_posix_sub(const prog& P, const traits& tr, flag_t f) : P_(P), tr_(tr), flags_(f) {}

  // [bsit, bsit + len) is the match found in [first, last); fills out[1...].
  void resolve(It first, It last, It bsit, std::ptrdiff_t len, std::vector<re_cap<It>>& out) {
    L_ = len;
    its_.reserve(static_cast<std::size_t>(len + 1));
    text_.reserve(static_cast<std::size_t>(len));
    It it = bsit;
    for (std::ptrdiff_t i = 0; i < len; ++i) {
      its_.push_back(it);
      text_.push_back(*it);
      ++it;
    }
    its_.push_back(it);
    at_first0_ = bsit == first;
    has_prev0_ = !at_first0_ || (flags_ & std::regex_constants::match_prev_avail) != 0;
    if (has_prev0_) {
      It q = bsit;
      --q;
      prev0_ = *q;
    }
    end_is_last_ = it == last;
    if (!end_is_last_)
      after_ = *it;
    a_.init(P_.code.size() + 1);
    b_.init(P_.code.size() + 1);
    caps_.assign(2 * static_cast<std::size_t>(P_.groups) + 2, -1);
    span(P_.root, 0, len);
    for (std::size_t g = 1; g <= static_cast<std::size_t>(P_.groups); ++g)
      if (caps_[2 * g] >= 0)
        out[g] = re_cap<It>{its_[static_cast<std::size_t>(caps_[2 * g])], its_[static_cast<std::size_t>(caps_[2 * g + 1])], true};
  }
};

// Runs program P on [first, last): out receives 1 + groups entries.
template <class It, class charT, class traits>
bool re_execute(const re_program<charT, traits>& P, const traits& tr, It first, It last,
                std::regex_constants::match_flag_type f, bool whole, std::vector<re_cap<It>>& out) {
  if (!P.nfa) {
    re_backtracker<It, charT, traits> bt(P, tr, first, last, f);
    return bt.search(whole, out);
  }
  std::ptrdiff_t bs = 0, be = 0;
  It bsit{}, beit{};
  re_nfa<It, charT, traits> sim(P, tr, first, last, f);
  if (!sim.find(whole, bs, bsit, be, beit))
    return false;
  out.assign(static_cast<std::size_t>(P.groups) + 1, re_cap<It>{last, last, false});
  out[0] = re_cap<It>{bsit, beit, true};
  if (P.groups > 0 && (P.flags & std::regex_constants::nosubs) == 0) {
    re_posix_sub<It, charT, traits> sub(P, tr, f);
    sub.resolve(first, last, bsit, be - bs, out);
  }
  return true;
}

}} // namespace ycxx::detail
