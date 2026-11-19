# macOS (arm64) exception failures: findings

Machine: Darwin 25.6.0 arm64, ld-27037.1, Clang 23.1.2 (provisioned), Homebrew GCC 16.2.0.
Branch `claude/sharp-meitner-e98ioi` at e40460e; `tools/test -c clang build`, `tools/test -c gcc build`.

## TL;DR

- **The working hypothesis does not hold.** No `__cxa_*`, personality, `std::terminate` or
  `__cxxabiv1` vtable binds to libc++abi. All of them are strong definitions from libycxx-abi.a,
  bound directly inside the executable.
- **The cause is dyld's weak-definition coalescing at load time.** libSystem loads
  `/usr/lib/libc++abi.dylib` and `/usr/lib/libc++.1.dylib` into every process, even a plain C
  `int main(){}`. libycxx itself does not link libc++. dyld coalesces weak definitions across all
  loaded images, and a **non-weak export wins over a weak one**. So every `std::` symbol that
  libycxx's headers emit as a *weak* definition in the program, and that libc++/libc++abi also
  export, is rebound at load time to Apple's copy:
  - `std::current_exception()` (inline in include/ycxx/core/exception_ptr.hpp:93) → libc++'s,
    which reads libc++abi's caught-exception stack. That stack is empty, because libycxx's runtime
    did the catch, so the result is null.
  - With GCC (which emits typeinfo and vtables of the all-inline exception classes as weak
    external), `typeinfo for std::exception`, the vtables and `what()` of std::exception,
    bad_alloc, bad_cast, bad_exception, logic_error, runtime_error and nested_exception, and so on
    all bind to libc++abi. As a result `catch (const std::exception&)` no longer matches a thrown
    `runtime_error`/`bad_function_call`: the base typeinfo is libc++abi's object, with
    libc++abi's `__cxxabiv1` vtable.
- **libc++ can't be kept out of the process** (libSystem → libdispatch/libxpc → libobjc →
  libc++; see §4c). The durable fix is to keep libycxx private to the image. Hidden visibility
  in the headers and archives works with both compilers and needs no link flags (§4b, option 1).
  The quick fix, verified against the test suites, is the export exclusion list:
- **Quick fix:** don't export those weak `std::` definitions from the image. Pass
  `-Wl,-unexported_symbols_list,<file>` (with wildcard patterns) on Darwin. They then become
  private and ld binds them directly. Result on `tools/run-conformance ycxx <cc> exception except rtti future`:
  **clang 36 → 2 failures, gcc 42 → 5 failures.** The remaining failures are not related to this
  bug (see §4). Patch: `macos-exception-fix.patch` (not committed, not applied).

## 1. What I ran

Scratch programs (in the session scratchpad), each built with `tools/ycxx-cxx clang|gcc`:

- **t1**: `throw 42; catch (int)` then `std::current_exception()`.
- **t2**: t1, plus `throw std::runtime_error` caught as `const std::exception&`, with the throws in
  a separate TU in a static archive (`libt2lib.a`).
- **t3**: exceptions thrown from libycxx.a: `std::function<void()>{}()` (caught as
  `bad_function_call&` and as `exception&`), `optional::value()`, `dynamic_cast<D&>`.
- **t4**: probe for the remaining `handler_pointer_reference` failure.
- Tools used: `xcrun dyld_info -imports`, `nm -m`, `DYLD_PRINT_BINDINGS=1`,
  `DYLD_PRINT_LIBRARIES=1`, `-Wl,-map`, `otool -L`, `xcrun dyld_info -exports` on
  /usr/lib/libc++.1.dylib and libc++abi.dylib.
- To list every collision: I built all 101 tests in tests/ycxx/{exception,except,rtti,future}
  with both compilers. Then I intersected the `<weak-def-coalesce>` imports with the exports of
  libc++.1/libc++abi.

## 2. Evidence

Before the fix, both compilers:
```
t1: caught 42, current_exception NULL                       (rc=1)
t2: int 7 cur=NULL; exception& 're' cur=NULL                (clang)
t2: int 7 cur=NULL; runtime_error NOT caught as std::exception&   (gcc)
t3 gcc: bad_function_call as exception&: caught by ... only
```

