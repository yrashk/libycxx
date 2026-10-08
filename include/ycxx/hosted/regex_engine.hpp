// libycxx hosted: the regular expression matchers.
//
// - re_backtracker runs ECMAScript programs (first match in ECMA-262's priority order) and POSIX
//   programs with back-references (every path is explored and the longest match kept; then
//   re_posix_bt_sub assigns the subexpressions by the POSIX rule, DECISIONS §3). It
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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _It>
struct __re_cap {
  _It first{}, second{};
  bool matched = false;
};

// The zero-width assertions ([re.matchflag], [re.grammar]).
template <class __charT, class __traits>
bool __re_assert(const __re_program<__charT, __traits>& _Pp, const __traits& __tr, __re_op op, std::regex_constants::match_flag_type __f,
               bool __at_first, bool __has_prev, __charT prev, bool __at_end, __charT cur) {
  namespace __rc = std::regex_constants;
  const bool __prev_avail = (__f & __rc::match_prev_avail) != 0;
  switch (op) {
  case __re_op::__bol:
    if (__at_first && !__prev_avail)
      return (__f & __rc::match_not_bol) == 0;
    return __has_prev && _Pp.multiline && ::__ycxx::__detail::__re_line_terminator(prev);
  case __re_op::__eol:
    if (__at_end)
      return (__f & __rc::match_not_eol) == 0;
    return _Pp.multiline && ::__ycxx::__detail::__re_line_terminator(cur);
  default: {
    bool b;
    if (__at_first && !__prev_avail && (__f & __rc::match_not_bow) != 0)
      b = false;
    else if (__at_end && (__f & __rc::match_not_eow) != 0)
      b = false;
    else
      b = (__has_prev && _Pp.__is_word(__tr, prev)) != (!__at_end && _Pp.__is_word(__tr, cur));
    return op == __re_op::__wordb ? b : !b;
  }
  }
}

// A set of pcs with constant-time insert, test and clear (Briggs and Torczon).
struct __re_pc_set {
  std::vector<int> __dense, __sparse;
  std::size_t n = 0;
  void init(std::size_t size) {
    __dense.assign(size, 0);
    __sparse.assign(size, 0);
    n = 0;
  }
  bool contains(int __pc) const {
    const auto s = static_cast<std::size_t>(__sparse[static_cast<std::size_t>(__pc)]);
    return s < n && __dense[s] == __pc;
  }
  void insert(int __pc) {
    __sparse[static_cast<std::size_t>(__pc)] = static_cast<int>(n);
    __dense[n++] = __pc;
  }
  void clear() { n = 0; }
};

[[noreturn]] [[__gnu__::__cold__]] inline void __re_throw_complexity() {
  ::__ycxx::__detail::__throw_regex_error(std::regex_constants::error_complexity);
}
[[noreturn]] [[__gnu__::__cold__]] inline void __re_throw_stack() {
  ::__ycxx::__detail::__throw_regex_error(std::regex_constants::error_stack);
}

// ---- backtracking ------------------------------------------------------------------------------
template <class _It, class __charT, class __traits>
class __re_backtracker {
  using __prog = __re_program<__charT, __traits>;
  using __flag_t = std::regex_constants::match_flag_type;

  struct __slot {
    _It __it{};
    bool set = false;
  };
  struct __loop_state {
    std::ptrdiff_t count = 0;
    _It start{};
    std::ptrdiff_t __start_idx = -1;
  };
  enum kind : unsigned char { __k_choice, __k_slot, __k_loop, __k_greedy, __k_lazy };
  struct __frame {
    kind k;
    bool __flag;
    int a;
    std::ptrdiff_t n;
    _It __it;
    std::ptrdiff_t __idx;
  };

  static constexpr std::size_t __max_frames = std::size_t(1) << 22;
  static constexpr std::size_t __max_memo_words = std::size_t(1) << 22;

  const __prog& _P_;
  const __traits& __tr_;
  _It __first_, __last_;
  __flag_t __flags_;
  bool __whole_ = false, __longest_ = false, __memo_on_ = false;
  std::ptrdiff_t __start_idx_ = 0, __max_idx_ = 0;
  std::vector<__slot> __caps_;
  std::vector<__loop_state> __ls_;
  std::vector<__frame> __stack_;
  std::vector<unsigned long long> __memo_;
  std::size_t __memo_wpp_ = 0;
  std::size_t __steps_ = 0, __next_check_ = 0;
  // longest_: the best match so far
  std::ptrdiff_t __best_len_ = -1;
  _It __best_end_{};
  std::vector<__slot> __best_caps_;

