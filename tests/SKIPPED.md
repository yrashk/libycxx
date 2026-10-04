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
`locale.fr_FR.UTF-8`, `libcpp-*` configuration features, availability markers), tests with `RUN:`
lines, and `.verify.cpp` tests without `expected-error` (they check only warnings).
