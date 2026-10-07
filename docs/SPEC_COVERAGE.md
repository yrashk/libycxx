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
   access, declarations) into `inventory.tsv` (6351 declarations of the synopses, plus 14 item declarations no synopsis of these clauses repeats, `samples.EXTRA`) and writes one probe file per
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
(presence only, 686 declarations); `constexpr` beyond one call with sample values; behaviour
(Effects, Returns, Throws, Complexity) other than through the existing own suite, which is the
semantic part of this audit (section "Semantic gaps" below). `samples.py` lists every place where
a sample is adjusted (`FUNC`, `MEMBER_CLASS`, `CONSTRAINED`), skipped (`SKIP`, `SKIP_DECLS`,
`NO_CONSTEXPR_PROBE`) or reduced to presence (`PRESENCE_ONLY`), with the reason.

<!-- part3:tables:begin (generated by tools/spec_audit/part3/report.py; do not edit) -->

| Clause | Declarations | Present | Correct shape | Presence only | Not probed | Macros (ok/all) | Checks | Fail GCC | Fail Clang | constexpr undecided (GCC/Clang) |
|---|---|---|---|---|---|---|---|---|---|---|
| [text] Text processing | 919 | 882 | 882 | 75 | 37 | 15/15 | 1368 | 0 | 0 | 20/19 |
| [numerics] Numerics | 2005 | 1981 | 1981 | 242 | 24 | 18/18 | 2965 | 0 | 0 | 62/64 |
| [time] Time | 744 | 739 | 739 | 81 | 5 | 4/4 | 1085 | 0 | 0 | 86/86 |
| [input.output] Input/output | 1262 | 1248 | 1248 | 75 | 14 | 215/215 | 1579 | 0 | 0 | 1/1 |
| [thread] Concurrency support | 1122 | 1095 | 1095 | 93 | 27 | 25/25 | 5682 | 0 | 0 | 14/14 |
| [exec] Execution control | 273 | 260 | 260 | 113 | 13 | 0/0 | 274 | 0 | 0 | 0/0 |
| Annex D [depr] | 40 | 40 | 40 | 7 | 0 | 10/10 | 56 | 0 | 0 | 1/1 |
| [version.syn] feature-test macros of these headers | 0 | 0 | 0 | 0 | 0 | 145/147 | 147 | 0 | 2 | 0/0 |
| [zombie.names] | 0 | 0 | 0 | 0 | 0 | 79/79 | 79 | 0 | 0 | 0/0 |
| **Total** | 6365 | 6245 | 6245 | 686 | 120 | 511/513 | 13235 | 0 | 2 | 184/185 |