  void push(kind k, int a, std::ptrdiff_t n, const _It& __it, std::ptrdiff_t __idx, bool __flag = false) {
    if (__stack_.size() >= __max_frames)
      ::__ycxx::__detail::__re_throw_stack();
    __stack_.push_back(__frame{k, __flag, a, n, __it, __idx});
  }
  void __set_slot(int i, const _It& __it, bool set) {
    __slot& s = __caps_[static_cast<std::size_t>(i)];
    push(__k_slot, i, 0, s.__it, 0, s.set);
    s.__it = __it;
    s.set = set;
  }
  void __save_loop(int __l) {
    const __loop_state& s = __ls_[static_cast<std::size_t>(__l)];
    push(__k_loop, __l, s.count, s.start, s.__start_idx);
  }
  // Counts steps; past a limit that grows with the input reached, the match is too complex.
  void __tick(std::ptrdiff_t __idx) {
    if (__idx > __max_idx_)
      __max_idx_ = __idx;
    if (++__steps_ < __next_check_)
      return;
    __next_check_ = __steps_ + (std::size_t(1) << 16);
    const std::size_t __limit = std::size_t(20000000) + std::size_t(32) * static_cast<std::size_t>(__max_idx_ + 1) * _P_.code.size();
    if (__steps_ > __limit)
      ::__ycxx::__detail::__re_throw_complexity();
  }
  // The memo point mp, refined by whether each enclosing nullable-body loop began its current
  // iteration at idx (the only state, besides pc and idx, that the rest of the match depends on).
  int __memo_key(int __mp, std::ptrdiff_t __idx) const {
    const unsigned mask = _P_.__memo_mask[static_cast<std::size_t>(__mp)];
    unsigned __v = 0;
    for (std::size_t k = 0; k < _P_.__memo_loops.size(); ++k)
      if ((mask >> k) & 1u)
        if (__ls_[static_cast<std::size_t>(_P_.__memo_loops[k])].__start_idx == __idx)
          __v |= 1u << k;
    return __mp << _P_.__memo_loops.size() | static_cast<int>(__v);
  }
  // false if (memo key mp, idx) was visited before; marks it.
  bool visit(int __mp, std::ptrdiff_t __idx) {
    const std::size_t __w = static_cast<std::size_t>(__idx) * __memo_wpp_ + static_cast<std::size_t>(__mp) / 64;
    if (__w >= __memo_.size()) {
      if (__w >= __max_memo_words)
        return true;
      std::size_t n = __memo_.size() * 2;
      if (n <= __w)
        n = __w + 1 + __memo_wpp_ * 64;
      __memo_.resize(n < __max_memo_words ? n : __max_memo_words, 0);
    }
    const unsigned long long b = 1ull << (static_cast<unsigned>(__mp) % 64);
    if (__memo_[__w] & b)
      return false;
    __memo_[__w] |= b;
    return true;
  }
  bool __check(__re_op op, const _It& cur) const {
    const bool __at_first = cur == __first_;
    const bool __has_prev = !__at_first || (__flags_ & std::regex_constants::match_prev_avail) != 0;
    __charT prev{}, c{};
    if (__has_prev) {
      _It __q = cur;
      --__q;
      prev = *__q;
    }
    const bool __at_end = cur == __last_;
    if (!__at_end)
      c = *cur;
    return ::__ycxx::__detail::__re_assert(_P_, __tr_, op, __flags_, __at_first, __has_prev, prev, __at_end, c);
  }
  bool __accept(const _It& cur, std::ptrdiff_t __idx) const {
    if (__whole_ && cur != __last_)
      return false;
    return (__flags_ & std::regex_constants::match_not_null) == 0 || __idx != __start_idx_;
  }
  void __decide(int __l, int& __pc, const _It& cur, std::ptrdiff_t __idx) {
    const __re_loop& __lp = _P_.__loops[static_cast<std::size_t>(__l)];
    const std::ptrdiff_t c = __ls_[static_cast<std::size_t>(__l)].count;
    if (c < __lp.min) {
      __pc = __lp.__iter_pc;
    } else if (__lp.max >= 0 && c >= __lp.max) {
      __pc = __lp.__exit_pc;
    } else if (__lp.__greedy) {
      push(__k_choice, __lp.__exit_pc, 0, cur, __idx);
      __pc = __lp.__iter_pc;
    } else {
      push(__k_choice, __lp.__iter_pc, 0, cur, __idx);
      __pc = __lp.__exit_pc;
    }
  }
  // Pops frames down to base, restoring the state they recorded, until one says where to resume.
  bool __backtrack(std::size_t base, int& __pc, _It& cur, std::ptrdiff_t& __idx) {
    while (__stack_.size() > base) {
      __frame& __f = __stack_.back();
      switch (__f.k) {
      case __k_slot:
        __caps_[static_cast<std::size_t>(__f.a)] = __slot{__f.__it, __f.__flag};
        __stack_.pop_back();
        break;
      case __k_loop:
        __ls_[static_cast<std::size_t>(__f.a)] = __loop_state{__f.n, __f.__it, __f.__idx};
        __stack_.pop_back();
        break;
      case __k_choice:
        __pc = __f.a;
        cur = __f.__it;
        __idx = __f.__idx;
        __stack_.pop_back();
        return true;
      case __k_greedy: { // give back one character
        const __re_loop& __lp = _P_.__loops[static_cast<std::size_t>(_P_.code[static_cast<std::size_t>(__f.a)].a)];
        --__f.__it;
        --__f.__idx;
        --__f.n;
        __pc = __lp.__exit_pc;
        cur = __f.__it;
        __idx = __f.__idx;
        if (__f.n <= __lp.min)
          __stack_.pop_back();
        return true;
      }
      case __k_lazy: { // take one more character
        const __re_loop& __lp = _P_.__loops[static_cast<std::size_t>(_P_.code[static_cast<std::size_t>(__f.a)].a)];
        if ((__lp.max < 0 || __f.n < __lp.max) && __f.__it != __last_ && _P_.single(__tr_, __f.a + 1, *__f.__it)) {
          ++__f.__it;
          ++__f.__idx;
          ++__f.n;
          __pc = __lp.__exit_pc;
          cur = __f.__it;
          __idx = __f.__idx;
          if (__lp.max >= 0 && __f.n >= __lp.max)
            __stack_.pop_back();
          return true;
        }
        __stack_.pop_back();
        break;
      }
      }
    }
    return false;
  }
  // Undoes and drops every frame above base (a negative lookahead that matched).
  void __unwind(std::size_t base) {
    int __pc;
    _It cur;
    std::ptrdiff_t __idx;
    while (__stack_.size() > base) {
      const kind k = __stack_.back().k;
      if (k == __k_slot || k == __k_loop) {
        __backtrack(__stack_.size() - 1, __pc, cur, __idx);
      } else {
        __stack_.pop_back();
      }
    }
  }
  // A positive lookahead matched: its choice points are discarded (it is atomic), its undo
  // records kept, so that backtracking past it restores the captures it set.
  void __keep_undos(std::size_t base) {
    std::size_t __w = base;
    for (std::size_t r = base; r < __stack_.size(); ++r)
      if (__stack_[r].k == __k_slot || __stack_[r].k == __k_loop)
        __stack_[__w++] = __stack_[r];
    __stack_.resize(__w, __frame{__k_choice, false, 0, 0, _It{}, 0});
  }

  // Runs from pc until the match instruction accepts (stop < 0) or the look_end `__stop` is
  // reached; returns false when every alternative failed (the stack is then back to its size at
  // entry). On success *end is the position reached.
  bool run(int __pc, _It cur, std::ptrdiff_t __idx, int __stop, _It* end) {
    const std::size_t base = __stack_.size();
    for (;;) {
      __tick(__idx);
      if (__memo_on_) {
        const int __mp = _P_.__memo_index[static_cast<std::size_t>(__pc)];
        if (__mp >= 0 && !visit(__memo_key(__mp, __idx), __idx))
          goto fail;
      }
      {
        const __re_inst<__charT>& in = _P_.code[static_cast<std::size_t>(__pc)];
        switch (in.op) {
        case __re_op::__chr:
        case __re_op::any:
        case __re_op::set:
          if (cur == __last_ || !_P_.single(__tr_, __pc, *cur))
            goto fail;
          ++cur;
          ++__idx;
          ++__pc;
          continue;
        case __re_op::split:
          push(__k_choice, in.b, 0, cur, __idx);
          __pc = in.a;
          continue;
        case __re_op::__jmp:
          __pc = in.a;
          continue;
        case __re_op::open:
          __set_slot(2 * in.a, cur, true);
          ++__pc;
          continue;
        case __re_op::close:
          __set_slot(2 * in.a + 1, cur, true);
          ++__pc;
          continue;
        case __re_op::__bol:
        case __re_op::__eol:
        case __re_op::__wordb:
        case __re_op::__nwordb:
          if (!__check(in.op, cur))
            goto fail;
          ++__pc;
          continue;
        case __re_op::__backref: {
          const auto __g = static_cast<std::size_t>(in.a);
          // POSIX (XBD 9.3.6): a subexpression that did not participate has no string to match
          // the same as; ECMA-262: such a back-reference matches the empty string.
          if (_P_.posix && !__caps_[2 * __g + 1].set)
            goto fail;
          if (__caps_[2 * __g + 1].set) {
            _It s = __caps_[2 * __g].__it;
            const _It e = __caps_[2 * __g + 1].__it;
            for (; s != e; ++s) {
              if (cur == __last_ || _P_.__tx(__tr_, *s) != _P_.__tx(__tr_, *cur))
                goto fail;
              ++cur;
              ++__idx;
            }
          }
          ++__pc;
          continue;
        }
        case __re_op::__look: {
          const std::size_t __b2 = __stack_.size();
          const bool ok = run(__pc + 1, cur, __idx, in.a, nullptr);
          if (ok) {
            if (in.__flag) {
              __unwind(__b2);
              goto fail;
            }
            __keep_undos(__b2);
          } else if (!in.__flag) {
            goto fail;
          }
          __pc = in.a + 1;
          continue;
        }
        case __re_op::__look_end:
          if (__pc == __stop)
            return true;
          goto fail;
        case __re_op::__loop_enter:
          __save_loop(in.a);
          __ls_[static_cast<std::size_t>(in.a)].count = 0;
          __decide(in.a, __pc, cur, __idx);
          continue;
        case __re_op::__loop_iter: {
          const __re_loop& __lp = _P_.__loops[static_cast<std::size_t>(in.a)];
          __save_loop(in.a);
          __loop_state& s = __ls_[static_cast<std::size_t>(in.a)];
          ++s.count;
          s.start = cur;
          s.__start_idx = __idx;
          // ECMA-262 RepeatMatcher step 4: the captures inside the atom are cleared.
          for (int __g = __lp.__group_lo; __g < __lp.__group_hi; ++__g)
            if (__caps_[static_cast<std::size_t>(2 * __g + 1)].set)
              __set_slot(2 * __g + 1, _It{}, false);
          ++__pc;
          continue;
        }
        case __re_op::__loop_tail: {
          const __re_loop& __lp = _P_.__loops[static_cast<std::size_t>(in.a)];
          const __loop_state& s = __ls_[static_cast<std::size_t>(in.a)];
          // An optional iteration that matched the empty string fails; for POSIX with
          // back-references, the first iteration may (XBD 9.3.6: it is the only match of the
          // repetition, and its subexpressions then hold the empty string a back-reference can
          // refer to).
          if (s.count > __lp.min && __idx == s.__start_idx && !(s.count == 1 && _P_.posix && _P_.__has_backref))
            goto fail;
          __decide(in.a, __pc, cur, __idx);
          continue;
        }
        case __re_op::rep: {
          const __re_loop& __lp = _P_.__loops[static_cast<std::size_t>(in.a)];
          std::ptrdiff_t n = 0;
          if (__lp.__greedy) {
            while ((__lp.max < 0 || n < __lp.max) && cur != __last_ && _P_.single(__tr_, __pc + 1, *cur)) {
              ++cur;
              ++__idx;
              ++n;
            }
            if (n < __lp.min)
              goto fail;
            if (n > __lp.min)
              push(__k_greedy, __pc, n, cur, __idx);
          } else {
            for (; n < __lp.min; ++n) {
              if (cur == __last_ || !_P_.single(__tr_, __pc + 1, *cur))
                goto fail;
              ++cur;
              ++__idx;
            }
            if (__lp.max < 0 || n < __lp.max)
              push(__k_lazy, __pc, n, cur, __idx);
          }
          if (__idx > __max_idx_)
            __max_idx_ = __idx;
          __pc = __lp.__exit_pc;
          continue;
        }
        case __re_op::__match:
          if (__stop < 0 && __accept(cur, __idx)) {
            if (!__longest_) {
              *end = cur;
              return true;
            }
            if (__idx - __start_idx_ > __best_len_) {
              __best_len_ = __idx - __start_idx_;
              __best_end_ = cur;
              __best_caps_ = __caps_;
            }
            if (cur == __last_ && __stop < 0 && base == 0) { // nothing can be longer
              __unwind(base);
              return false;
            }
          }
          goto fail;
        }
      }
    fail:
      if (!__backtrack(base, __pc, cur, __idx))
        return false;
    }
  }

public:
  __re_backtracker(const __prog& _Pp, const __traits& __tr, _It first, _It last, __flag_t __f)
      : _P_(_Pp), __tr_(__tr), __first_(first), __last_(last), __flags_(__f) {}

