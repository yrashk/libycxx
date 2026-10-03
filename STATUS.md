# libycxx status

## Toolchain
| Compiler | Version | Notes |
|---|---|---|
| GCC | 16.2.0 (built from source, `/opt/gcc-16`) | latest release |
| Clang | 23.1.2 (apt.llvm.org) | latest release |

Conformance oracle: libc++ test suite from `llvmorg-23.1.2` (`libcxx/test/std`, run only).

## Headers
| Header | Layer | Clang | GCC | Freestanding | Notes |
|---|---|---|---|---|---|
| `<cstddef>` `<cstdint>` `<climits>` | core | builds | builds | n/a yet | no libc headers |
| `<initializer_list>` `<version>` | core | builds | builds | n/a yet | |
| `<type_traits>` | core | builds | builds | n/a yet | `is_trivial` omitted (deprecated) |
| `<compare>` `<concepts>` | core (internal) | builds | builds | | public headers pending |

Conformance pass rates are not measured yet (harness pending).

## Known compiler gaps
- GCC 16.2: no `__builtin_is_within_lifetime`, so `std::is_within_lifetime` is unavailable with GCC.
- Clang 23.1: no `__builtin_is_corresponding_member` or
  `__builtin_is_pointer_interconvertible_with_class`, so those functions are GCC-only.

## Open issues
- Phase 1 in progress: utility, limits, bit, new, memory primitives, hosted runtime,
  CMake, lit harness, include-graph check, freestanding check.
