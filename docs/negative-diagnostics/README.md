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