  // Finds the first match (ECMAScript) or the leftmost-longest one (POSIX with
  // back-references) in [first, last); whole: it must span [first, last).
  bool search(bool __whole, std::vector<__re_cap<_It>>& out) {
    __whole_ = __whole;
    __longest_ = _P_.posix;
    __memo_on_ = _P_.__memo && !__longest_;
    __memo_wpp_ = ((static_cast<std::size_t>(_P_.__memo_points) << _P_.__memo_loops.size()) + 63) / 64;
    const bool __continuous = __whole || (__flags_ & std::regex_constants::match_continuous) != 0;
    __ls_.assign(_P_.__loops.size(), __loop_state{});
    _It start = __first_;
    for (__start_idx_ = 0;; ++__start_idx_) {
      __caps_.assign(2 * static_cast<std::size_t>(_P_.__groups) + 2, __slot{});
      _It end{};
      bool ok;
      if (__longest_) {
        __best_len_ = -1;
        run(0, start, __start_idx_, -1, nullptr);
        ok = __best_len_ >= 0;
        if (ok) {
          end = __best_end_;
          __caps_ = __best_caps_;
        }
      } else {
        ok = run(0, start, __start_idx_, -1, &end);
      }
      if (ok) {
        __stack_.clear();
        out.assign(static_cast<std::size_t>(_P_.__groups) + 1, __re_cap<_It>{__last_, __last_, false});
        out[0] = __re_cap<_It>{start, end, true};
        for (std::size_t __g = 1; __g <= static_cast<std::size_t>(_P_.__groups); ++__g)
          if (__caps_[2 * __g + 1].set)
            out[__g] = __re_cap<_It>{__caps_[2 * __g].__it, __caps_[2 * __g + 1].__it, true};
        return true;
      }
      if (__continuous || start == __last_)
        return false;
      ++start;
    }
  }
};

// ---- POSIX: the leftmost-longest match --------------------------------------------------------
template <class _It, class __charT, class __traits>
class __re_nfa {
  using __prog = __re_program<__charT, __traits>;
  using __flag_t = std::regex_constants::match_flag_type;
  struct __pos_ctx {
    bool __at_first = false, __has_prev = false, __at_end = false;
    __charT prev{}, cur{};
  };
  struct __threads {
    __re_pc_set __pcs;
    std::vector<std::ptrdiff_t> __sidx; // per pc: the start of the thread that holds it
    std::vector<_It> __sit;
    void init(std::size_t n) {
      __pcs.init(n);
      __sidx.assign(n, 0);
      __sit.assign(n, _It{});
    }
  };

  const __prog& _P_;
  const __traits& __tr_;
  _It __first_, __last_;
  __flag_t __flags_;
  std::vector<int> __stk_;

  // Adds pc0 and everything reachable from it by epsilon moves at position c.
  void __closure(__threads& _Lp, int __pc0, std::ptrdiff_t __si, const _It& __sit, const __pos_ctx& c) {
    __stk_.clear();
    __stk_.push_back(__pc0);
    while (!__stk_.empty()) {
      const int __pc = __stk_.back();
      __stk_.pop_back();
      if (_Lp.__pcs.contains(__pc))
        continue;
      _Lp.__pcs.insert(__pc);
      _Lp.__sidx[static_cast<std::size_t>(__pc)] = __si;
      _Lp.__sit[static_cast<std::size_t>(__pc)] = __sit;
      const __re_inst<__charT>& in = _P_.code[static_cast<std::size_t>(__pc)];
      switch (in.op) {
      case __re_op::split:
        __stk_.push_back(in.b);
        __stk_.push_back(in.a);
        break;
      case __re_op::__jmp:
        __stk_.push_back(in.a);
        break;
      case __re_op::open:
      case __re_op::close:
        __stk_.push_back(__pc + 1);
        break;
      case __re_op::__bol:
      case __re_op::__eol:
      case __re_op::__wordb:
      case __re_op::__nwordb:
        if (::__ycxx::__detail::__re_assert(_P_, __tr_, in.op, __flags_, c.__at_first, c.__has_prev, c.prev, c.__at_end, c.cur))
          __stk_.push_back(__pc + 1);
        break;
      default:
        break;
      }
    }
  }

public:
  __re_nfa(const __prog& _Pp, const __traits& __tr, _It first, _It last, __flag_t __f) : _P_(_Pp), __tr_(__tr), __first_(first), __last_(last), __flags_(__f) {}