`nm -m t1c`: the whole runtime is in the executable, as strong externals:
```
000000010000150c (__TEXT,__text) external ___cxa_begin_catch
0000000100001a90 (__TEXT,__text) external ___cxa_end_catch
00000001000018a4 (__TEXT,__text) external ___cxa_get_globals
0000000100002104 (__TEXT,__text) external ___gxx_personality_v0
00000001000015a0 (__TEXT,__text) external __ZSt9terminatev
000000010000085c (__TEXT,__text) weak external __ZSt17current_exceptionv   <-- weak
```
`dyld_info -imports t1c`: the only non-libSystem imports are weak-coalesced:
```
0x0031  __ZNKSt13exception_ptrcvbEv  (from <weak-def-coalesce>)
0x0032  __ZSt17current_exceptionv    (from <weak-def-coalesce>)
```
`DYLD_PRINT_BINDINGS=1 ./t1c`, the bug itself:
```
looking for weak-def symbol '__ZSt17current_exceptionv':
  using non-weak '__ZSt17current_exceptionv' in /usr/lib/libc++.1.dylib
<t1c/bind#50> -> 0x190317f30 <libc++.1.dylib/__ZSt17current_exceptionv>)
```
GCC (t1g), many more, for example:
```
using non-weak '__ZTISt9exception' in /usr/lib/libc++abi.dylib
using non-weak '__ZNSt13bad_exceptionD1Ev' in /usr/lib/libc++abi.dylib
using non-weak '__ZTSi' in /usr/lib/libc++abi.dylib      (all fundamental typeinfo names)
```
libc++ is in every process: a C program `int main(){}` whose only dependency is libSystem
(`otool -L`) still loads both, according to `DYLD_PRINT_LIBRARIES=1`:
```
/usr/lib/libc++abi.dylib
/usr/lib/libc++.1.dylib
```
Collision set over the 101 exception, except, rtti and future tests (weak-coalesced imports ∩
libc++/libc++abi exports):
- clang, 5: `std::current_exception()`, `std::rethrow_exception(exception_ptr)`,
  `exception_ptr::operator=(const&)`, `nested_exception::rethrow_nested()`,
  `runtime_error::operator=`.
- gcc, 141: the 5 above, plus the ctors, dtors, `what()`, typeinfo and vtables of exception,
  bad_alloc, bad_array_new_length, bad_cast, bad_typeid, bad_exception, logic_error,
  runtime_error and nested_exception, `exception_ptr`'s copy constructor and destructor, and the
  typeinfo names of every fundamental type.

Other candidates, ruled out:
- **Archive-vs-dylib order, `-search_paths_first`, two-level namespace:** irrelevant. The
  executable links no C++ dylib at all, and every `__cxa_*` comes from libycxx-abi.a.
- **Personality and compact unwind:** fine. Our `__gxx_personality_v0` runs, and catches work.
  Only the lookups that go through the weak `std::` symbols fail.
- **Non-unique RTTI and emutls:** not the cause. GCC's `___emutls_get_address` is also weak-coalesced,
  but no system dylib exports it.

## 3. Hypothesis that holds

The cause is load-time weak-def coalescing against libc++.1.dylib and libc++abi.dylib, which
libSystem loads into every process. It is not link-time binding. `-unexported_symbol` alone
doesn't help because it takes no wildcards (tested: no effect). Wildcards work only in
`-unexported_symbols_list` / `-exported_symbols_list`.

Fix variants tested on t1/t2/t3, both compilers:

| variant | t1 | t2 | t3 |
|---|---|---|---|
| none | FAIL | FAIL | gcc FAIL |
| `-Wl,-exported_symbols_list` = {`_main`, `__mh_execute_header`} | ok | ok | ok |
| `-Wl,-unexported_symbol,__ZSt*` … (no wildcard support) | FAIL | FAIL | – |
| `-Wl,-unexported_symbols_list,cmake/darwin-unexported-symbols.txt` (the patch) | ok | ok | ok |

Hiding typeinfo is safe because libycxx's runtime compares `type_info` by name
(src/abi/rtti.hpp `same_type`), so a typeinfo object doesn't need to be unique across images.
The same flag is needed when linking a **dylib** against libycxx, because coalescing applies to
every image. The patch adds it to the `ycxx::ycxx` CMake interface, which covers dylibs too.

## 4. The fix and its verification

`macos-exception-fix.patch` (`git apply` checks clean on e40460e) does three things:
- adds `cmake/darwin-unexported-symbols.txt` with the patterns `__ZSt*`, `__ZNSt*`, `__ZNKSt*`,
  `__ZT{I,S,V,T}St*`, plus the typeinfo of the fundamental types and pointers to them
  (`__ZT[IS]?`, `__ZT[IS]P?`, `__ZT[IS]PK?`, `__ZT[IS]D*`, `__ZT[IS]u*`, …);
- `tools/ycxx-cxx`: on Darwin, appends `-Wl,-unexported_symbols_list,$here/cmake/darwin-unexported-symbols.txt`;
- `CMakeLists.txt`: on APPLE, `ycxx_ycxx` gets
  `LINKER:-unexported_symbols_list,<file>` (BUILD_INTERFACE and INSTALL_INTERFACE paths), and the
  file is installed with the CMake package.
  **This part is not exercised yet:** I did not run tests/cmake/run.sh. The libycxxConfig install
  path also needs a look.