<details><summary>[text] Text processing: 55 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [charconv.syn] 28.2.1 | 18 | 18 | 18 | 2 | 0 | 79 (14) | 0/0 | [probe](../tools/spec_audit/part3/probes/charconv.syn.cpp) |
| [locale.syn] 28.3.2 | 43 | 43 | 43 | 26 | 0 | 43 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.syn.cpp) |
| [locale.general] 28.3.3.1.1 | 29 | 29 | 29 | 3 | 0 | 29 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.general.cpp) |
| [locale.facet] 28.3.3.1.2.2 | 5 | 2 | 2 | 1 | 3 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.facet.cpp) |
| [locale.id] 28.3.3.1.2.3 | 4 | 4 | 4 | 1 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.id.cpp) |
| [category.ctype.general] 28.3.4.2.1 | 14 | 14 | 14 | 1 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/category.ctype.general.cpp) |
| [locale.ctype.general] 28.3.4.2.2.1 | 29 | 28 | 28 | 0 | 1 | 31 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.ctype.general.cpp) |
| [locale.ctype.byname] 28.3.4.2.3 | 5 | 4 | 4 | 0 | 1 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.ctype.byname.cpp) |
| [facet.ctype.special.general] 28.3.4.2.4.1 | 28 | 27 | 27 | 0 | 1 | 28 | 0/0 | [probe](../tools/spec_audit/part3/probes/facet.ctype.special.general.cpp) |
| [locale.codecvt.general] 28.3.4.2.5.1 | 23 | 22 | 22 | 1 | 1 | 29 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.codecvt.general.cpp) |
| [locale.codecvt.byname] 28.3.4.2.6 | 4 | 3 | 3 | 0 | 1 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.codecvt.byname.cpp) |
| [locale.num.get.general] 28.3.4.3.2.1 | 28 | 27 | 27 | 0 | 1 | 29 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.num.get.general.cpp) |
| [locale.nm.put.general] 28.3.4.3.3.1 | 22 | 21 | 21 | 0 | 1 | 23 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.nm.put.general.cpp) |
| [locale.numpunct.general] 28.3.4.4.1.1 | 16 | 15 | 15 | 0 | 1 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.numpunct.general.cpp) |
| [locale.numpunct.byname] 28.3.4.4.2 | 6 | 5 | 5 | 0 | 1 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.numpunct.byname.cpp) |
| [locale.collate.general] 28.3.4.5.1.1 | 12 | 11 | 11 | 0 | 1 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.collate.general.cpp) |
| [locale.collate.byname] 28.3.4.5.2 | 5 | 4 | 4 | 0 | 1 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.collate.byname.cpp) |
| [locale.time.get.general] 28.3.4.6.2.1 | 23 | 22 | 22 | 1 | 1 | 31 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.time.get.general.cpp) |
| [locale.time.get.byname] 28.3.4.6.3 | 6 | 5 | 5 | 0 | 1 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.time.get.byname.cpp) |
| [locale.time.put.general] 28.3.4.6.4.1 | 9 | 8 | 8 | 0 | 1 | 11 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.time.put.general.cpp) |
| [locale.time.put.byname] 28.3.4.6.5 | 6 | 5 | 5 | 0 | 1 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.time.put.byname.cpp) |
| [locale.money.get.general] 28.3.4.7.2.1 | 11 | 10 | 10 | 0 | 1 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.money.get.general.cpp) |
| [locale.money.put.general] 28.3.4.7.3.1 | 11 | 10 | 10 | 0 | 1 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.money.put.general.cpp) |
| [locale.moneypunct.general] 28.3.4.7.4.1 | 29 | 28 | 28 | 2 | 1 | 36 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.moneypunct.general.cpp) |
| [locale.moneypunct.byname] 28.3.4.7.5 | 6 | 5 | 5 | 0 | 1 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.moneypunct.byname.cpp) |
| [locale.messages.general] 28.3.4.8.2.1 | 14 | 13 | 13 | 1 | 1 | 16 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.messages.general.cpp) |
| [locale.messages.byname] 28.3.4.8.3 | 6 | 5 | 5 | 0 | 1 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/locale.messages.byname.cpp) |
| [clocale.syn] 28.3.5.1 | 3 | 3 | 3 | 1 | 0 | 10 | 0/0 | [probe](../tools/spec_audit/part3/probes/clocale.syn.cpp) |
| [text.encoding.syn] 28.4.1 | 3 | 3 | 3 | 2 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/text.encoding.syn.cpp) |
| [text.encoding.overview] 28.4.2.1 | 15 | 14 | 14 | 2 | 1 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/text.encoding.overview.cpp) |
| [text.encoding.id] 28.4.2.6 | 1 | 1 | 1 | 0 | 0 | 260 | 0/0 | [probe](../tools/spec_audit/part3/probes/text.encoding.id.cpp) |
| [format.error] 28.5.10 | 3 | 3 | 3 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/format.error.cpp) |
| [format.syn] 28.5.1 | 54 | 53 | 53 | 10 | 1 | 69 | 0/0 | [probe](../tools/spec_audit/part3/probes/format.syn.cpp) |
| [format.fmt.string] 28.5.4 | 4 | 4 | 4 | 1 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/format.fmt.string.cpp) |
| [format.parse.ctx] 28.5.6.6 | 15 | 15 | 15 | 1 | 0 | 23 | 0/0 | [probe](../tools/spec_audit/part3/probes/format.parse.ctx.cpp) |
| [format.context] 28.5.6.7 | 8 | 8 | 8 | 1 | 0 | 10 | 0/0 | [probe](../tools/spec_audit/part3/probes/format.context.cpp) |
| [format.arg] 28.5.8.1 | 8 | 8 | 8 | 4 | 0 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/format.arg.cpp) |
| [format.args] 28.5.8.3 | 4 | 3 | 3 | 1 | 1 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/format.args.cpp) |
| [format.tuple] 28.5.9 | 6 | 0 | 0 | 0 | 6 | 0 | 0/0 |  |
| [re.regiter.general] 28.6.11.1.1 | 19 | 19 | 19 | 1 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.regiter.general.cpp) |
| [re.tokiter.general] 28.6.11.2.1 | 25 | 25 | 25 | 1 | 0 | 29 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.tokiter.general.cpp) |
| [re.syn] 28.6.3 | 65 | 61 | 61 | 7 | 4 | 77 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.syn.cpp) |
| [re.synopt] 28.6.4.2 | 12 | 12 | 12 | 0 | 0 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.synopt.cpp) |
| [re.matchflag] 28.6.4.3 | 14 | 14 | 14 | 0 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.matchflag.cpp) |
| [re.err] 28.6.4.4 | 14 | 14 | 14 | 0 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.err.cpp) |
| [re.badexp] 28.6.5 | 3 | 3 | 3 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.badexp.cpp) |
| [re.traits] 28.6.6 | 17 | 17 | 17 | 1 | 0 | 18 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.traits.cpp) |
| [re.regex.general] 28.6.7.1 | 44 | 44 | 44 | 1 | 0 | 54 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.regex.general.cpp) |
| [re.submatch.general] 28.6.8.1 | 14 | 14 | 14 | 0 | 0 | 15 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.submatch.general.cpp) |
| [re.results.general] 28.6.9.1 | 40 | 40 | 40 | 1 | 0 | 47 | 0/0 | [probe](../tools/spec_audit/part3/probes/re.results.general.cpp) |
| [cctype.syn] 28.7.1 | 14 | 14 | 14 | 0 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/cctype.syn.cpp) |
| [cwctype.syn] 28.7.2 | 21 | 21 | 21 | 0 | 0 | 22 | 0/0 | [probe](../tools/spec_audit/part3/probes/cwctype.syn.cpp) |
| [cwchar.syn] 28.7.3 | 68 | 68 | 68 | 1 | 0 | 74 (27) | 0/0 | [probe](../tools/spec_audit/part3/probes/cwchar.syn.cpp) |
| [cuchar.syn] 28.7.4 | 8 | 8 | 8 | 0 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/cuchar.syn.cpp) |
| [c.mb.wcs] 28.7.5 | 5 | 5 | 5 | 0 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/c.mb.wcs.cpp) |

</details>