  // The leftmost-longest match: [bs, be) as offsets from first and as iterators.
  bool find(bool __whole, std::ptrdiff_t& __bs, _It& __bsit, std::ptrdiff_t& __be, _It& __beit) {
    namespace __rc = std::regex_constants;
    const bool __continuous = __whole || (__flags_ & __rc::match_continuous) != 0;
    const bool __not_null = (__flags_ & __rc::match_not_null) != 0;
    __threads __cl, __nl;
    __cl.init(_P_.code.size());
    __nl.init(_P_.code.size());
    _It cur = __first_;
    std::ptrdiff_t i = 0;
    __pos_ctx c;
    c.__at_first = true;
    c.__has_prev = (__flags_ & __rc::match_prev_avail) != 0;
    if (c.__has_prev) {
      _It __q = __first_;
      --__q;
      c.prev = *__q;
    }
    c.__at_end = cur == __last_;
    if (!c.__at_end)
      c.cur = *cur;
    bool found = false;
    for (;;) {
      if (!found && (i == 0 || !__continuous))
        __closure(__cl, 0, i, cur, c); // the new thread has the lowest priority
      for (std::size_t k = 0; k < __cl.__pcs.n; ++k) {
        const int __pc = __cl.__pcs.__dense[k];
        if (_P_.code[static_cast<std::size_t>(__pc)].op != __re_op::__match)
          continue;
        const std::ptrdiff_t __si = __cl.__sidx[static_cast<std::size_t>(__pc)];
        if ((__whole && !c.__at_end) || (__not_null && __si == i))
          continue;
        if (!found || __si < __bs || (__si == __bs && i > __be)) {
          found = true;
          __bs = __si;
          __bsit = __cl.__sit[static_cast<std::size_t>(__pc)];
          __be = i;
          __beit = cur;
        }
      }
      if (c.__at_end || (__cl.__pcs.n == 0 && (found || __continuous)))
        break;
      __nl.__pcs.clear();
      const __charT __ch = c.cur;
      _It __nx = cur;
      ++__nx;
      __pos_ctx __nc;
      __nc.__has_prev = true;
      __nc.prev = __ch;
      __nc.__at_end = __nx == __last_;
      if (!__nc.__at_end)
        __nc.cur = *__nx;
      for (std::size_t k = 0; k < __cl.__pcs.n; ++k) {
        const int __pc = __cl.__pcs.__dense[k];
        const __re_op op = _P_.code[static_cast<std::size_t>(__pc)].op;
        if (op != __re_op::__chr && op != __re_op::any && op != __re_op::set)
          continue;
        const std::ptrdiff_t __si = __cl.__sidx[static_cast<std::size_t>(__pc)];
        if (found && __si > __bs)
          continue;
        if (_P_.single(__tr_, __pc, __ch))
          __closure(__nl, __pc + 1, __si, __cl.__sit[static_cast<std::size_t>(__pc)], __nc);
      }
      using std::swap;
      swap(__cl, __nl);
      cur = __nx;
      ++i;
      c = __nc;
    }
    return found;
  }
};

// ---- POSIX: the subexpressions of a match -----------------------------------------------------
template <class _It, class __charT, class __traits>
class __re_posix_sub {
  using __prog = __re_program<__charT, __traits>;
  using __flag_t = std::regex_constants::match_flag_type;
  using node = __re_node<__charT>;

  const __prog& _P_;
  const __traits& __tr_;
  __flag_t __flags_;
  std::vector<_It> __its_;     // the positions of the match, [0, L]
  std::vector<__charT> __text_; // its characters
  std::ptrdiff_t _L_ = 0;
  bool __at_first0_ = false, __has_prev0_ = false, __end_is_last_ = false;
  __charT __prev0_{}, __after_{};
  std::vector<std::ptrdiff_t> __caps_; // per group: start, end (relative); -1 unset
  __re_pc_set __a_, __b_;
  std::vector<int> __stk_;

  bool __assert_at(__re_op op, std::ptrdiff_t p) const {
    const bool __has_prev = p > 0 || __has_prev0_;
    const __charT prev = p > 0 ? __text_[static_cast<std::size_t>(p - 1)] : __prev0_;
    const bool __at_end = p == _L_ && __end_is_last_;
    const __charT cur = p < _L_ ? __text_[static_cast<std::size_t>(p)] : __after_;
    return ::__ycxx::__detail::__re_assert(_P_, __tr_, op, __flags_, p == 0 && __at_first0_, __has_prev, prev, __at_end, cur);
  }
  static bool __consuming(__re_op op) { return op == __re_op::__chr || op == __re_op::any || op == __re_op::set; }

