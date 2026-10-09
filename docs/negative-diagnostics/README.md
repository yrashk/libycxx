# Negative diagnostic audit

Baseline: `2f91a1a5238ba6d9b4302f2f55b9fd988093d1ec`.
Draft: `c7015b485cc3db8efaa9dfb9ff0809c5394a4ed1`.
The source link on https://eel.is/c++draft/ was independently checked against this revision.

`inventory.json` preserves the complete initial 422-file queue. All baseline paths are present
and still lack a directive. The other 50 negative tests initially have a pattern applicable to
both compilers. No AGENTS.md was found in the checkout or its ancestor directories.

Each repaired test has its own commit and its own ledger entry. Compiler output and temporary
controls are kept under `build/test-logs/negative-diagnostics/`. The ledger distinguishes own
library validation from reference implementations; their behavior does not decide the specification.
The final commit index records each repair SHA (a commit cannot record its own SHA).

Toolchains: GCC 16.2.0 (Homebrew), Clang 23.1.2, arm64 Darwin. Both CMake configurations provide
the detected C-library headers; unconfigured compilation produced incidental errors for absent
C23 declarations and is not evidence for these tests. Installed libstdc++ and LLVM libc++ are
used for reference diagnostics. Upstream suites are available under ~/.local/share/ycxx/suites.

Four random-engine/distribution tests overlap a separate semantic investigation. Their
supported-type policy remains that investigation's responsibility; any expectation here must
be documented as an implementation rejection if universal ill-formedness is not established.

Completed: all 422 baseline tests, with 422 separate test commits. `commits.json` maps each test
to its full SHA; `issues/` contains its draft clauses, changes, own-harness validation, causal
controls and reference commands. `summary.json` records final counts and exact aggregate commands.
`reference-comparison.json` classifies all installed-library observations, and
`upstream-comparison.json` records the 43 COUNTERPART mappings and original upstream runs.

The final 472-test negative suite has no failures: GCC 470 PASS / 2 UNSUPPORTED, Clang 469 PASS /
3 UNSUPPORTED. The two Linux-only policy cases are `depr/vol_store_add_big` and
`transitive_includes/strict/string_eof`; Clang also lacks the extended float32 type needed for
`cmath/nexttoward_extended`. The assigned queue is 422 PASS on GCC, and 421 PASS / 1 UNSUPPORTED
on Clang. There are no pending diagnostic repairs. All 472 files have valid, nonempty patterns
applicable to both compilers; prerequisites decide whether a configuration can execute a test.

The pair constructor fix passed 100 own utility/tuple tests on each compiler and the selected
upstream constructor tests. The final library builds passed on both compilers. Eight harness
regressions passed. The mapped upstream runs reported libstdc++ 2 PASS / 14 UNSUPPORTED and
libc++ 18 PASS / 20 UNSUPPORTED (warning-only tests, existing documented divergences, and older
language profiles); these skips are individually recorded and are not claimed as validation.

The four overlapping random tests have localized diagnostic-only commits. Their source policy
assertions are left for the separate semantic investigation. The 66 pre-existing deprecation
tests remain explicitly documented libycxx attribute policy tests: Annex D permits those
attributes, and `-Werror=deprecated-declarations` makes their warnings compilation failures.
Installed reference feature gaps and observed acceptance differences did not weaken the oracles.