<details><summary>[numerics] Numerics: 55 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [simd.syn] 29.10.3 | 433 | 433 | 433 | 140 | 0 | 681 | 0/0 | [probe](../tools/spec_audit/part3/probes/simd.syn.cpp) |
| [simd.flags.overview] 29.10.5.1 | 2 | 2 | 2 | 1 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/simd.flags.overview.cpp) |
| [simd.overview] 29.10.7.1 | 65 | 59 | 59 | 4 | 6 | 103 | 0/0 | [probe](../tools/spec_audit/part3/probes/simd.overview.cpp) |
| [simd.mask.overview] 29.10.9.1 | 40 | 39 | 39 | 3 | 1 | 64 | 0/0 | [probe](../tools/spec_audit/part3/probes/simd.mask.overview.cpp) |
| [stdckdint.h.syn] 29.11.1 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/stdckdint.h.syn.cpp) |
| [numerics.c.ckdint] 29.11.2 | 3 | 3 | 3 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/numerics.c.ckdint.cpp) |
| [cfenv.syn] 29.3.1 | 13 | 13 | 13 | 0 | 0 | 15 | 0/0 | [probe](../tools/spec_audit/part3/probes/cfenv.syn.cpp) |
| [complex.syn] 29.4.2 | 60 | 60 | 60 | 3 | 0 | 114 | 0/0 | [probe](../tools/spec_audit/part3/probes/complex.syn.cpp) |
| [complex] 29.4.3 | 20 | 20 | 20 | 1 | 0 | 36 | 0/0 | [probe](../tools/spec_audit/part3/probes/complex.cpp) |
| [c.math.rand] 29.5.10 | 2 | 2 | 2 | 0 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/c.math.rand.cpp) |
| [rand.synopsis] 29.5.2 | 47 | 43 | 43 | 30 | 4 | 43 (17) | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.synopsis.cpp) |
| [rand.eng.lcong] 29.5.4.2 | 18 | 18 | 18 | 1 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.eng.lcong.cpp) |
| [rand.eng.mers] 29.5.4.3 | 28 | 28 | 28 | 1 | 0 | 31 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.eng.mers.cpp) |
| [rand.eng.sub] 29.5.4.4 | 18 | 18 | 18 | 1 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.eng.sub.cpp) |
| [rand.eng.philox] 29.5.4.5 | 21 | 21 | 21 | 1 | 0 | 24 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.eng.philox.cpp) |
| [rand.adapt.disc] 29.5.5.2 | 20 | 20 | 20 | 1 | 0 | 22 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.adapt.disc.cpp) |
| [rand.adapt.ibits] 29.5.5.3 | 18 | 18 | 18 | 1 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.adapt.ibits.cpp) |
| [rand.adapt.shuf] 29.5.5.4 | 19 | 19 | 19 | 1 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.adapt.shuf.cpp) |
| [rand.device] 29.5.7 | 10 | 10 | 10 | 1 | 0 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.device.cpp) |
| [rand.util.seedseq] 29.5.8.1 | 10 | 10 | 10 | 1 | 0 | 10 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.util.seedseq.cpp) |
| [rand.dist.uni.int] 29.5.9.2.1 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.uni.int.cpp) |
| [rand.dist.uni.real] 29.5.9.2.2 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.uni.real.cpp) |
| [rand.dist.bern.bernoulli] 29.5.9.3.1 | 17 | 17 | 17 | 1 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.bern.bernoulli.cpp) |
| [rand.dist.bern.bin] 29.5.9.3.2 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.bern.bin.cpp) |
| [rand.dist.bern.geo] 29.5.9.3.3 | 17 | 17 | 17 | 1 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.bern.geo.cpp) |
| [rand.dist.bern.negbin] 29.5.9.3.4 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.bern.negbin.cpp) |
| [rand.dist.pois.poisson] 29.5.9.4.1 | 17 | 17 | 17 | 1 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.pois.poisson.cpp) |
| [rand.dist.pois.exp] 29.5.9.4.2 | 17 | 17 | 17 | 1 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.pois.exp.cpp) |
| [rand.dist.pois.gamma] 29.5.9.4.3 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.pois.gamma.cpp) |
| [rand.dist.pois.weibull] 29.5.9.4.4 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.pois.weibull.cpp) |
| [rand.dist.pois.extreme] 29.5.9.4.5 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.pois.extreme.cpp) |
| [rand.dist.norm.normal] 29.5.9.5.1 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.norm.normal.cpp) |
| [rand.dist.norm.lognormal] 29.5.9.5.2 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.norm.lognormal.cpp) |
| [rand.dist.norm.chisq] 29.5.9.5.3 | 17 | 17 | 17 | 1 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.norm.chisq.cpp) |
| [rand.dist.norm.cauchy] 29.5.9.5.4 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.norm.cauchy.cpp) |
| [rand.dist.norm.f] 29.5.9.5.5 | 18 | 18 | 18 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.norm.f.cpp) |
| [rand.dist.norm.t] 29.5.9.5.6 | 17 | 17 | 17 | 1 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.norm.t.cpp) |
| [rand.dist.samp.discrete] 29.5.9.6.1 | 19 | 19 | 19 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.samp.discrete.cpp) |
| [rand.dist.samp.pconst] 29.5.9.6.2 | 20 | 20 | 20 | 1 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.samp.pconst.cpp) |
| [rand.dist.samp.plinear] 29.5.9.6.3 | 20 | 20 | 20 | 1 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/rand.dist.samp.plinear.cpp) |
| [valarray.syn] 29.6.1 | 82 | 82 | 82 | 7 | 0 | 82 | 0/0 | [probe](../tools/spec_audit/part3/probes/valarray.syn.cpp) |
| [template.valarray.overview] 29.6.2.1 | 73 | 73 | 73 | 1 | 0 | 74 | 0/0 | [probe](../tools/spec_audit/part3/probes/template.valarray.overview.cpp) |
| [class.slice.overview] 29.6.4.1 | 8 | 8 | 8 | 1 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/class.slice.overview.cpp) |
| [template.slice.array.overview] 29.6.5.1 | 18 | 18 | 18 | 1 | 0 | 18 | 0/0 | [probe](../tools/spec_audit/part3/probes/template.slice.array.overview.cpp) |
| [class.gslice.overview] 29.6.6.1 | 6 | 6 | 6 | 1 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/class.gslice.overview.cpp) |
| [template.gslice.array.overview] 29.6.7.1 | 18 | 18 | 18 | 1 | 0 | 18 | 0/0 | [probe](../tools/spec_audit/part3/probes/template.gslice.array.overview.cpp) |
| [template.mask.array.overview] 29.6.8.1 | 18 | 18 | 18 | 1 | 0 | 18 | 0/0 | [probe](../tools/spec_audit/part3/probes/template.mask.array.overview.cpp) |
| [template.indirect.array.overview] 29.6.9.1 | 18 | 18 | 18 | 1 | 0 | 18 | 0/0 | [probe](../tools/spec_audit/part3/probes/template.indirect.array.overview.cpp) |
| [cmath.syn] 29.7.1 | 264 | 264 | 264 | 0 | 0 | 790 (6) | 0/0 | [probe](../tools/spec_audit/part3/probes/cmath.syn.cpp) |
| [numbers.syn] 29.8.1 | 39 | 26 | 26 | 0 | 13 | 26 | 0/0 | [probe](../tools/spec_audit/part3/probes/numbers.syn.cpp) |
| [linalg.transp.layout.transpose] 29.9.10.3 | 21 | 21 | 21 | 2 | 0 | 33 | 0/0 | [probe](../tools/spec_audit/part3/probes/linalg.transp.layout.transpose.cpp) |
| [linalg.syn] 29.9.2 | 186 | 186 | 186 | 11 | 0 | 190 | 0/0 | [probe](../tools/spec_audit/part3/probes/linalg.syn.cpp) |
| [linalg.layout.packed.overview] 29.9.6.1 | 25 | 25 | 25 | 2 | 0 | 37 | 0/0 | [probe](../tools/spec_audit/part3/probes/linalg.layout.packed.overview.cpp) |
| [linalg.scaled.scaledaccessor] 29.9.8.2 | 12 | 12 | 12 | 1 | 0 | 16 | 0/0 | [probe](../tools/spec_audit/part3/probes/linalg.scaled.scaledaccessor.cpp) |
| [linalg.conj.conjugatedaccessor] 29.9.9.2 | 11 | 11 | 11 | 1 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/linalg.conj.conjugatedaccessor.cpp) |

</details>