  // Forward: the closure of pc0 at position p inside [B, E); reaching E adds p to ends (in
  // increasing order, once).
  void __fclosure(__re_pc_set& _Sp, int __pc0, std::ptrdiff_t p, int _Bp, int _Ep, std::vector<std::ptrdiff_t>& ends) {
    __stk_.clear();
    __stk_.push_back(__pc0);
    while (!__stk_.empty()) {
      const int __pc = __stk_.back();
      __stk_.pop_back();
      if (__pc == _Ep) {
        if (ends.empty() || ends.back() != p)
          ends.push_back(p);
        continue;
      }
      if (__pc < _Bp || __pc > _Ep || _Sp.contains(__pc))
        continue;
      _Sp.insert(__pc);
      const __re_inst<__charT>& in = _P_.code[static_cast<std::size_t>(__pc)];
      switch (in.op) {
      case __re_op::split:
        __stk_.push_back(in.b);
        __stk_.push_back(in.a);
        break;
      case __re_op::__jmp:
        __stk_.push_back(in.a);
        break;
      case __re_op::open:
      case __re_op::close:
        __stk_.push_back(__pc + 1);
        break;
      case __re_op::__bol:
      case __re_op::__eol:
      case __re_op::__wordb:
      case __re_op::__nwordb:
        if (__assert_at(in.op, p))
          __stk_.push_back(__pc + 1);
        break;
      default:
        break;
      }
    }
  }
  // The positions e in [a, lim] such that node n matches [a, e), in increasing order. The work is
  // proportional to how far the node can match, not to lim - a.
  std::vector<std::ptrdiff_t> ends(int n, std::ptrdiff_t a, std::ptrdiff_t __lim) {
    const int _Bp = _P_.__node_begin[static_cast<std::size_t>(n)], _Ep = _P_.__node_end[static_cast<std::size_t>(n)];
    std::vector<std::ptrdiff_t> __res;
    __re_pc_set* __cl = &__a_;
    __re_pc_set* __nl = &__b_;
    __cl->clear();
    __fclosure(*__cl, _Bp, a, _Bp, _Ep, __res);
    for (std::ptrdiff_t p = a; p < __lim && __cl->n != 0; ++p) {
      __nl->clear();
      const __charT __ch = __text_[static_cast<std::size_t>(p)];
      for (std::size_t k = 0; k < __cl->n; ++k) {
        const int __pc = __cl->__dense[k];
        if (__consuming(_P_.code[static_cast<std::size_t>(__pc)].op) && _P_.single(__tr_, __pc, __ch))
          __fclosure(*__nl, __pc + 1, p + 1, _Bp, _Ep, __res);
      }
      __re_pc_set* t = __cl;
      __cl = __nl;
      __nl = t;
    }
    return __res;
  }
  // Backward: the closure of q0 at position p inside [B, E) over epsilon predecessors.
  void __bclosure(__re_pc_set& _Sp, int __q0, std::ptrdiff_t p, int _Bp, int _Ep) {
    __stk_.clear();
    __stk_.push_back(__q0);
    while (!__stk_.empty()) {
      const int __q = __stk_.back();
      __stk_.pop_back();
      if (_Sp.contains(__q))
        continue;
      _Sp.insert(__q);
      for (int e : _P_.__eps_pred[static_cast<std::size_t>(__q)]) {
        if (e < _Bp || e >= _Ep)
          continue;
        const __re_op op = _P_.code[static_cast<std::size_t>(e)].op;
        if ((op == __re_op::__bol || op == __re_op::__eol || op == __re_op::__wordb || op == __re_op::__nwordb) && !__assert_at(op, p))
          continue;
        __stk_.push_back(e);
      }
    }
  }
  // For each pc of `at`: the positions p in [s, t] such that (pc, p) reaches (end of n, t).
  std::vector<std::vector<char>> __reach(int n, std::ptrdiff_t s, std::ptrdiff_t t, const std::vector<int>& at) {
    const int _Bp = _P_.__node_begin[static_cast<std::size_t>(n)], _Ep = _P_.__node_end[static_cast<std::size_t>(n)];
    std::vector<std::vector<char>> __res(at.size(), std::vector<char>(static_cast<std::size_t>(t - s + 1), 0));
    __re_pc_set* __cl = &__a_;
    __re_pc_set* __nl = &__b_;
    __cl->clear();
    __bclosure(*__cl, _Ep, t, _Bp, _Ep);
    for (std::ptrdiff_t p = t;; --p) {
      for (std::size_t k = 0; k < at.size(); ++k)
        __res[k][static_cast<std::size_t>(p - s)] = __cl->contains(at[k]) ? 1 : 0;
      if (p == s || __cl->n == 0)
        break;
      __nl->clear();
      const __charT __ch = __text_[static_cast<std::size_t>(p - 1)];
      for (std::size_t k = 0; k < __cl->n; ++k) {
        const int c = __cl->__dense[k] - 1;
        if (c >= _Bp && c < _Ep && __consuming(_P_.code[static_cast<std::size_t>(c)].op) && _P_.single(__tr_, c, __ch))
          __bclosure(*__nl, c, p - 1, _Bp, _Ep);
      }
      __re_pc_set* __tmp = __cl;
      __cl = __nl;
      __nl = __tmp;
    }
    return __res;
  }
  // The largest end e >= lo in E from which the rest can still finish (R[e - s]), or -1.
  static std::ptrdiff_t __longest(const std::vector<std::ptrdiff_t>& _Ep, const std::vector<char>& _Rp, std::ptrdiff_t s,
                                std::ptrdiff_t __lo) {
    for (std::size_t k = _Ep.size(); k-- > 0;)
      if (_Ep[k] >= __lo && _Rp[static_cast<std::size_t>(_Ep[k] - s)])
        return _Ep[k];
    return -1;
  }
  void __reset_groups(const node& __x) {
    for (int __g = __x.__group_lo; __g < __x.__group_hi; ++__g) {
      __caps_[2 * static_cast<std::size_t>(__g)] = -1;
      __caps_[2 * static_cast<std::size_t>(__g) + 1] = -1;
    }
  }
  // Assigns the subexpressions of node n, which matches [s, t).
  void span(int n, std::ptrdiff_t s, std::ptrdiff_t t) {
    const node& __x = _P_.__nodes[static_cast<std::size_t>(n)];
    if (__x.__reset) // another iteration of a repeated atom: its groups report this one
      __reset_groups(__x);
    switch (__x.kind) {
    case __re_kind::__group:
      __caps_[2 * static_cast<std::size_t>(__x.__val)] = s;
      __caps_[2 * static_cast<std::size_t>(__x.__val) + 1] = t;
      span(__x.__kids[0], s, t);
      return;
    case __re_kind::__alt:
      for (int k : __x.__kids) {
        const auto _Ep = ends(k, s, t);
        if (!_Ep.empty() && _Ep.back() == t) {
          span(k, s, t);
          return;
        }
      }
      return;
    case __re_kind::concat: {
      std::vector<int> at;
      for (std::size_t i = 1; i < __x.__kids.size(); ++i)
        at.push_back(_P_.__node_begin[static_cast<std::size_t>(__x.__kids[i])]);
      const auto _Rp = __reach(n, s, t, at);
      std::ptrdiff_t cur = s;
      for (std::size_t i = 0; i + 1 < __x.__kids.size(); ++i) {
        const std::ptrdiff_t b = __longest(ends(__x.__kids[i], cur, t), _Rp[i], s, cur);
        if (b < 0)
          return; // cannot happen when [s, t) is a match of n
        span(__x.__kids[i], cur, b);
        cur = b;
      }
      span(__x.__kids.back(), cur, t);
      return;
    }
    case __re_kind::repeat: {
      const int __body = __x.__kids[0];
      if (s == t) { // one empty iteration if the body can match it and no iteration preceded
        if (!__x.__tail && !ends(__body, s, s).empty()) {
          __reset_groups(__x);
          span(__body, s, s);
        }
        return;
      }
      if (__x.max == 1) {
        __reset_groups(__x);
        span(__body, s, t);
        return;
      }
      const std::vector<int> at{_P_.__node_begin[static_cast<std::size_t>(n)]};
      const auto _Rp = __reach(n, s, t, at);
      for (std::ptrdiff_t cur = s; cur < t;) {
        const std::ptrdiff_t b = __longest(ends(__body, cur, t), _Rp[0], s, cur + 1);
        if (b < 0)
          return; // cannot happen when [s, t) is a match of n
        __reset_groups(__x);
        span(__body, cur, b);
        cur = b;
      }
      return;
    }
    default:
      return;
    }
  }

public:
  __re_posix_sub(const __prog& _Pp, const __traits& __tr, __flag_t __f) : _P_(_Pp), __tr_(__tr), __flags_(__f) {}

  // [bsit, bsit + len) is the match found in [first, last); fills out[1...].
  void __resolve(_It first, _It last, _It __bsit, std::ptrdiff_t __len, std::vector<__re_cap<_It>>& out) {
    _L_ = __len;
    __its_.reserve(static_cast<std::size_t>(__len + 1));
    __text_.reserve(static_cast<std::size_t>(__len));
    _It __it = __bsit;
    for (std::ptrdiff_t i = 0; i < __len; ++i) {
      __its_.push_back(__it);
      __text_.push_back(*__it);
      ++__it;
    }
    __its_.push_back(__it);
    __at_first0_ = __bsit == first;
    __has_prev0_ = !__at_first0_ || (__flags_ & std::regex_constants::match_prev_avail) != 0;
    if (__has_prev0_) {
      _It __q = __bsit;
      --__q;
      __prev0_ = *__q;
    }
    __end_is_last_ = __it == last;
    if (!__end_is_last_)
      __after_ = *__it;
    __a_.init(_P_.code.size() + 1);
    __b_.init(_P_.code.size() + 1);
    __caps_.assign(2 * static_cast<std::size_t>(_P_.__groups) + 2, -1);
    span(_P_.__root, 0, __len);
    for (std::size_t __g = 1; __g <= static_cast<std::size_t>(_P_.__groups); ++__g)
      if (__caps_[2 * __g] >= 0)
        out[__g] = __re_cap<_It>{__its_[static_cast<std::size_t>(__caps_[2 * __g])], __its_[static_cast<std::size_t>(__caps_[2 * __g + 1])], true};
  }
};

// ---- POSIX: the subexpressions of a match found by backtracking ---------------------------------
// The match [s, t) is known (re_backtracker, longest mode). POSIX's subexpression rule orders the
// ways of matching it lexicographically: each node's end, in the order the nodes are entered,
// the longest first; an alternation's alternatives in order. This search decides every node's
// end when it enters the node (largest first), so the first way that completes is the answer.
// Goals form continuations in an arena (a goal is "match node n on exactly [p, e), then the goal
// next"); choice points record where to resume, with marks into the arena and the capture undo
// log. A node no back-reference outside it refers to commits to its first success (a cut), and
// a node without back-references remembers the spans it failed on.
template <class _It, class __charT, class __traits>
class __re_posix_bt_sub {
  using __prog = __re_program<__charT, __traits>;
  using __flag_t = std::regex_constants::match_flag_type;
  using node = __re_node<__charT>;

  enum __gk : unsigned char { __g_match, __g_seq, __g_rep, __g_cut, __g_done };
  struct __goal {
    __gk k;
    int n;            // node
    int next;         // the continuation
    std::ptrdiff_t i; // seq: the element; rep: iterations done; cut: choice mark
    std::ptrdiff_t p, e;
  };
  enum __ck : unsigned char { __c_alt, __c_end, __c_empty, __c_memo };
  struct __choice {
    __ck k;
    int __goal; // the goal it resumes (memo: the node)
    int __cont; // alt: the continuation of the alternatives
    std::ptrdiff_t __cand, __lo;
    std::size_t __undo, __arena;
  };
  struct __memo_ent {
    int n = -1;
    std::ptrdiff_t p = 0, e = 0;
  };

