# lit configuration: run libc++'s conformance tests (libcxx/test/std) against libycxx.
#
#   Use tools/run-conformance libcxx gcc|clang [subdir...]   (sets up the suite directory)
#
# The tests are an oracle only: they are run, never modified. Skipped categories are listed
# in tests/SKIPPED.md.
import os, re, shutil, subprocess, tempfile
import lit.formats, lit.Test, lit.TestRunner
from lit.TestRunner import IntegratedTestKeywordParser, ParserKind

repo = lit_config.params['repo']
compiler = lit_config.params.get('compiler', 'clang')
tests_root = lit_config.params.get('tests', '/opt/src/llvm-project/libcxx/test')
sanitizer = lit_config.params.get('sanitizer', '')
# The libycxx build to link (tools/ycxx-cxx --libdir); a sanitizer run's is instrumented with the
# same sanitizers (tools/run-conformance; DECISIONS §6.8).
libdir = lit_config.params.get('libdir', '')

config.name = f'libycxx-libcxx-{compiler}'
config.test_source_root = os.path.join(tests_root, 'std')
config.test_exec_root = os.path.join(repo, 'build', f'lit-libcxx-{compiler}' + (f'-{sanitizer.replace(",", "-")}' if sanitizer else ''))
config.suffixes = ['.cpp']
config.excludes = ['Inputs', 'gen.py']

wrapper = os.path.join(repo, 'tools', 'ycxx-cxx')
support = os.path.join(tests_root, 'support')

def compiler_version():
    exe = {'gcc': os.environ.get('YCXX_GXX', 'g++-16'),
           'clang': os.environ.get('YCXX_CLANGXX', 'clang++-23')}[compiler]
    out = subprocess.run([exe, '-dumpfullversion' if compiler == 'gcc' else '-dumpversion'],
                         capture_output=True, text=True).stdout.strip()
    return out.split('.')

ver = compiler_version()
features = {
    'c++26', 'std-at-least-c++11', 'std-at-least-c++14', 'std-at-least-c++17',
    'std-at-least-c++20', 'std-at-least-c++23', 'std-at-least-c++26',
    compiler, f'{compiler}-{ver[0]}', f'{compiler}-{ver[0]}.{ver[1]}',
    'has-unix-headers', 'has-64-bit-atomics',
    # Atomics of any size work without libatomic (lock-based in libycxx's runtime).
    'has-1024-bit-atomics',
    'stdlib=libycxx', 'can-create-symlinks', 'has-fblocks-off',
    # libycxx is not hardened by default (YCXX_HARDENED); libc++'s assertion tests need a hardened
    # libc++ and its own messages.
    'libcpp-hardening-mode=none',
}
# The platform as libc++'s own configuration names it: the target triple (tests select on it,
# e.g. target={{.+}}-apple-{{.+}}) and linux or darwin (REQUIRES: linux, UNSUPPORTED: darwin).
import platform
_machine = {'amd64': 'x86_64'}.get(platform.machine().lower(), platform.machine().lower())
if platform.system() == 'Darwin':
    features |= {f'target={_machine}-apple-macosx{platform.mac_ver()[0]}', 'darwin', f'{compiler}-darwin'}
else:
    features |= {f'target={_machine}-pc-linux-gnu', 'linux'}
if compiler == 'clang':
    features |= {'verify-support', 'clang-diagnostics', 'has-fconstexpr-steps'}
else:
    # GCC's constexpr operation limit; the tests that need a raised one set it under this feature.
    features |= {'gcc-style-warnings', 'has-fconstexpr-ops-limit'}
# Running as root: permissions such as perms::none do not deny access, so tests that check a
# permission error cannot fail as they expect (tests/libcxx/unsupported.txt lists them).
if os.geteuid() == 0:
    features.add('root')
if sanitizer:
    for s in sanitizer.split(','):
        features.add({'asan': 'asan', 'ubsan': 'ubsan', 'tsan': 'tsan'}[s])
    # Under AddressSanitizer or ThreadSanitizer the suite's count_new.h replaces no allocation
    # function (TEST_HAS_SANITIZERS, from __has_feature: DISABLE_NEW_COUNT), so the tests that count
    # allocations or make them fail say UNSUPPORTED: sanitizer-new-delete; libc++'s own
    # configuration sets this feature for those sanitizers.
    if {'asan', 'tsan'} & set(sanitizer.split(',')):
        features.add('sanitizer-new-delete')
# Named locales the machine has and libycxx accepts (tests/ycxxlit/locales.py), and libc++'s long tests on request
# (YCXX_LONG_TESTS=1: the nightly runs).
import sys
sys.path.insert(0, os.path.join(repo, 'tests'))
from ycxxlit import locales
locale_probe = locales.build_probe(wrapper, compiler, config.test_exec_root)
features |= {f'locale.{n}' for n in locales.LIBCXX_LOCALES if locales.usable(n, locale_probe) is None}
features |= {f'missing-locale.{n}' for n in locales.LIBCXX_UNDECLARED_LOCALES
             if locales.usable(n, locale_probe) is not None}
if os.environ.get('YCXX_LONG_TESTS') == '1':
    features.add('long_tests')
config.available_features = features

base_flags = ['-I' + support, '-D_LIBCPP_DISABLE_DEPRECATION_WARNINGS', '-fno-diagnostics-color',
              '-Wno-deprecated-declarations', '-Wno-unused-command-line-argument'] if compiler == 'clang' else \
             ['-I' + support, '-fdiagnostics-color=never', '-Wno-deprecated-declarations']
from ycxxlit import sanitizers
sanitizer_list = sanitizers.parse(sanitizer)
base_flags += sanitizers.compile_flags(sanitizer_list)
if libdir:
    base_flags = ['--libdir=' + libdir] + base_flags

import sys
sys.path.insert(0, os.path.join(repo, 'tests'))
from ycxxlit.libcxx_format import LibcxxFormat
# Journaled: every finished test's result is kept even if the run is stopped (Ctrl-C).
from ycxxlit.journal import Journaled
# A sanitizer run: each program runs with the sanitizers' options (TSAN_OPTIONS: this suite's
# tests/libcxx/tsan.supp, each suppression with its reason; tests/ycxxlit/sanitizers.py).
config.test_format = Journaled(LibcxxFormat(wrapper, compiler, base_flags, config.available_features,
                                            os.path.join(repo, 'tests', 'libcxx', 'skip.txt'), sanitizer_list,
                                            sanitizers.run_env(sanitizer_list, os.path.join(repo, 'tests', 'libcxx'))))