<details><summary>[time] Time: 41 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [time.zone.db.tzdb] 30.11.2.1 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.db.tzdb.cpp) |
| [time.zone.db.list] 30.11.2.2 | 10 | 10 | 10 | 2 | 0 | 10 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.db.list.cpp) |
| [time.zone.exception.nonexist] 30.11.3.1 | 2 | 2 | 2 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.exception.nonexist.cpp) |
| [time.zone.exception.ambig] 30.11.3.2 | 2 | 2 | 2 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.exception.ambig.cpp) |
| [time.zone.info.sys] 30.11.4.1 | 6 | 6 | 6 | 1 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.info.sys.cpp) |
| [time.zone.info.local] 30.11.4.2 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.info.local.cpp) |
| [time.zone.overview] 30.11.5.1 | 9 | 9 | 9 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.overview.cpp) |
| [time.zone.zonedtraits] 30.11.6 | 4 | 4 | 4 | 1 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.zonedtraits.cpp) |
| [time.zone.zonedtime.overview] 30.11.7.1 | 33 | 29 | 29 | 1 | 4 | 30 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.zonedtime.overview.cpp) |
| [time.zone.leap.overview] 30.11.8.1 | 5 | 5 | 5 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.leap.overview.cpp) |
| [time.zone.link.overview] 30.11.9.1 | 5 | 5 | 5 | 1 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.zone.link.overview.cpp) |
| [ctime.syn] 30.15 | 17 | 17 | 17 | 2 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/ctime.syn.cpp) |
| [time.syn] 30.2 | 375 | 374 | 374 | 43 | 1 | 564 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.syn.cpp) |
| [time.duration.general] 30.5.1 | 25 | 25 | 25 | 1 | 0 | 41 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.duration.general.cpp) |
| [time.point.general] 30.6.1 | 17 | 17 | 17 | 1 | 0 | 26 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.point.general.cpp) |
| [time.clock.conv] 30.7.10.1 | 1 | 1 | 1 | 1 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.conv.cpp) |
| [time.clock.system.overview] 30.7.2.1 | 9 | 9 | 9 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.system.overview.cpp) |
| [time.clock.utc.overview] 30.7.3.1 | 9 | 9 | 9 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.utc.overview.cpp) |
| [time.clock.utc.nonmembers] 30.7.3.3 | 2 | 2 | 2 | 0 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.utc.nonmembers.cpp) |
| [time.clock.tai.overview] 30.7.4.1 | 9 | 9 | 9 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.tai.overview.cpp) |
| [time.clock.gps.overview] 30.7.5.1 | 9 | 9 | 9 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.gps.overview.cpp) |
| [time.clock.file.overview] 30.7.6.1 | 1 | 1 | 1 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.file.overview.cpp) |
| [time.clock.steady] 30.7.7 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.steady.cpp) |
| [time.clock.hires] 30.7.8 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.clock.hires.cpp) |
| [time.cal.mdlast] 30.8.10 | 4 | 4 | 4 | 1 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.mdlast.cpp) |
| [time.cal.mwd.overview] 30.8.11.1 | 5 | 5 | 5 | 1 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.mwd.overview.cpp) |
| [time.cal.mwdlast.overview] 30.8.12.1 | 5 | 5 | 5 | 1 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.mwdlast.overview.cpp) |
| [time.cal.ym.overview] 30.8.13.1 | 10 | 10 | 10 | 1 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.ym.overview.cpp) |
| [time.cal.ymd.overview] 30.8.14.1 | 16 | 16 | 16 | 1 | 0 | 27 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.ymd.overview.cpp) |
| [time.cal.ymdlast.overview] 30.8.15.1 | 13 | 13 | 13 | 1 | 0 | 25 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.ymdlast.overview.cpp) |
| [time.cal.ymwd.overview] 30.8.16.1 | 17 | 17 | 17 | 1 | 0 | 30 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.ymwd.overview.cpp) |
| [time.cal.ymwdlast.overview] 30.8.17.1 | 13 | 13 | 13 | 1 | 0 | 25 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.ymwdlast.overview.cpp) |
| [time.cal.last] 30.8.2 | 2 | 2 | 2 | 1 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.last.cpp) |
| [time.cal.day.overview] 30.8.3.1 | 11 | 11 | 11 | 1 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.day.overview.cpp) |
| [time.cal.month.overview] 30.8.4.1 | 11 | 11 | 11 | 1 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.month.overview.cpp) |
| [time.cal.year.overview] 30.8.5.1 | 16 | 16 | 16 | 1 | 0 | 30 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.year.overview.cpp) |
| [time.cal.wd.overview] 30.8.6.1 | 16 | 16 | 16 | 1 | 0 | 27 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.wd.overview.cpp) |
| [time.cal.wdidx.overview] 30.8.7.1 | 6 | 6 | 6 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.wdidx.overview.cpp) |
| [time.cal.wdlast.overview] 30.8.8.1 | 4 | 4 | 4 | 1 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.wdlast.overview.cpp) |
| [time.cal.md.overview] 30.8.9.1 | 6 | 6 | 6 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.cal.md.overview.cpp) |
| [time.hms.overview] 30.9.1 | 11 | 11 | 11 | 1 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/time.hms.overview.cpp) |

</details>