  static constexpr std::size_t __max_entries = std::size_t(1) << 22;
  static constexpr std::size_t __max_memo = std::size_t(1) << 20;

  const __prog& _P_;
  const __traits& __tr_;
  __flag_t __flags_;
  _It __last_;
  std::vector<_It> __its_;
  std::vector<__charT> __text_;
  std::ptrdiff_t _L_ = 0;
  bool __at_first0_ = false, __has_prev0_ = false, __end_is_last_ = false;
  __charT __prev0_{}, __after_{};
  std::vector<std::ptrdiff_t> __caps_;
  std::vector<std::pair<std::size_t, std::ptrdiff_t>> __undo_;
  std::vector<__goal> __arena_;
  std::vector<__choice> __choices_;
  std::vector<__memo_ent> __memo_;
  std::size_t __memo_n_ = 0;
  std::vector<int> __run_of_;                    // per node: index into runs_, or -1
  std::vector<std::vector<std::ptrdiff_t>> __runs_; // single-character repetitions: run lengths
  std::size_t __steps_ = 0, __limit_ = 0;

  const node& __nd(int n) const { return _P_.__nodes[static_cast<std::size_t>(n)]; }
  std::ptrdiff_t __min(int n) const { return _P_.__nmin[static_cast<std::size_t>(n)]; }
  std::ptrdiff_t __max(int n) const { return _P_.__nmax[static_cast<std::size_t>(n)]; }
  bool __fits(int n, std::ptrdiff_t __len) const { return __len >= __min(n) && (__max(n) < 0 || __len <= __max(n)); }

  int __push(__gk k, int n, std::ptrdiff_t i, std::ptrdiff_t p, std::ptrdiff_t e, int next) {
    if (__arena_.size() >= __max_entries)
      ::__ycxx::__detail::__re_throw_stack();
    __arena_.push_back(__goal{k, n, next, i, p, e});
    return static_cast<int>(__arena_.size() - 1);
  }
  __choice& __push_choice(__ck k, int __g, int __cont, std::ptrdiff_t __cand, std::ptrdiff_t __lo) {
    if (__choices_.size() >= __max_entries)
      ::__ycxx::__detail::__re_throw_stack();
    __choices_.push_back(__choice{k, __g, __cont, __cand, __lo, __undo_.size(), __arena_.size()});
    return __choices_.back();
  }
  void __set_cap(std::size_t i, std::ptrdiff_t __v) {
    __undo_.emplace_back(i, __caps_[i]);
    __caps_[i] = __v;
  }
  void __reset_groups(const node& __x) {
    for (int __g = __x.__group_lo; __g < __x.__group_hi; ++__g)
      if (__caps_[2 * static_cast<std::size_t>(__g) + 1] >= 0) {
        __set_cap(2 * static_cast<std::size_t>(__g), -1);
        __set_cap(2 * static_cast<std::size_t>(__g) + 1, -1);
      }
  }
  void __restore(const __choice& c) {
    while (__undo_.size() > c.__undo) {
      __caps_[__undo_.back().first] = __undo_.back().second;
      __undo_.pop_back();
    }
    __arena_.resize(c.__arena, __goal{});
  }
  void __tick() {
    if (++__steps_ > __limit_)
      ::__ycxx::__detail::__re_throw_complexity();
  }

  // The failure memo of nodes without back-references (open addressing; stops growing when full).
  std::size_t __slot(int n, std::ptrdiff_t p, std::ptrdiff_t e) const {
    std::size_t h = static_cast<std::size_t>(n) * 0x9E3779B97F4A7C15ull;
    h ^= static_cast<std::size_t>(p) + 0x7F4A7C15ull + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(e) + 0x9E3779B9ull + (h << 6) + (h >> 2);
    return h & (__memo_.size() - 1);
  }
  bool __failed(int n, std::ptrdiff_t p, std::ptrdiff_t e) const {
    if (__memo_.empty())
      return false;
    for (std::size_t i = __slot(n, p, e);; i = (i + 1) & (__memo_.size() - 1)) {
      const __memo_ent& m = __memo_[i];
      if (m.n < 0)
        return false;
      if (m.n == n && m.p == p && m.e == e)
        return true;
    }
  }
  void __remember(int n, std::ptrdiff_t p, std::ptrdiff_t e) {
    if (__memo_.empty())
      __memo_.resize(1024);
    if (4 * (__memo_n_ + 1) > 3 * __memo_.size()) {
      if (__memo_.size() >= __max_memo)
        return;
      std::vector<__memo_ent> __old(__memo_.size() * 2);
      __old.swap(__memo_);
      __memo_n_ = 0;
      for (const __memo_ent& m : __old)
        if (m.n >= 0)
          __remember(m.n, m.p, m.e);
    }
    std::size_t i = __slot(n, p, e);
    while (__memo_[i].n >= 0)
      i = (i + 1) & (__memo_.size() - 1);
    __memo_[i] = __memo_ent{n, p, e};
    ++__memo_n_;
  }

  bool __assert_at(__re_op op, std::ptrdiff_t p) const {
    const bool __has_prev = p > 0 || __has_prev0_;
    const __charT prev = p > 0 ? __text_[static_cast<std::size_t>(p - 1)] : __prev0_;
    const bool __at_end = p == _L_ && __end_is_last_;
    const __charT cur = p < _L_ ? __text_[static_cast<std::size_t>(p)] : __after_;
    return ::__ycxx::__detail::__re_assert(_P_, __tr_, op, __flags_, p == 0 && __at_first0_, __has_prev, prev, __at_end, cur);
  }
  // A lookahead's body (an alternation of literal strings, from a non-matching list): does it
  // match at p? The input past the match is read through the iterators.
  bool __lit_match(int n, _It& __it) const {
    const node& __x = __nd(n);
    switch (__x.kind) {
    case __re_kind::__chr:
      if (__it == __last_ || _P_.__tx(__tr_, *__it) != _P_.code[static_cast<std::size_t>(_P_.__node_begin[static_cast<std::size_t>(n)])].__ch)
        return false;
      ++__it;
      return true;
    case __re_kind::concat:
      for (int k : __x.__kids)
        if (!__lit_match(k, __it))
          return false;
      return true;
    case __re_kind::__alt:
      for (int k : __x.__kids) {
        _It __j = __it;
        if (__lit_match(k, __j)) {
          __it = __j;
          return true;
        }
      }
      return false;
    default:
      return __x.kind == __re_kind::empty;
    }
  }
  // A repetition of one character, set or '.': the number of characters from p it can take.
  const std::vector<std::ptrdiff_t>* __run(int n) {
    const auto __un = static_cast<std::size_t>(n);
    if (__run_of_[__un] < 0) {
      const int __body = __nd(n).__kids[0];
      const int __pc = _P_.__node_begin[static_cast<std::size_t>(__body)];
      std::vector<std::ptrdiff_t> r(static_cast<std::size_t>(_L_ + 1), 0);
      for (std::ptrdiff_t p = _L_; p-- > 0;)
        if (_P_.single(__tr_, __pc, __text_[static_cast<std::size_t>(p)]))
          r[static_cast<std::size_t>(p)] = r[static_cast<std::size_t>(p + 1)] + 1;
      __run_of_[__un] = static_cast<int>(__runs_.size());
      __runs_.push_back(static_cast<std::vector<std::ptrdiff_t>&&>(r));
    }
    return &__runs_[static_cast<std::size_t>(__run_of_[__un])];
  }
  bool __single_rep(const node& __x) const {
    if (__x.kind != __re_kind::repeat || __x.max == 0) // x{0} has no code (and matches only "")
      return false;
    const __re_kind k = __nd(__x.__kids[0]).kind;
    return k == __re_kind::__chr || k == __re_kind::any || k == __re_kind::set;
  }

