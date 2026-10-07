## Part 3: text, numerics, time, input/output, concurrency, execution, Annex D

Clauses [text] (28), [numerics] (29), [time] (30), [input.output] (31), [thread] (32), [exec]
(33), Annex D [depr], [zombie.names], and the [version.syn] macros of their headers, against the
draft at github.com/Eelis/draft revision `c7015b485cc3` (eel.is, 2026-10-07). Where the clauses
moved: `<bit>` is [utilities] (22.11), `numeric_limits` is [support] (17.3.5) and saturation
arithmetic is [numeric.sat] in [algorithms] (26.10.17), so they belong to other parts; [print]
is in [input.output] ([print.syn], [print.fun], [ostream.formatted.print]); atomics, `<stdatomic.h>`,
hazard pointers and RCU are in [thread]; `atomic<shared_ptr<T>>` ([util.smartptr.atomic]) is
probed here with `<memory>`.

### How the audit works (re-run it when the draft changes)

`tools/spec_audit/part3/` (Python, no dependencies beyond the compilers):

1. `draft.py --html full.html` reads a saved https://eel.is/c++draft/full and writes
   `cache/regions.json`: every subclause of these clauses with its code blocks and item
   declarations (italics, "exposition only" and "freestanding"/"optional" comments kept as markers).