<details><summary>[input.output] Input/output: 47 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [fstream.syn] 31.10.1 | 16 | 16 | 16 | 4 | 0 | 16 | 0/0 | [probe](../tools/spec_audit/part3/probes/fstream.syn.cpp) |
| [filebuf.general] 31.10.3.1 | 31 | 31 | 31 | 0 | 0 | 36 | 0/0 | [probe](../tools/spec_audit/part3/probes/filebuf.general.cpp) |
| [ifstream.general] 31.10.4.1 | 25 | 25 | 25 | 0 | 0 | 34 | 0/0 | [probe](../tools/spec_audit/part3/probes/ifstream.general.cpp) |
| [ofstream.general] 31.10.5.1 | 25 | 25 | 25 | 0 | 0 | 34 | 0/0 | [probe](../tools/spec_audit/part3/probes/ofstream.general.cpp) |
| [fstream.general] 31.10.6.1 | 25 | 25 | 25 | 0 | 0 | 34 | 0/0 | [probe](../tools/spec_audit/part3/probes/fstream.general.cpp) |
| [syncstream.syn] 31.11.1 | 7 | 7 | 7 | 2 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/syncstream.syn.cpp) |
| [syncstream.syncbuf.overview] 31.11.2.1 | 20 | 20 | 20 | 0 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/syncstream.syncbuf.overview.cpp) |
| [syncstream.osyncstream.overview] 31.11.3.1 | 19 | 19 | 19 | 0 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/syncstream.osyncstream.overview.cpp) |
| [fs.class.directory.entry.general] 31.12.10.1 | 48 | 48 | 48 | 1 | 0 | 48 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.class.directory.entry.general.cpp) |
| [fs.class.directory.iterator.general] 31.12.11.1 | 21 | 21 | 21 | 1 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.class.directory.iterator.general.cpp) |
| [fs.class.rec.dir.itr.general] 31.12.12.1 | 27 | 27 | 27 | 1 | 0 | 27 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.class.rec.dir.itr.general.cpp) |
| [fs.filesystem.syn] 31.12.4 | 129 | 129 | 129 | 13 | 0 | 132 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.filesystem.syn.cpp) |
| [fs.class.path.general] 31.12.6.1 | 92 | 92 | 92 | 3 | 0 | 100 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.class.path.general.cpp) |
| [fs.path.fmtr.general] 31.12.6.9.1 | 4 | 4 | 4 | 0 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.path.fmtr.general.cpp) |
| [fs.class.filesystem.error.general] 31.12.7.1 | 7 | 7 | 7 | 0 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.class.filesystem.error.general.cpp) |
| [fs.class.file.status.general] 31.12.9.1 | 13 | 13 | 13 | 1 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/fs.class.file.status.general.cpp) |
| [cstdio.syn] 31.13.1 | 48 | 48 | 48 | 0 | 0 | 66 | 0/0 | [probe](../tools/spec_audit/part3/probes/cstdio.syn.cpp) |
| [cinttypes.syn] 31.13.2 | 9 | 7 | 7 | 0 | 2 | 206 | 0/0 | [probe](../tools/spec_audit/part3/probes/cinttypes.syn.cpp) |
| [iosfwd.syn] 31.3.1 | 72 | 72 | 72 | 24 | 0 | 72 | 0/0 | [probe](../tools/spec_audit/part3/probes/iosfwd.syn.cpp) |
| [iostream.syn] 31.4.1 | 8 | 8 | 8 | 0 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/iostream.syn.cpp) |
| [ios.syn] 31.5.1 | 34 | 34 | 34 | 3 | 0 | 35 | 0/0 | [probe](../tools/spec_audit/part3/probes/ios.syn.cpp) |
| [ios.base.general] 31.5.2.1 | 61 | 60 | 60 | 3 | 1 | 64 | 0/0 | [probe](../tools/spec_audit/part3/probes/ios.base.general.cpp) |
| [ios.failure] 31.5.2.2.1 | 3 | 3 | 3 | 0 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/ios.failure.cpp) |
| [ios.init] 31.5.2.2.6 | 5 | 5 | 5 | 1 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/ios.init.cpp) |
| [fpos.general] 31.5.3.1 | 3 | 3 | 3 | 1 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/fpos.general.cpp) |
| [ios.overview] 31.5.4.1 | 37 | 36 | 36 | 0 | 1 | 39 | 0/0 | [probe](../tools/spec_audit/part3/probes/ios.overview.cpp) |
| [streambuf.syn] 31.6.1 | 3 | 3 | 3 | 1 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/streambuf.syn.cpp) |
| [streambuf.general] 31.6.3.1 | 48 | 46 | 46 | 1 | 2 | 52 | 0/0 | [probe](../tools/spec_audit/part3/probes/streambuf.general.cpp) |
| [istream.syn] 31.7.1 | 8 | 8 | 8 | 2 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/istream.syn.cpp) |
| [ostream.syn] 31.7.2 | 15 | 15 | 15 | 1 | 0 | 15 | 0/0 | [probe](../tools/spec_audit/part3/probes/ostream.syn.cpp) |
| [iomanip.syn] 31.7.3 | 14 | 14 | 14 | 0 | 0 | 20 | 0/0 | [probe](../tools/spec_audit/part3/probes/iomanip.syn.cpp) |
| [print.syn] 31.7.4 | 12 | 12 | 12 | 0 | 0 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/print.syn.cpp) |
| [istream.general] 31.7.5.2.1 | 54 | 51 | 51 | 1 | 3 | 53 | 0/0 | [probe](../tools/spec_audit/part3/probes/istream.general.cpp) |
| [istream.sentry] 31.7.5.2.4 | 6 | 6 | 6 | 1 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/istream.sentry.cpp) |
| [iostreamclass.general] 31.7.5.7.1 | 13 | 11 | 11 | 0 | 2 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/iostreamclass.general.cpp) |
| [ostream.general] 31.7.6.2.1 | 60 | 57 | 57 | 1 | 3 | 58 | 0/0 | [probe](../tools/spec_audit/part3/probes/ostream.general.cpp) |
| [ostream.sentry] 31.7.6.2.4 | 6 | 6 | 6 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/ostream.sentry.cpp) |
| [sstream.syn] 31.8.1 | 16 | 16 | 16 | 4 | 0 | 16 | 0/0 | [probe](../tools/spec_audit/part3/probes/sstream.syn.cpp) |
| [stringbuf.general] 31.8.2.1 | 40 | 40 | 40 | 0 | 0 | 49 | 0/0 | [probe](../tools/spec_audit/part3/probes/stringbuf.general.cpp) |
| [istringstream.general] 31.8.3.1 | 32 | 32 | 32 | 0 | 0 | 37 | 0/0 | [probe](../tools/spec_audit/part3/probes/istringstream.general.cpp) |
| [ostringstream.general] 31.8.4.1 | 32 | 32 | 32 | 0 | 0 | 37 | 0/0 | [probe](../tools/spec_audit/part3/probes/ostringstream.general.cpp) |
| [stringstream.general] 31.8.5.1 | 32 | 32 | 32 | 0 | 0 | 37 | 0/0 | [probe](../tools/spec_audit/part3/probes/stringstream.general.cpp) |
| [spanstream.syn] 31.9.2 | 16 | 16 | 16 | 4 | 0 | 16 | 0/0 | [probe](../tools/spec_audit/part3/probes/spanstream.syn.cpp) |
| [spanbuf.general] 31.9.3.1 | 19 | 19 | 19 | 0 | 0 | 23 | 0/0 | [probe](../tools/spec_audit/part3/probes/spanbuf.general.cpp) |
| [ispanstream.general] 31.9.4.1 | 17 | 17 | 17 | 0 | 0 | 19 | 0/0 | [probe](../tools/spec_audit/part3/probes/ispanstream.general.cpp) |
| [ospanstream.general] 31.9.5.1 | 15 | 15 | 15 | 0 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/ospanstream.general.cpp) |
| [spanstream.general] 31.9.6.1 | 15 | 15 | 15 | 0 | 0 | 17 | 0/0 | [probe](../tools/spec_audit/part3/probes/spanstream.general.cpp) |

