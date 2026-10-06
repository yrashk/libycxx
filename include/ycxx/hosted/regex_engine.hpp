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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

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
          // An optional iteration that matched the empty string fails.
          if (s.count > __lp.min && __idx == s.__start_idx)
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

// Runs program P on [first, last): out receives 1 + groups entries.
template <class _It, class __charT, class __traits>
bool __re_execute(const __re_program<__charT, __traits>& _Pp, const __traits& __tr, _It first, _It last,
                std::regex_constants::match_flag_type __f, bool __whole, std::vector<__re_cap<_It>>& out) {
  if (!_Pp.__nfa) {
    __re_backtracker<_It, __charT, __traits> __bt(_Pp, __tr, first, last, __f);
    return __bt.search(__whole, out);
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