2. `gen.py` parses every synopsis code block (`decls.py`: namespaces, classes, template heads,
   access, declarations) into `inventory.tsv` (6351 declarations) and writes one probe file per
   subclause, `probes/<stable.name>.cpp` (the freestanding declarations again in
   `probes/freestanding/`), with `checks.tsv` listing every check. Each check is one line:
   a concept over a dummy type, so a failure is a false `static_assert` on its own line.
   Template parameters and placeholders (`integer-type`, `floating-point-type`, `integral-type`,
   `pointer-type`) are replaced by samples (`samples.py`; every integer and floating-point type
   for the placeholders). Kinds of check:
   - presence: `using ns::name;`, or `using C::name;` in a class derived from a sample
     specialization C (members, nested classes, protected members);
   - calls: a call with arguments of the declared parameter types is valid, has the declared
     return type, is `noexcept` where declared, is valid with the defaulted arguments left out,
     and is ill-formed for a deleted function; operators as expressions, hidden friends by ADL,
     explicit object parameters, conversion functions (and `explicit`);
   - constructors: constructible (by a new-expression, so protected destructors of facets do not
     matter), nothrow where `noexcept`, not convertible where `explicit`, not constructible where
     deleted; destructors: nothrow, virtual where declared;
   - types and variables: member types and aliases name the declared type, variables and data
     members have the declared type, specializations of variable templates have the declared value
     (`enable_view<filesystem::directory_iterator>`), class specializations are complete
     (`hash`/`formatter`: enabled), base classes are public bases, enumerations are scoped with
     the declared underlying type and enumerators;
   - deduction guides: CTAD gives the declared type;
   - constexpr: every function declared `constexpr`/`consteval` is called in a constant expression
     with sample arguments (0.5 for floating-point, 1 for other arithmetic types, value-initialized
     otherwise; `spec_probe::sample`). A failure counts only when the probed function itself is not
     usable in constant evaluation; a failure inside it for the sample (a pole error, a
     precondition, a null pointer, a type without a default constructor) is "undecided";
   - macros: each `#define` of a header synopsis is defined (the `⟨N⟩` macros of `<cinttypes>`
     for N = 8, 16, 32, 64; `// optional` ones are skipped), the Annex D macros, and every
     `__cpp_lib_*` macro that [version.syn] lists for one of these headers has the specified value
     in `<version>` and in each listed header (freestanding ones also with `-ffreestanding`);
   - freestanding: the declarations marked `// freestanding` and the headers whose synopsis is
     all freestanding (`<atomic>`) compile with `-ffreestanding -nostdinc` as
     `tools/check_freestanding.sh` does;
   - [zombie.names]: with every header included, no name of Table 38 is declared in `std`
     (an ambiguity test against a program's own declaration), no name of Tables 39-40 is a macro,
     no header of Table 41 exists (informative: the names are reserved, not forbidden).
3. `run.py [-c gcc|clang] [-j N] [--filter S]` compiles every probe file (`tools/ycxx-cxx`,
   `-fsyntax-only`), maps each diagnostic to its check, and compares with `gaps.tsv` (the known
   gaps, with their class). Exit status 1 for an unlisted failure or a listed gap that passes.
   `results-<compiler>.tsv` has every check's status.
4. `report.py` writes the tables below from the last run.

What the probes do not cover: the `wchar_t` (and other) specializations beyond the first sample
(`charT` is `char`; one sample per class template); exposition-only types in a signature
(presence only, 690 declarations); `constexpr` beyond one call with sample values; behaviour
(Effects, Returns, Throws, Complexity) other than through the existing own suite, which is the
semantic part of this audit (section "Semantic gaps" below). `samples.py` lists every place where
a sample is adjusted (`FUNC`, `MEMBER_CLASS`, `CONSTRAINED`), skipped (`SKIP`, `SKIP_DECLS`,
`NO_CONSTEXPR_PROBE`) or reduced to presence (`PRESENCE_ONLY`), with the reason.

<!-- part3:tables:begin -->
<!-- part3:tables:end -->

### Gaps found, classified

Classes: (1) missing entity, (2) wrong shape, (3) missing or wrong behaviour, (4) feature macro,
(5) intentional (STATUS/DECISIONS cited), (6) compiler gap, (7) draft defect.

**Fixed in this audit** (each with a test citing the paragraph):

| # | Class | Gap | Draft | Fix |
|---|---|---|---|---|
| F1 | 1 | `volatile atomic<T>::store_add` ... `store_fminimum_num` (19 members across the integral, floating-point and pointer specializations) existed only for always-lock-free `T`: `volatile atomic<long double>{}.store_add(1)` did not compile. They are now the deprecated overloads for `!is_always_lock_free`, like the other volatile members. | [atomics.types.int]/1, [atomics.types.float]/1, [atomics.types.pointer]/1, [depr.atomics.volatile]/1 | `f222ebf`; `tests/ycxx/atomic/volatile_store_ops_any_type.pass.cpp`, `tests/ycxx/depr/vol_store_add_big.compile.fail.cpp` |
| F2 | 2 | `stop_token::operator==` and `stop_source::operator==` were hidden friends; the draft declares defaulted members (`t.operator==(u)` did not compile). | [stoptoken.general]/1, [stopsource.general]/1 (see D1) | `9446019`; `tests/ycxx/stop_token/equality_member.pass.cpp` |
| F3 | 3 | Constant-evaluated `compare_exchange_weak/strong` of `atomic<long double>` and `atomic_ref<long double>` was not a constant expression on Clang: the value representations were compared over all 16 bytes, six of which are x87 padding (indeterminate in constant evaluation). | [atomics.types.float]/1, [atomics.ref.float]/1 (constexpr, P3309), [atomics.types.operations]/23 | `370b7e3`; `tests/ycxx/atomic/constexpr_cas_long_double.pass.cpp` |

**Open** (not fixed here):

| # | Class | Gap | Draft | Status / effort |
|---|---|---|---|---|
| G1 | 6 (Clang) | `__cpp_lib_constexpr_exceptions` is not defined with Clang 23 (also not in `<format>`, which [version.syn] lists): Clang cannot throw during constant evaluation (P3068). Library side complete (GCC defines it). | [version.syn] | STATUS "Known compiler gaps"; `gaps.tsv` |
| G2 | 3 | `chrono::parse`: the names of `%a %A %b %B %h %p` and `%c %x %X %r` are parsed in the "C" locale; the stream's locale (`time_get`) is not consulted. | [time.parse], Table 134 [tab:time.parse.spec] ("the locale's ...") | STATUS `<chrono>`; medium (parse through `time_get` of `is.getloc()` for those flags; 1-2 days) |
| G3 | 3 | `{:L}` chrono formatting with a non-classic `time_put` writes `%c %x %X` through `strftime` of a C `tm`, and the duration count is grouped from `numpunct` without calling a replaced `num_put`. | [time.format]/2-3 | STATUS `<chrono>`; small-medium |
| G4 | 3 | `<regex>` POSIX grammars (basic, extended, awk, grep, egrep): with back-references, or bounded repetitions beyond 256 copies / 65536 nodes, the matcher backtracks exhaustively and reports subexpressions in first-found order, not by the POSIX leftmost-longest rule for subexpressions. | [re.synopt]/1 (basic, extended, awk, grep: "shall be that used by ... in POSIX"; POSIX's subexpression rule) | STATUS `<regex>`; large (a POSIX subexpression-rule matcher: a week) |
| G5 | 3 | `<regex>`: multi-character collating elements (`[[.ch.]]`) are not supported (no C library locale defines them), and `regex_traits::transform_primary` returns the full sort key where it cannot find the primary one, where [re.traits]/7 would return an empty key. | [re.traits]/6-7 | STATUS `<regex>`; small (the empty-key choice) / blocked by the C library (collating elements) |
| G6 | 3 | `rcu_barrier()` called from inside a scheduled evaluation returns without waiting (waiting would deadlock), and inside a read-side region it does not wait for objects retired after the region began. | [saferecl.rcu.domain.func]/4 | STATUS concurrency; the draft gives no exception for these cases: a draft question as much as a gap |
| G7 | 3 | `notify_all_at_thread_exit` and the `*_at_thread_exit` results ([futures.promise], [futures.task.members]) never run for the thread that ends the process. | [thread.condition.nonmember] (`notify_all_at_thread_exit`), [futures.promise], [futures.task.members] (the `at_thread_exit` members) | STATUS concurrency; medium (run them from the exit path of the main thread) |
| G8 | 3 | `when_all`/`when_all_with_variant` and the `let_*` adaptors report no completion scheduler or domain in their attributes. | [exec.when.all], [exec.let] (but see D2: the draft's get-attrs is undefined) | STATUS `<execution>`; decide after D2 |
| G9 | 3 | `<filesystem>`: no root-names (`//host` is not special), ill-formed UTF-8 converts to U+FFFD, `permissions(..., nofollow)` on a link fails with ENOTSUP on Linux. | [fs.path.generic]/root-name (implementation-defined), [fs.op.permissions] | STATUS `<filesystem>`; root-names are implementation-defined (POSIX has none): (5) in effect; the others follow the OS |
| G10 | 3 | `tzdb`: zone data from the zoneinfo directory only; `remote_version`/`reload_tzdb` do not download; `sys_info::save` is derived (TZif has only an is-DST flag). | [time.zone.db.remote] (the remote source is implementation-defined), [time.zone.info.sys]/save | STATUS `<chrono>`; save: small heuristic already; no further work planned |

Documented choices where the draft leaves room or that STATUS lists as deliberate divergences
(class 5, not counted as gaps): `to_chars` fixed/general shortest forms, `num_get` of negative
fields into unsigned types, floating `from_chars` out of range, ECMAScript corner cases of
`<regex>`, `num_put` boolalpha padding and inserter failbit/badbit, x87 `%a` (STATUS "Deliberate
divergences"); named locales come from the C library, `messages` has no gettext, the base
`time_get`/`time_put` are the "C" locale's (STATUS "Iostreams/locale", DECISIONS §7); no
`native_handle` for mutexes and condition variables ([thread.req.native]/1 makes the members
implementation-defined; STATUS concurrency: the probes skip them); `simd` elementwise math and
`uninit_element` giving `T()` (STATUS `<simd>`); the `linalg` and parallel-algorithm execution
policy overloads run sequentially (nothing observable is required); `split`/`ensure_started`
are not provided (not in the draft, P3682); removed features (`<strstream>`, `<codecvt>`,
`wstring_convert`, ...; STATUS "Deliberate omissions"), which the [zombie.names] probes confirm
are not declared.

### Draft defects found (class 7)

- **D1** [stoptoken.general]/1 and [stopsource.general]/1 declare
  `bool operator==(const stop_token& rhs) noexcept = default;` (and for `stop_source`): a defaulted
  comparison member must be `const` ([class.compare.default]/1); both GCC 16 and Clang 23 reject
  the declaration as written. libycxx declares the `const` member (F2).
- **D2** [exec.snd.expos]/43: `basic-sender::get_env()` returns
  `impls-for<Tag>::get-attrs(data, child...)`, but `default-impls` no longer declares `get-attrs`
  and no `impls-for` specialization defines it: `get-attrs` is used once in the whole draft and
  defined nowhere, so the attributes of every library sender are unspecified.
- Already in STATUS "Draft issues noticed" and in these clauses: [linalg.algs.reqs]/1.1
  (`is_execution_policy` of a reference type), [exec.task.scheduler] (`ts-domain`'s
  `transform_sender` with and without the tag), [simd.bit]/15 (`shl`/`shr` Constraints),
  [exec.snd.transform]/3, [exec.affine]/5-7, [task.state]/5.2, [exec.sched]/6 vs
  [exec.run.loop.types]/5.

### Probe limitations (not gaps)

- `constexpr` undecided: 198 (GCC) / 199 (Clang) calls fail inside the function for the sample
  values (pole and domain errors of `<cmath>`/`<complex>` at 0.5 or 0, division by zero of
  value-initialized `duration`/`simd` operands, arithmetic on a null `atomic<T*>`, preconditions
  of `linalg` layouts and `simd` reductions, sample types without a default constructor:
  calendar types, `format_string`, `layout_transpose::mapping`). None is a call of a
  non-constexpr function; they are listed in `results-*.tsv` as UNDECIDED.
- Not probed (96 declarations): protected constructors and destructors of the facets, of
  `basic_streambuf`, `basic_ios`, `ios_base`, the `*_obj_base` classes; deduction guides whose
  result is `see below`; the `formatter` of `pair-or-tuple` (an exposition-only template); the
  extended floating-point stream operators (`extended-floating-point-type`: no sample on Clang).
- Skipped with a reason in `samples.py`: `abs`/`div` of `<cinttypes>` (declared only when
  `intmax_t` is an extended integer type), the `FP_FAST_FMA*` macros (optional in C), `// optional`
  declarations, `task::promise_type::return_void` (only for `task<void>`), the `native_handle`
  members.