</details>

<details><summary>[thread] Concurrency support: 65 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [futures.task.general] 32.10.10.1 | 4 | 2 | 2 | 1 | 2 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/futures.task.general.cpp) |
| [futures.task.members] 32.10.10.2 | 1 | 1 | 1 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/futures.task.members.cpp) |
| [future.syn] 32.10.2 | 23 | 22 | 22 | 5 | 1 | 31 | 0/0 | [probe](../tools/spec_audit/part3/probes/future.syn.cpp) |
| [futures.future.error] 32.10.4 | 4 | 4 | 4 | 0 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/futures.future.error.cpp) |
| [futures.promise] 32.10.6 | 14 | 14 | 14 | 3 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/futures.promise.cpp) |
| [futures.unique.future] 32.10.7 | 13 | 13 | 13 | 1 | 0 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/futures.unique.future.cpp) |
| [futures.shared.future] 32.10.8 | 13 | 13 | 13 | 1 | 0 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/futures.shared.future.cpp) |
| [rcu.syn] 32.11.2.2 | 6 | 6 | 6 | 2 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/rcu.syn.cpp) |
| [saferecl.rcu.base] 32.11.2.3 | 8 | 4 | 4 | 1 | 4 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/saferecl.rcu.base.cpp) |
| [saferecl.rcu.domain.general] 32.11.2.4.1 | 6 | 6 | 6 | 1 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/saferecl.rcu.domain.general.cpp) |
| [hazard.pointer.syn] 32.11.3.2 | 6 | 6 | 6 | 2 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/hazard.pointer.syn.cpp) |
| [saferecl.hp.base] 32.11.3.3 | 8 | 4 | 4 | 1 | 4 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/saferecl.hp.base.cpp) |
| [saferecl.hp.holder.general] 32.11.3.4.1 | 11 | 11 | 11 | 1 | 0 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/saferecl.hp.holder.general.cpp) |
| [stopcallback.inplace.general] 32.3.10.1 | 9 | 9 | 9 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/stopcallback.inplace.general.cpp) |
| [thread.stoptoken.syn] 32.3.2 | 13 | 13 | 13 | 10 | 0 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.stoptoken.syn.cpp) |
| [stoptoken.general] 32.3.4.1 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/stoptoken.general.cpp) |
| [stopsource.general] 32.3.5.1 | 9 | 9 | 9 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/stopsource.general.cpp) |
| [stopcallback.general] 32.3.6.1 | 10 | 10 | 10 | 1 | 0 | 10 | 0/0 | [probe](../tools/spec_audit/part3/probes/stopcallback.general.cpp) |
| [stoptoken.never] 32.3.7 | 5 | 5 | 5 | 2 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/stoptoken.never.cpp) |
| [stoptoken.inplace.general] 32.3.8.1 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/stoptoken.inplace.general.cpp) |
| [stopsource.inplace.general] 32.3.9.1 | 11 | 11 | 11 | 1 | 0 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/stopsource.inplace.general.cpp) |
| [thread.syn] 32.4.2 | 7 | 7 | 7 | 2 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.syn.cpp) |
| [thread.thread.class.general] 32.4.3.1 | 21 | 21 | 21 | 4 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.thread.class.general.cpp) |
| [thread.attributes.hint] 32.4.3.2.2 | 4 | 1 | 1 | 1 | 3 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.attributes.hint.cpp) |
| [thread.attributes.size] 32.4.3.2.3 | 2 | 2 | 2 | 1 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.attributes.size.cpp) |
| [thread.thread.id] 32.4.3.3 | 8 | 8 | 8 | 2 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.thread.id.cpp) |
| [thread.jthread.class.general] 32.4.4.1 | 23 | 23 | 23 | 1 | 0 | 23 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.jthread.class.general.cpp) |
| [thread.thread.this] 32.4.5 | 4 | 4 | 4 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.thread.this.cpp) |
| [atomics.flag] 32.5.10 | 17 | 17 | 17 | 1 | 0 | 31 (31) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.flag.cpp) |
| [stdatomic.h.syn] 32.5.12 | 0 | 0 | 0 | 0 | 0 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/stdatomic.h.syn.cpp) |
| [atomics.syn] 32.5.2 | 166 | 166 | 166 | 4 | 0 | 180 (180) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.syn.cpp) |
| [atomics.order] 32.5.4 | 1 | 1 | 1 | 0 | 0 | 6 (6) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.order.cpp) |
| [atomics.ref.generic.general] 32.5.7.1 | 23 | 23 | 23 | 1 | 0 | 42 (42) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.ref.generic.general.cpp) |
| [atomics.ref.int] 32.5.7.3 | 47 | 47 | 47 | 0 | 0 | 1545 (1545) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.ref.int.cpp) |
| [atomics.ref.float] 32.5.7.4 | 42 | 42 | 42 | 0 | 0 | 285 (285) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.ref.float.cpp) |
| [atomics.ref.pointer] 32.5.7.5 | 38 | 37 | 37 | 0 | 1 | 60 (60) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.ref.pointer.cpp) |
| [atomics.types.generic.general] 32.5.8.1 | 34 | 34 | 34 | 1 | 0 | 58 (58) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.types.generic.general.cpp) |
| [atomics.types.int] 32.5.8.3 | 81 | 81 | 81 | 0 | 0 | 2340 (2340) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.types.int.cpp) |
| [atomics.types.float] 32.5.8.4 | 71 | 71 | 71 | 0 | 0 | 435 (435) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.types.float.cpp) |
| [atomics.types.pointer] 32.5.8.5 | 63 | 63 | 63 | 0 | 0 | 105 (105) | 0/0 | [probe](../tools/spec_audit/part3/probes/atomics.types.pointer.cpp) |
| [util.smartptr.atomic.shared] 32.5.8.7.2 | 22 | 22 | 22 | 0 | 0 | 41 | 0/0 | [probe](../tools/spec_audit/part3/probes/util.smartptr.atomic.shared.cpp) |
| [util.smartptr.atomic.weak] 32.5.8.7.3 | 20 | 20 | 20 | 0 | 0 | 38 | 0/0 | [probe](../tools/spec_audit/part3/probes/util.smartptr.atomic.weak.cpp) |
| [mutex.syn] 32.6.2 | 21 | 21 | 21 | 11 | 0 | 21 | 0/0 | [probe](../tools/spec_audit/part3/probes/mutex.syn.cpp) |
| [shared.mutex.syn] 32.6.3 | 4 | 4 | 4 | 3 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/shared.mutex.syn.cpp) |
| [thread.mutex.class] 32.6.4.2.2 | 10 | 8 | 8 | 1 | 2 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.mutex.class.cpp) |
| [thread.mutex.recursive] 32.6.4.2.3 | 10 | 8 | 8 | 1 | 2 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.mutex.recursive.cpp) |
| [thread.timedmutex.class] 32.6.4.3.2 | 12 | 10 | 10 | 1 | 2 | 10 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.timedmutex.class.cpp) |
| [thread.timedmutex.recursive] 32.6.4.3.3 | 12 | 10 | 10 | 1 | 2 | 10 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.timedmutex.recursive.cpp) |
| [thread.sharedmutex.class] 32.6.4.4.2 | 13 | 11 | 11 | 1 | 2 | 11 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.sharedmutex.class.cpp) |
| [thread.sharedtimedmutex.class] 32.6.4.5.2 | 15 | 15 | 15 | 1 | 0 | 15 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.sharedtimedmutex.class.cpp) |
| [thread.lock.general] 32.6.5.1 | 6 | 6 | 6 | 3 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.lock.general.cpp) |
| [thread.lock.guard] 32.6.5.2 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.lock.guard.cpp) |
| [thread.lock.scoped] 32.6.5.3 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.lock.scoped.cpp) |
| [thread.lock.unique.general] 32.6.5.4.1 | 24 | 24 | 24 | 1 | 0 | 25 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.lock.unique.general.cpp) |
| [thread.lock.shared.general] 32.6.5.5.1 | 24 | 24 | 24 | 1 | 0 | 25 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.lock.shared.general.cpp) |
| [thread.once.onceflag] 32.6.7.1 | 4 | 4 | 4 | 1 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.once.onceflag.cpp) |
| [condition.variable.syn] 32.7.2 | 4 | 4 | 4 | 2 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/condition.variable.syn.cpp) |
| [thread.condition.condvar] 32.7.4 | 15 | 13 | 13 | 1 | 2 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.condition.condvar.cpp) |
| [thread.condition.condvarany.general] 32.7.5.1 | 16 | 16 | 16 | 1 | 0 | 16 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.condition.condvarany.general.cpp) |
| [semaphore.syn] 32.8.2 | 2 | 2 | 2 | 1 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/semaphore.syn.cpp) |
| [thread.sema.cnt] 32.8.3 | 11 | 11 | 11 | 1 | 0 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.sema.cnt.cpp) |
| [latch.syn] 32.9.2.2 | 1 | 1 | 1 | 1 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/latch.syn.cpp) |
| [thread.latch.class] 32.9.2.3 | 10 | 10 | 10 | 1 | 0 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.latch.class.cpp) |
| [barrier.syn] 32.9.3.2 | 1 | 1 | 1 | 1 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/barrier.syn.cpp) |
| [thread.barrier.class] 32.9.3.3 | 11 | 11 | 11 | 1 | 0 | 14 | 0/0 | [probe](../tools/spec_audit/part3/probes/thread.barrier.class.cpp) |