  // Resumes choice c with its next candidate: sets g, or returns false when none is left.
  bool __resume(__choice& c, int& __g) {
    const __goal _Gp = __arena_[static_cast<std::size_t>(c.__goal)];
    const node& __x = __nd(_Gp.n);
    switch (c.k) {
    case __c_alt:
      while (c.__cand < static_cast<std::ptrdiff_t>(__x.__kids.size())) {
        const int k = __x.__kids[static_cast<std::size_t>(c.__cand++)];
        if (__fits(k, _Gp.e - _Gp.p)) {
          __g = __push(__g_match, k, 0, _Gp.p, _Gp.e, c.__cont);
          return true;
        }
      }
      return false;
    case __c_end: {
      if (c.__cand < c.__lo)
        return false;
      const std::ptrdiff_t b = c.__cand--;
      if (_Gp.k == __g_seq) {
        const int __s2 = __push(__g_seq, _Gp.n, _Gp.i + 1, b, _Gp.e, _Gp.next);
        __g = __push(__g_match, __x.__kids[static_cast<std::size_t>(_Gp.i)], 0, _Gp.p, b, __s2);
      } else {
        __reset_groups(__x);
        const int __r2 = __push(__g_rep, _Gp.n, _Gp.i + 1, b, _Gp.e, _Gp.next);
        __g = __push(__g_match, __x.__kids[0], 0, _Gp.p, b, __r2);
      }
      return true;
    }
    case __c_empty: // first one empty iteration, then none
      if (c.__cand == 1) {
        c.__cand = 0;
        __reset_groups(__x);
        __g = __push(__g_match, __x.__kids[0], 0, _Gp.p, _Gp.p, _Gp.next);
        return true;
      }
      if (c.__cand == 0) {
        c.__cand = -1;
        __g = _Gp.next;
        return true;
      }
      return false;
    default:
      return false;
    }
  }
  // Starts choice point c (just pushed): its first candidate, or pops it and fails.
  bool __start(int& __g) {
    if (__resume(__choices_.back(), __g))
      return true;
    __choices_.pop_back();
    return false;
  }
  bool __backtrack(int& __g) {
    while (!__choices_.empty()) {
      __choice& c = __choices_.back();
      __restore(c);
      if (c.k == __c_memo) {
        __remember(c.__goal, c.__cand, c.__lo);
        __choices_.pop_back();
        continue;
      }
      if (__resume(c, __g))
        return true;
      __choices_.pop_back();
    }
    return false;
  }

  // Expands goal G (match node n on [p, e)); false: it fails.
  bool __expand(const __goal& _Gp, int& __g) {
    const int n = _Gp.n;
    const std::ptrdiff_t p = _Gp.p, e = _Gp.e;
    const node& __x = __nd(n);
    if (!__fits(n, e - p))
      return false;
    int next = _Gp.next;
    const unsigned char __fl = _P_.__nflags[static_cast<std::size_t>(n)];
    if ((__fl & __prog::__nf_commit) != 0 &&
        (__x.kind == __re_kind::concat || __x.kind == __re_kind::__alt || (__x.kind == __re_kind::repeat && !__single_rep(__x)))) {
      const bool __y_pure = (__fl & __prog::__nf_pure) != 0;
      if (__y_pure && __failed(n, p, e))
        return false;
      const std::size_t __mark = __choices_.size();
      if (__y_pure)
        __push_choice(__c_memo, n, 0, p, e);
      next = __push(__g_cut, 0, static_cast<std::ptrdiff_t>(__mark), 0, 0, next);
    }
    switch (__x.kind) {
    case __re_kind::empty:
      __g = next;
      return true;
    case __re_kind::__chr:
    case __re_kind::any:
    case __re_kind::set:
      if (!_P_.single(__tr_, _P_.__node_begin[static_cast<std::size_t>(n)], __text_[static_cast<std::size_t>(p)]))
        return false;
      __g = next;
      return true;
    case __re_kind::__bol:
    case __re_kind::__eol:
    case __re_kind::__wordb:
    case __re_kind::__nwordb: {
      const __re_op op = __x.kind == __re_kind::__bol   ? __re_op::__bol
                         : __x.kind == __re_kind::__eol ? __re_op::__eol
                         : __x.kind == __re_kind::__wordb ? __re_op::__wordb
                                                          : __re_op::__nwordb;
      if (!__assert_at(op, p))
        return false;
      __g = next;
      return true;
    }
    case __re_kind::__look:
    case __re_kind::__nlook: {
      _It __it = __its_[static_cast<std::size_t>(p)];
      if (__lit_match(__x.__kids[0], __it) != (__x.kind == __re_kind::__look))
        return false;
      __g = next;
      return true;
    }
    case __re_kind::__backref: {
      const auto __gi = 2 * static_cast<std::size_t>(__x.__val);
      if (__caps_[__gi + 1] < 0) // XBD 9.3.6: no string to match the same as
        return false;
      const std::ptrdiff_t s = __caps_[__gi], __len = __caps_[__gi + 1] - s;
      if (e - p != __len)
        return false;
      for (std::ptrdiff_t i = 0; i < __len; ++i)
        if (_P_.__tx(__tr_, __text_[static_cast<std::size_t>(s + i)]) != _P_.__tx(__tr_, __text_[static_cast<std::size_t>(p + i)]))
          return false;
      __g = next;
      return true;
    }
    case __re_kind::__group:
      __set_cap(2 * static_cast<std::size_t>(__x.__val), p);
      __set_cap(2 * static_cast<std::size_t>(__x.__val) + 1, e);
      __g = __push(__g_match, __x.__kids[0], 0, p, e, next);
      return true;
    case __re_kind::__alt:
      __push_choice(__c_alt, __push(__g_match, n, 0, p, e, next), next, 0, 0);
      return __start(__g);
    case __re_kind::concat:
      __g = __push(__g_seq, n, 0, p, e, next);
      return true;
    case __re_kind::repeat:
      if (__single_rep(__x)) { // the run decides; there is nothing inside to assign
        const std::ptrdiff_t __len = e - p;
        if ((*__run(n))[static_cast<std::size_t>(p)] < __len)
          return false;
        __g = next;
        return true;
      }
      __g = __push(__g_rep, n, 0, p, e, next);
      return true;
    }
    return false;
  }
  // The elements i... of a concatenation on [p, e).
  bool __seq(int __gidx, const __goal& _Gp, int& __g) {
    const node& __x = __nd(_Gp.n);
    const auto i = static_cast<std::size_t>(_Gp.i);
    const int k = __x.__kids[i];
    if (i + 1 == __x.__kids.size()) {
      __g = __push(__g_match, k, 0, _Gp.p, _Gp.e, _Gp.next);
      return true;
    }
    const auto __o = static_cast<std::size_t>(_P_.__suf_off[static_cast<std::size_t>(_Gp.n)]) + i + 1;
    const std::ptrdiff_t __rmin = _P_.__suf_min[__o], __rmax = _P_.__suf_max[__o];
    std::ptrdiff_t __hi = _Gp.e - __rmin, __lo = _Gp.p + __min(k);
    if (__max(k) >= 0 && _Gp.p + __max(k) < __hi)
      __hi = _Gp.p + __max(k);
    if (__rmax >= 0 && _Gp.e - __rmax > __lo)
      __lo = _Gp.e - __rmax;
    const node& __kx = __nd(k);
    if (__kx.kind == __re_kind::__backref) { // its length is known
      const auto __gi = 2 * static_cast<std::size_t>(__kx.__val);
      if (__caps_[__gi + 1] < 0)
        return false;
      const std::ptrdiff_t b = _Gp.p + __caps_[__gi + 1] - __caps_[__gi];
      __lo = b > __lo ? b : __lo;
      __hi = b < __hi ? b : __hi;
    } else if (__single_rep(__kx)) {
      const std::ptrdiff_t __r = (*__run(k))[static_cast<std::size_t>(_Gp.p)];
      if (_Gp.p + __r < __hi)
        __hi = _Gp.p + __r;
    }
    if (__lo > __hi)
      return false;
    __push_choice(__c_end, __gidx, 0, __hi, __lo);
    return __start(__g);
  }
  // A repetition with G.i iterations done, at G.p, to end at G.e.
  bool __rep(int __gidx, const __goal& _Gp, int& __g) {
    const node& __x = __nd(_Gp.n);
    const int __body = __x.__kids[0];
    const std::ptrdiff_t p = _Gp.p, e = _Gp.e, __cnt = _Gp.i;
    if (p == e) {
      if (__cnt < __x.min) { // needed for the minimum count
        __reset_groups(__x);
        __g = __push(__g_match, __body, 0, p, p, __push(__g_rep, _Gp.n, __cnt + 1, p, e, _Gp.next));
        return true;
      }
      if (__cnt == 0 && __x.max != 0 && __min(__body) == 0) { // the only iteration: one empty one
        __push_choice(__c_empty, __gidx, 0, 1, 0);
        return __start(__g);
      }
      __g = _Gp.next;
      return true;
    }
    if (__x.max >= 0 && __cnt >= __x.max)
      return false;
    // An iteration is not empty unless needed for the minimum count (XBD 9.3.6).
    std::ptrdiff_t __lo = p + (__min(__body) > 0 ? __min(__body) : __cnt < __x.min ? 0 : 1);
    std::ptrdiff_t __hi = e;
    if (__max(__body) >= 0 && p + __max(__body) < __hi)
      __hi = p + __max(__body);
    if (__cnt + 1 < __x.min && __min(__body) > 0) { // the rest of the minimum
      const std::ptrdiff_t __rest = __x.min - __cnt - 1;
      if (__rest > (e - p) / __min(__body))
        return false;
      if (e - __rest * __min(__body) < __hi)
        __hi = e - __rest * __min(__body);
    }
    if (__x.max >= 0 && __max(__body) > 0) { // what the remaining iterations can still take
      const std::ptrdiff_t __rem = __x.max - __cnt - 1;
      if (__rem <= (e - p) / __max(__body) && e - __rem * __max(__body) > __lo)
        __lo = e - __rem * __max(__body);
    }
    if (__lo > __hi)
      return false;
    __push_choice(__c_end, __gidx, 0, __hi, __lo);
    return __start(__g);
  }

