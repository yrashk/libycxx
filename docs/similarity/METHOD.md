# Similarity analysis: method

libycxx was implemented by AI agents from the C++ working draft, without access to other
implementations' sources. The models behind the agents were trained on public code that very
likely includes libstdc++, libc++ and the MSVC STL. This analysis measures how similar libycxx is
to those three libraries, and how similar they are to each other, so that every number has a
reference point. It is regenerated on every build of libycxx.org
([libycxx.org/similarity](https://libycxx.org/similarity/)) from pinned sources. The judgments it
applies are committed in advance, in this directory.

## Separation

- **Agents implementing libycxx must not read the rendered pages** (libycxx.org/similarity/,
  in particular the findings, fingerprint-item and tuning pages) or the CI artifacts of the
  Pages workflow. Those pages quote the other implementations.
- Nothing from the other implementations is committed: no code, identifiers, literals,
  constants or comments. The tools under `tools/similarity/` contain spelling rules, file paths
  and line numbers only. The judgments in this directory describe each match by category and in
  libycxx's terms. The policy stage of `tools/test` enforces this with a leak check (see
  [Leak check](#leak-check)).
- The other implementations' sources are fetched at build time into a cache outside the
  repository. Excerpts exist only in the rendered pages, with licence and attribution:
  - libstdc++: GNU GPL v3 with the GCC Runtime Library Exception;
  - libc++ and the MSVC STL: Apache License 2.0 with LLVM Exceptions;
  - libcxxrt: BSD.

## Why standard libraries are similar by necessity

Two independent implementations of the same specification share a great deal:

- **Code the standard specifies.** The standard specifies most of the library's interface and
  much of its code: function signatures and their order in synopses, "Effects: Equivalent to"
  bodies, and exposition-only classes (the ranges views are written out almost in full).
- **Common idioms.** All implementations use the same reserved-name spelling rules, the same
  idioms (iterator boilerplate, allocator plumbing) and published algorithms.
- **Specified ABIs and data.** ABIs (Itanium C++ ABI), registries (IANA, Unicode), POSIX and
  mathematical constants force names and values.

A useful measurement therefore asks a comparative question: **is libycxx more similar to one of
them than the established implementations are to each other?** It also has to tell what is
*forced* from what is an *arbitrary choice*.

## Sources

`tools/similarity/sources.json` pins every input:

- GCC 16.2.0's libstdc++, plus libiberty's demangler for the demangler area;
- LLVM 23.1.2's libc++ and libc++abi;
- an MSVC STL commit;
- libcxxrt, for the ABI runtime;
- the C++ working draft sources, for the standard's vocabulary;
- JPlag 6.2.0.

Archives are checked against their SHA-256, and Git sources are fetched by commit. Bumping a
version is a reviewed commit, because curated line ranges refer to the pinned versions. libycxx
is the checkout being built: its `include/` and `src/` trees.

**The positive control** is libc++'s C++03 fork (`libcxx/include/__cxx03`). It was copied from
libc++ in 2024 and has been maintained separately since. It is a known case of derived code, and
it shows what derivation looks like on each metric.

## Areas

43 library areas map files to files: vector, string, string_view, algorithm, sort and heap,
parallel algorithms, type_traits, tuple, variant, optional and expected, functional, smart
pointers, allocators, hash tables, red-black trees, deque, lists, flat containers, charconv,
format, regex, chrono, atomics, threads, random, iostreams, locale, filesystem, ranges,
iterators, mdspan, complex and valarray, cmath, bit and bitset, simd, stacktrace, exceptions, new
and delete, the ABI runtime, the demangler, text_encoding, utility and compare, and a
miscellaneous area. The mapping is in `tools/similarity/lib/areas.py`.

## Normalisation

The same rules apply to every implementation (`tools/similarity/lib/lexer.py`). Removed:

- comments;
- preprocessor directives, which carry licence headers, include guards and configuration;
- configuration and attribute macros (reserved names in capitals) with their arguments;
- `[[…]]` attribute lists and `__attribute__((…))`.

Line numbers are kept. Two token streams are produced:

- **structural**: identifiers and literals are abstracted, and runs of three or more literals
  (data tables) are collapsed;
- **lexical**: identifiers are kept, and the reserved-name conventions are reduced to a
  lower-case core (leading and trailing underscores, member and static prefixes, and a "My"
  prefix are removed). Template-parameter names become one placeholder.

## Metrics

1. **JPlag** 6.2.0 runs per area with the C/C++ scanner frontend, a minimum match of 12 tokens
   and one submission per library. The page reports the average similarity
   (2 × matched / (length A + length B)).
2. **k-gram Dice** is computed over distinct k-grams: structural with k = 24, lexical with
   k = 10. Greedy tiling, seeded by the k-grams, finds maximal common runs (at least 60
   structural or 25 lexical tokens) with their file and line ranges.
3. **Fingerprints** are computed over whole libraries. For each pair they count items shared
   *exclusively* by the two libraries, that is, absent from every other compared library. The
   categories:
   - internal identifiers, both in exact spelling and as a convention-free core of at least two
     words;
   - non-trivial numeric literals;
   - literal tables of 8 or more numbers, matched by their longest common run;
   - string literals of 6 or more characters;
   - comment lines of 6 or more words, and 8-word comment shingles;
   - misspellings: rare words one edit away from a frequent word, excluding inflections.

   Generic filters remove names and values that carry no signal:
   - compiler builtins, predefined macros and ABI- or platform-specified names;
   - template parameters;
   - the standard's own vocabulary, read from the draft sources (exposition-only names and
     code-font identifiers);
   - small numbers, powers of two and ten, bit masks, and calendar and Unicode constants.
4. **Tuning constants and design choices** are read from the pinned sources by generic
   extractors, at locations pinned in `tools/similarity/tuning-pins.json`. A value the extractor
   cannot read is shown as "not determined" with the reason. libycxx's column is committed, in
   `tuning.toml`.

## Calibration

Each metric is computed for every pair of the five libraries (libycxx, libstdc++, libc++, MSVC
STL, positive control). Medians and quartiles are taken over the areas, with charconv left out,
because the three established libraries share Ryu code there. The pages show the established
libraries' pairs with the same prominence as libycxx's.

## Triage, findings and dispositions

Every JPlag match of at least 40 tokens, and every lexical run of at least 25 tokens or
structural run of at least 80 tokens, between libycxx and another library is annotated with:

- the internal (reserved, non-standard) identifier cores both regions share;
- the share of libycxx's identifiers in the region that are standard vocabulary;
- a lexical similarity ratio.

Every such match, and every fingerprint item libycxx shares exclusively with another library,
gets exactly one disposition:

1. **A curated finding** (`findings.toml`) claims it. The finding locates its libycxx range by
   an anchor (a text the libycxx line contains, or the hash of that line) rather than a line
   number, so it survives small edits. A result is claimed when it lies in that range, involves
   the finding's other library, and is of a category the finding claims. A finding whose anchor
   is no longer found is shown as stale.
2. **An automatic rule** dismisses it:
   - *standard-shaped*: the match shares no internal name, and at least 70% of libycxx's
     identifiers in it are standard vocabulary;
   - *shape-only*: no shared internal name, and a lexical ratio below 0.5.
3. **A reviewed group** (`findings.toml`, `[[group]]`) lists its key. The keys are hashes, so
   the committed file names nothing:
   - for a fingerprint item, of its category, the other library and the value;
   - for a long match, of the other library, the libycxx file, the libycxx declaration the match
     lies in (the nearest line above it that starts a declaration at column 0, such as a class,
     a function or an alias), the other library's file, and the internal names the two regions
     share.

   A match key deliberately ignores the exact line range and the statements inside the
   declaration. Editing a statement in an already-reviewed declaration (a cast added, a
   comma-expression rewritten) does not reopen the review. Neither does a small shift of the
   matched range between runs: JPlag's match boundaries are not perfectly stable across machines.

   A match still surfaces as new when any of these changes:
   - it lies in another declaration;
   - it is against another file of the other library;
   - the two regions share an internal name they did not share before.
4. Otherwise it is **unreviewed**. It is listed openly on the findings page with its excerpt,
   and counted in the CI log.

### Severity

- **none**: no similarity beyond chance.
- **expected**: forced by the standard (specified code, exposition-only names, specified
  values), by an ABI, a platform or a registry (Itanium C++ ABI, POSIX, Unicode, IANA), or by a
  published algorithm or reference implementation (pdqsort, Schubfach, Eisel–Lemire, Ryu,
  Hinnant's date algorithms).
- **notable**: an arbitrary choice (name, constant, string, layout, local names) that matches
  one implementation in a short or isolated place, where coincidence or diffuse influence is
  plausible.
- **significant**: an extended arbitrary match, where several arbitrary elements appear
  together over a region (structure plus internal names, comments or constants over tens of
  lines). This suggests the region derives from that implementation.

## The verdict and its thresholds

The verdict on the overview page is static text from `verdict.toml`. It is shown only while all
of the following hold, and otherwise a "needs review" banner listing the crossed thresholds
replaces it:

- for each metric, libycxx's median similarity to each established library is at most the
  stated multiple of the largest median among the established libraries' own pairs:
  - JPlag: 2.0;
  - structural k-gram: 1.25;
  - lexical k-gram: 1.25;
- libycxx's JPlag upper quartile with each library stays below the positive control's lower
  quartile;
- no curated finding is *significant*;
- nothing is unreviewed;
- no finding is stale.

## Leak check

`tools/test`'s policy stage runs `tools/similarity/leakcheck --quick`. It compares every
identifier-like word, quoted string and code span of the committed similarity files against
`tools/similarity/data/foreign-vocab.txt`. That file holds truncated SHA-256 hashes of the
other implementations' reserved identifiers, string literals and comment lines (it is refreshed
with `tools/similarity/run --refresh-vocab`).

When the pinned sources are available (in CI, or after a local run), `--full` also compares
12-token windows of the committed files with the sources' token streams. The files checked
are:

- `tools/similarity/`;
- `docs/similarity/`;
- the site's own pages and templates.

## Limitations

- Token metrics cannot see paraphrase: code rewritten from memory with new names scores low. The
  fingerprints and the design-choice table address part of this. Influence at the level of
  design is a judgment, which the findings state with their reasons.
- The JPlag scanner is coarse and the lexer approximate. Both are applied identically to every
  library. Both branches of `#if` blocks are kept.
- The area mapping is made by hand. Code outside an area's files is covered only by the
  whole-library fingerprints.
- The established libraries are not perfectly independent of each other: they share Ryu, the
  parallel-algorithm lineage, names from the original HP/SGI STL, and some published reference
  code. That makes the baseline slightly generous.
- libcxxrt is compared only in the ABI areas. The MSVC runtime (vcruntime) is not open source.
  Other public sources the models may have seen (EASTL, STLport, Boost, the SGI STL, books) are
  not compared.
- The "forced or arbitrary" classifications are judgments. They are committed with their
  rationale and can be disputed in a pull request.