</details>

<details><summary>[exec] Execution control: 24 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [exec.cmplsig] 33.10 | 4 | 2 | 2 | 1 | 2 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.cmplsig.cpp) |
| [exec.prop] 33.11.1 | 3 | 2 | 2 | 2 | 1 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.prop.cpp) |
| [exec.env] 33.11.2 | 3 | 3 | 3 | 1 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.env.cpp) |
| [exec.run.loop.general] 33.12.1.1 | 7 | 7 | 7 | 1 | 0 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.run.loop.general.cpp) |
| [exec.with.awaitable.senders] 33.13.2 | 5 | 3 | 3 | 1 | 2 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.with.awaitable.senders.cpp) |
| [exec.inline.scheduler] 33.13.4 | 4 | 4 | 4 | 1 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.inline.scheduler.cpp) |
| [exec.task.scheduler] 33.13.5 | 8 | 8 | 8 | 1 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.task.scheduler.cpp) |
| [task.class] 33.13.6.2 | 12 | 11 | 11 | 2 | 1 | 11 | 0/0 | [probe](../tools/spec_audit/part3/probes/task.class.cpp) |
| [task.promise] 33.13.6.5 | 15 | 12 | 12 | 1 | 3 | 13 | 0/0 | [probe](../tools/spec_audit/part3/probes/task.promise.cpp) |
| [exec.scope.concepts] 33.14.1 | 2 | 2 | 2 | 2 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.scope.concepts.cpp) |
| [exec.scope.simple.counting.general] 33.14.2.2.1 | 9 | 9 | 9 | 2 | 0 | 9 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.scope.simple.counting.general.cpp) |
| [exec.simple.counting.token] 33.14.2.2.4 | 3 | 3 | 3 | 1 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.simple.counting.token.cpp) |
| [exec.scope.counting] 33.14.2.3 | 12 | 12 | 12 | 2 | 0 | 12 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.scope.counting.cpp) |
| [exec.par.scheduler] 33.15 | 1 | 1 | 1 | 1 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.par.scheduler.cpp) |
| [exec.parschedrepl.recvproxy] 33.16.2 | 7 | 6 | 6 | 1 | 1 | 7 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.parschedrepl.recvproxy.cpp) |
| [exec.parschedrepl.psb] 33.16.4 | 5 | 5 | 5 | 1 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.parschedrepl.psb.cpp) |
| [execution.syn] 33.4 | 156 | 154 | 154 | 83 | 2 | 157 (2) | 0/0 | [probe](../tools/spec_audit/part3/probes/execution.syn.cpp) |
| [exec.get.fwd.progress] 33.5.10 | 1 | 1 | 1 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.get.fwd.progress.cpp) |
| [exec.sched] 33.6 | 1 | 1 | 1 | 1 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.sched.cpp) |
| [exec.recv.concepts] 33.7.1 | 2 | 2 | 2 | 2 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.recv.concepts.cpp) |
| [exec.opstate.general] 33.8.1 | 1 | 1 | 1 | 1 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.opstate.general.cpp) |
| [exec.snd.concepts] 33.9.3 | 5 | 4 | 4 | 3 | 1 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.snd.concepts.cpp) |
| [exec.domain.indeterminate] 33.9.5 | 4 | 4 | 4 | 1 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.domain.indeterminate.cpp) |
| [exec.domain.default] 33.9.6 | 3 | 3 | 3 | 1 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/exec.domain.default.cpp) |