  bool __search() {
    int __g = __push(__g_done, 0, 0, 0, 0, -1);
    __g = __push(__g_match, _P_.__root, 0, 0, _L_, __g);
    for (;;) {
      __tick();
      const __goal _Gp = __arena_[static_cast<std::size_t>(__g)];
      bool ok;
      switch (_Gp.k) {
      case __g_done:
        return true;
      case __g_cut: // the node succeeded: drop its choice points and its goals
        __choices_.resize(static_cast<std::size_t>(_Gp.i), __choice{});
        __arena_.resize(static_cast<std::size_t>(__g), __goal{});
        __g = _Gp.next;
        continue;
      case __g_match:
        ok = __expand(_Gp, __g);
        break;
      case __g_seq:
        ok = __seq(__g, _Gp, __g);
        break;
      default:
        ok = __rep(__g, _Gp, __g);
        break;
      }
      if (!ok && !__backtrack(__g))
        return false;
    }
  }

public:
  __re_posix_bt_sub(const __prog& _Pp, const __traits& __tr, __flag_t __f) : _P_(_Pp), __tr_(__tr), __flags_(__f) {}

  // [bsit, bsit + len) is the match found in [first, last); fills out[1...] if the search
  // completes (it does whenever the match is one of the program).
  void __resolve(_It first, _It last, _It __bsit, std::ptrdiff_t __len, std::vector<__re_cap<_It>>& out) {
    _L_ = __len;
    __last_ = last;
    __its_.reserve(static_cast<std::size_t>(__len + 1));
    __text_.reserve(static_cast<std::size_t>(__len));
    _It __it = __bsit;
    for (std::ptrdiff_t i = 0; i < __len; ++i) {
      __its_.push_back(__it);
      __text_.push_back(*__it);
      ++__it;
    }
    __its_.push_back(__it);
    __at_first0_ = __bsit == first;
    __has_prev0_ = !__at_first0_ || (__flags_ & std::regex_constants::match_prev_avail) != 0;
    if (__has_prev0_) {
      _It __q = __bsit;
      --__q;
      __prev0_ = *__q;
    }
    __end_is_last_ = __it == last;
    if (!__end_is_last_)
      __after_ = *__it;
    __caps_.assign(2 * static_cast<std::size_t>(_P_.__groups) + 2, -1);
    __run_of_.assign(_P_.__nodes.size(), -1);
    __limit_ = std::size_t(20000000) + std::size_t(32) * static_cast<std::size_t>(__len + 1) * _P_.__nodes.size();
    if (!__search())
      return; // cannot happen: keep the first-found subexpressions
    for (std::size_t __g = 1; __g <= static_cast<std::size_t>(_P_.__groups); ++__g)
      out[__g] = __caps_[2 * __g + 1] >= 0
                     ? __re_cap<_It>{__its_[static_cast<std::size_t>(__caps_[2 * __g])], __its_[static_cast<std::size_t>(__caps_[2 * __g + 1])], true}
                     : __re_cap<_It>{last, last, false};
  }
};

// Runs program P on [first, last): out receives 1 + groups entries.
template <class _It, class __charT, class __traits>
bool __re_execute(const __re_program<__charT, __traits>& _Pp, const __traits& __tr, _It first, _It last,
                std::regex_constants::match_flag_type __f, bool __whole, std::vector<__re_cap<_It>>& out) {
  if (!_Pp.__nfa) {
    __re_backtracker<_It, __charT, __traits> __bt(_Pp, __tr, first, last, __f);
    if (!__bt.search(__whole, out))
      return false;
    // POSIX: the subexpressions by the POSIX rule, inside the leftmost-longest match.
    if (_Pp.posix && _Pp.__groups > 0 && (_Pp.flags & std::regex_constants::nosubs) == 0) {
      std::ptrdiff_t __len = 0;
      for (_It __i = out[0].first; __i != out[0].second; ++__i)
        ++__len;
      __re_posix_bt_sub<_It, __charT, __traits> __sub(_Pp, __tr, __f);
      __sub.__resolve(first, last, out[0].first, __len, out);
    }
    return true;
  }
  std::ptrdiff_t __bs = 0, __be = 0;
  _It __bsit{}, __beit{};
  __re_nfa<_It, __charT, __traits> __sim(_Pp, __tr, first, last, __f);
  if (!__sim.find(__whole, __bs, __bsit, __be, __beit))
    return false;
  out.assign(static_cast<std::size_t>(_Pp.__groups) + 1, __re_cap<_It>{last, last, false});
  out[0] = __re_cap<_It>{__bsit, __beit, true};
  if (_Pp.__groups > 0 && (_Pp.flags & std::regex_constants::nosubs) == 0) {
    __re_posix_sub<_It, __charT, __traits> __sub(_Pp, __tr, __f);
    __sub.__resolve(first, last, __bsit, __be - __bs, out);
  }
  return true;
}

}} // namespace __ycxx::__detail