Results with the patch applied (all t1/t2/t3 pass with both compilers, and
`tests/ycxx/exception/exception_ptr.pass.cpp` passes with both):

`tools/run-conformance ycxx <cc> exception except rtti future`:

| | before | after |
|---|---|---|
| clang | 75 pass, 36 fail, 2 xfail | 109 pass, **2 fail**, 2 xfail |
| gcc   | 71 pass, 42 fail | 108 pass, **5 fail** |

Remaining failures, which are not this bug:
- `except/handler_pointer_reference{,_exact}` (both compilers) and
  `except/handler_array_decay`, `except/handler_function_pointer` (gcc): `CHECK(which == 1)`.
  `catch (Base*&)` catches a thrown `Derived*`. This is the documented limitation in
  src/abi/rtti.cpp:266-268: the handler's type_info can't tell `T&` from `const T&`. Apple's own
  libc++abi behaves the same way (t4 built with plain `clang++`: `which=-1`). Check whether these
  are expected failures on Linux too; tests/ycxx has no xfail list.
- `exception/exception_ptr_constexpr` (gcc): fails at compile time, a compiler gap.

## 4b. Alternatives to the exclusion list (researched in dyld, ld64 and lld sources; tested here)

**How dyld picks a weak definition** (apple-oss-distributions/dyld, `dyld/Loader.cpp`,
`Loader::resolveSymbol`, `BIND_SPECIAL_DYLIB_WEAK_LOOKUP` branch):
- Only images with `MH_WEAK_DEFINES` take part. They are searched in load order, main executable
  first.
- The first definition found wins, except that a later *non-weak* definition replaces a weak one.
- When the winner is outside the shared cache, every cache image that uses the symbol is
  *patched* to use the winner.
- Only global (exported) weak definitions get weak-lookup binds. Hidden or private-extern weak
  definitions are bound directly (ld64 `OutputFile.cpp`; lld `Symbols.h`
  `isExternalWeakDef`/`needsBinding`). ld64.lld would behave the same way, and it also can't
  read this SDK's `.tbd` files (`unknown target arm64e.x1-macos`).

Options, ranked:

1. **Hidden visibility (recommended; this is what Chromium does for its bundled libc++/libc++abi
   on Mac).** Put `#pragma GCC visibility push(hidden)` around libycxx's `std::` and `__cxxabiv1`
   declarations in its headers (or use a Darwin-only `[[gnu::visibility("hidden")]]` on the
   namespaces), and build libycxx.a and libycxx-abi.a with `-fvisibility=hidden`.
   - Tested on scratch archives (`cmake -DCMAKE_CXX_FLAGS='-fvisibility=hidden -include vis.h'`,
     where vis.h is the pragma) and user TUs (`-include vis.h`): **t1, t2 and t3 all pass with
     both compilers**, with no link flags. Clang leaves no weak-coalesce binds at all. GCC leaves
     only the fundamental typeinfo *names* (`__ZTSi` …, harmless strings) and
     `___emutls_get_address`.
   - GCC needs `-Wno-attributes`, or no visibility pragma on the `__cxa_*` definitions:
     src/abi/exception.cpp:133 fails with "visibility attribute ignored because it conflicts with
     previous declaration", against GCC's builtin `__cxa_begin_catch`/`__cxa_end_catch`.
   - Works for dylibs too, since nothing is exported. Costs: a program can't export these
     symbols to plugins, and a dylib and a program each built with libycxx don't share exception
     *state*. Type matching still works across images because `same_type` compares names.