</details>

<details><summary>Annex D [depr]: 14 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [depr.numeric.limits.has.denorm] D.10 | 1 | 1 | 1 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.numeric.limits.has.denorm.cpp) |
| [depr.c.macros] D.11 | 0 | 0 | 0 | 0 | 0 | 5 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.c.macros.cpp) |
| [depr.cerrno] D.12 | 0 | 0 | 0 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.cerrno.cpp) |
| [depr.meta.types] D.13 | 8 | 8 | 8 | 4 | 0 | 8 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.meta.types.cpp) |
| [depr.relops] D.14 | 4 | 4 | 4 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.relops.cpp) |
| [depr.tuple] D.15 | 4 | 4 | 4 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.tuple.cpp) |
| [depr.variant] D.16 | 4 | 4 | 4 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.variant.cpp) |
| [depr.vector.bool.swap] D.17 | 2 | 2 | 2 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.vector.bool.swap.cpp) |
| [depr.iterator] D.18 | 6 | 6 | 6 | 1 | 0 | 6 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.iterator.cpp) |
| [depr.move.iter.elem] D.19 | 2 | 2 | 2 | 1 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.move.iter.elem.cpp) |
| [depr.format.syn] D.21.1 | 1 | 1 | 1 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.format.syn.cpp) |
| [depr.fs.path.factory] D.23.1 | 1 | 1 | 1 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.fs.path.factory.cpp) |
| [depr.fs.path.obs] D.23.2 | 3 | 3 | 3 | 1 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.fs.path.obs.cpp) |
| [depr.atomics.general] D.24.1 | 4 | 4 | 4 | 0 | 0 | 6 (6) | 0/0 | [probe](../tools/spec_audit/part3/probes/depr.atomics.general.cpp) |

</details>

<details><summary>[version.syn] feature-test macros of these headers: 39 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [version.syn@atomic] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 11 (11) | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@atomic.cpp) |
| [version.syn@barrier] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@barrier.cpp) |
| [version.syn@charconv] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 3 (2) | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@charconv.cpp) |
| [version.syn@chrono] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@chrono.cpp) |
| [version.syn@cmath] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 5 (1) | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@cmath.cpp) |
| [version.syn@complex] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@complex.cpp) |
| [version.syn@cwchar] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 (1) | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@cwchar.cpp) |
| [version.syn@execution] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 6 (1) | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@execution.cpp) |
| [version.syn@filesystem] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@filesystem.cpp) |
| [version.syn@format] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 5 | 0/1 | [probe](../tools/spec_audit/part3/probes/version.syn@format.cpp) |
| [version.syn@fstream] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@fstream.cpp) |
| [version.syn@hazard_pointer] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@hazard_pointer.cpp) |
| [version.syn@iomanip] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@iomanip.cpp) |
| [version.syn@ios] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@ios.cpp) |
| [version.syn@iosfwd] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@iosfwd.cpp) |
| [version.syn@istream] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@istream.cpp) |
| [version.syn@latch] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@latch.cpp) |
| [version.syn@linalg] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@linalg.cpp) |
| [version.syn@locale] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@locale.cpp) |
| [version.syn@mutex] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@mutex.cpp) |
| [version.syn@numbers] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@numbers.cpp) |
| [version.syn@ostream] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@ostream.cpp) |
| [version.syn@print] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@print.cpp) |
| [version.syn@random] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@random.cpp) |
| [version.syn@rcu] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@rcu.cpp) |
| [version.syn@regex] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@regex.cpp) |
| [version.syn@semaphore] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@semaphore.cpp) |
| [version.syn@shared_mutex] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 2 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@shared_mutex.cpp) |
| [version.syn@simd] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 4 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@simd.cpp) |
| [version.syn@spanstream] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@spanstream.cpp) |
| [version.syn@sstream] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@sstream.cpp) |
| [version.syn@stdatomic.h] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@stdatomic.h.cpp) |
| [version.syn@stdckdint.h] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@stdckdint.h.cpp) |
| [version.syn@stop_token] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@stop_token.cpp) |
| [version.syn@syncstream] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@syncstream.cpp) |
| [version.syn@text_encoding] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@text_encoding.cpp) |
| [version.syn@thread] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 3 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@thread.cpp) |
| [version.syn@valarray] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 1 | 0/0 | [probe](../tools/spec_audit/part3/probes/version.syn@valarray.cpp) |
| [version.syn@version] 17.3.2 | 0 | 0 | 0 | 0 | 0 | 69 (16) | 0/1 | [probe](../tools/spec_audit/part3/probes/version.syn@version.cpp) |

</details>

<details><summary>[zombie.names]: 1 subclauses</summary>

| Subclause | Decls | Present | Shape | Presence only | Not probed | Checks (freestanding) | Fail GCC/Clang | Probes |
|---|---|---|---|---|---|---|---|---|
| [zombie.names] 16.4.5.3.2 | 0 | 0 | 0 | 0 | 0 | 79 | 0/0 | [probe](../tools/spec_audit/part3/probes/zombie.names.cpp) |

</details>

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

- `constexpr` undecided: 184 (GCC) / 185 (Clang) calls (198/199 counting the freestanding repeats) fail inside the function for the sample
  values (pole and domain errors of `<cmath>`/`<complex>` at 0.5 or 0, division by zero of
  value-initialized `duration`/`simd` operands, arithmetic on a null `atomic<T*>`, preconditions
  of `linalg` layouts and `simd` reductions, sample types without a default constructor:
  calendar types, `format_string`, `layout_transpose::mapping`). None is a call of a
  non-constexpr function; they are listed in `results-*.tsv` as UNDECIDED.
- Not probed (120 declarations, 17 of them skipped with a reason): protected constructors and destructors of the facets, of
  `basic_streambuf`, `basic_ios`, `ios_base`, the `*_obj_base` classes; deduction guides whose
  result is `see below`; the `formatter` of `pair-or-tuple` (an exposition-only template); the
  extended floating-point stream operators (`extended-floating-point-type`: no sample on Clang).
- Skipped with a reason in `samples.py`: `abs`/`div` of `<cinttypes>` (declared only when
  `intmax_t` is an extended integer type), the `FP_FAST_FMA*` macros (optional in C), `// optional`
  declarations, `task::promise_type::return_void` (only for `task<void>`), the `native_handle`
  members.
