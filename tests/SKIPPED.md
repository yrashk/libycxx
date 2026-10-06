# Skipped conformance tests

Tests are an oracle: they are never edited or weakened. A test is skipped only for one of the
reasons below. Skips are listed per suite in `tests/<suite>/skip.txt` and reported as
UNSUPPORTED with the reason.

| Category | Meaning |
|---|---|
| `removed` | Tests a feature removed from the standard (`result_of`, `is_literal_type`, `auto_ptr`, `<strstream>`, `<codecvt>`, ...). Deprecated features that the current draft still specifies (Annex D, [depr]) are implemented and their tests run; there is no `deprecated` category. |
| `divergence` | The test expects behaviour that contradicts the current working draft. Each entry cites the draft section. |
| `extension` | Tests a libc++ or libstdc++ extension, not standard behaviour. |
| `implementation-specific` | Asserts something the standard leaves unspecified (object sizes, hash values, behaviour after `#undef` of a reserved macro). |
| `compiler-internals` | Checks compiler output such as GCC tree dumps (`scan-tree-dump`), not library behaviour. |
| `pre-c++26` | Requires behaviour of an older standard mode. |
| `infrastructure` | Needs harness features we do not emulate (shell `RUN:` lines, `.sh.cpp`, generated tests, warning-only `-verify` tests). |

Automatically unsupported: tests whose `REQUIRES:` features we do not provide (locales such as
`locale.fr_FR.UTF-8` that the C library lacks, `libcpp-*` configuration features, availability markers), tests with `RUN:`
lines, and `.verify.cpp` tests without `expected-error` (they check only warnings).

## Expected failures (xfail.txt)

A test that fails only because of a cause outside the test and the library, a compiler gap or
bug (TRIAGE category D) or a draft defect, is listed in `tests/<suite>/xfail.txt` with the
compiler it applies to (`any` for a draft defect). It still runs: it reports XFAIL while it
fails, and XPASS, which fails the run, once the cause is gone and the line should go. CI fails on
any other failure.

## Unsupported in one configuration (unsupported.txt)

`tests/libcxx/unsupported.txt` (`<path regex> | <lit feature> | <reason>`; the libc++ suite, whose
lit configuration has features) lists tests that do not apply in one configuration only; they are
reported UNSUPPORTED while the lit feature is available and run normally otherwise. `root` (the harness runs as root, `os.geteuid() == 0`, as in CI's
Linux containers): tests that expect a permission error, which root never gets. `missing-locale.<name>`
(`tests/ycxxlit/locales.py`): tests that use a locale name without requiring it, run where the C
library lacks that locale (`tools/ci/gen-locales` generates it). `clang`: tests
that also exercise a libc++ extension under Clang only (`_BitInt`); the reason starts with the
skip category.
`tests/libstdcxx/unsupported.txt` (`<path regex> | gcc|clang | <reason>`) does the same for the
libstdc++ suite, with the compiler as the configuration: tests that rely on one compiler's
implementation-defined behaviour, predefined macros, attributes or optional types.