2. **The unexported-symbols list** (the patch above): the same effect, applied at link time.
   It needs the link flag in every link (ycxx-cxx, the CMake target, users' own builds).
3. **`-Wl,-force_symbols_not_weak_list,FILE`:** the symbol stays exported but is not weak, so
   there is no coalescing. The research agent confirmed this in a test. Apple's ld only, and
   deprecated in ld-prime.
4. **`-Wl,-no_fixup_chains`:** it works because dyld skips non-weak *cache* exports for binaries
   in the old bind format (rdar://75956202 in Loader.cpp). Undocumented, legacy and slower;
   don't use it.
5. Things that don't help: `-Wl,-bind_at_load` (tested, no change); `-no_weak_exports` (only
   makes them an error); `-unexported_symbol` (no wildcards); `-fvisibility-inlines-hidden`
   (fixes Clang, but GCC's typeinfo and vtables stay weak-exported).

**Also check: dyld's coalescing works in the other direction too.** The system's libc++abi
exports about 47 *weak* symbols, among them operator new/delete and the typeinfo of the
stdexcept classes (`range_error`, `length_error`, …). If the program *exports* a weak or strong
definition of one of these, the program wins and **dyld patches the shared cache**, so system
code (libobjc, the frameworks) uses libycxx's copy.
- Already happening: `DYLD_PRINT_BINDINGS` for t1c shows `using '__ZdlPv' in …/t1c` (libycxx's
  operator delete for the whole process) while `__Znwm` comes from libc++abi. That pair only
  works because both sides use malloc/free.
- Whether a replaceable global new/delete should be process-wide on Darwin is a libycxx design
  decision. Chromium deliberately hides it with `-fvisibility-global-new-delete=force-hidden`.
- Either way, typeinfo and vtables of `std::` classes must not be exported, or system code ends
  up calling libycxx's `type_info` virtuals.

## 4c. Can libc++ be kept out of the process, or replaced by libycxx? No.

- A plain C `int main(){}` loads 1,155 images on macOS 26 (`DYLD_PRINT_LIBRARIES`).
- The chain, from `dyld_info -dependents`: libSystem re-exports `libdispatch`, `libxpc`,
  `libsystem_featureflags`, … These have an *upward* link to `/usr/lib/libobjc.A.dylib`, which
  links `libc++abi.dylib` and `libc++.1.dylib` as ordinary (non-delayed) dependencies.
- dyld refuses a program without libSystem ("program does not link with libSystem.B.dylib",
  dyldMain.cpp), and static executables are unsupported.
- Interposing is read only from dylibs outside the shared cache, not from the main executable.
  It redirects calls but doesn't stop the load, and AMFI can forbid it.
- Overriding the cached `/usr/lib/libc++*.dylib` from disk (`DYLD_LIBRARY_PATH`,
  `DYLD_INSERT_LIBRARIES`) needs a development cache or Developer Mode. Hardened runtime and SIP
  ignore `DYLD_*`. So none of these works for production.
- Redirecting the system's libc++ users to libycxx would also need libycxx to implement Apple's
  `std::__1` ABI exactly (libobjc, ICU, CoreFoundation and others call into it). Not feasible.
- The workable model is the one Chromium uses: let Apple's libc++ load, and keep libycxx entirely
  private to the image (option 1 above).

## 5. User's question: can libycxx use Apple's exception runtime (libc++abi) as an option?

This means libc++abi only, never libc++'s `std::` API. Today libycxx does not link libc++; the
only libc++ involvement is that the OS loads it into the process.

**Experiment:** link libycxx.a with `-lc++abi` instead of libycxx-abi.a. Only two libycxx-private
hooks are left unresolved for t1/t3: `ycxx::abi::current_exception_object` and
`exception_ptr_release`. A 15-line shim maps the four basic hooks onto libc++abi's documented
exports:
- `__cxa_current_primary_exception`
- `__cxa_increment_exception_refcount`
- `__cxa_decrement_exception_refcount`
- `__cxa_rethrow_primary_exception`

With that shim, t1 (current_exception), t5 (exception_ptr capture and rethrow), and t3 under GCC
all pass. `otool -L` shows only libc++abi and libSystem.

**Divergences that would need work:**
- **`dynamic_cast<D&>` → `bad_cast` with Clang is not caught (t3 `bad_cast: ... only`).** libc++abi's
  `__dynamic_cast` and `__cxa_bad_cast` throw *libc++abi's* `std::bad_cast`. Its typeinfo bypasses
  libycxx's inline one, so catching it needs libycxx's class to be the same object as libc++abi's.
  Generally, libycxx's all-inline, constexpr exception classes (P3068) would have to bind to
  libc++abi's exports. Their `what()` strings then come from libc++abi and can differ from
  libycxx's (for example `bad_optional_access`).
- **`exception_object_as`** (exception_ptr_cast) and **`exception_object_create`**
  (make_exception_ptr without exceptions) have no public libc++abi equivalent. The first could be
  done by rethrowing into `try { } catch (const E&)`. The second could use
  `__cxa_allocate_exception` plus `__cxa_init_primary_exception`, which libc++abi exports.
- **Handler matching and RTTI** become libc++abi's, so libycxx's own rtti tests would measure
  Apple's behaviour.
- **GCC on Darwin with libc++abi's `__gxx_personality_v0`** worked for these programs, but that
  pairing is unusual and would need the full suite.

**Verdict:** this is feasible as an option, through a small shim library in place of
libycxx-abi.a. The coupling is narrow: five `ycxx::abi::` hooks and the `std::` exception classes'
identity. The price is that libycxx's exception classes and RTTI stop being its own on that
configuration. The unexport fix above is the smaller change for the default build.
