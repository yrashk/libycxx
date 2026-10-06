# lit configuration: run the libstdc++ testsuite against libycxx (run only; never edited).
#   tools/run-conformance libstdcxx gcc|clang [subdir...]
import os, sys

repo = lit_config.params['repo']
compiler = lit_config.params.get('compiler', 'gcc')
tests_root = lit_config.params.get('tests', '/opt/src/libstdcxx-testsuite')
sanitizer = lit_config.params.get('sanitizer', '')
# The libycxx build to link (tools/ycxx-cxx --libdir); a sanitizer run's is instrumented with the
# same sanitizers (tools/run-conformance; DECISIONS §6.8).
libdir = lit_config.params.get('libdir', '')

config.name = f'libycxx-libstdcxx-{compiler}'
config.test_source_root = tests_root
config.test_exec_root = os.path.join(repo, 'build', f'lit-libstdcxx-{compiler}' + (f'-{sanitizer.replace(",", "-")}' if sanitizer else ''))
config.suffixes = ['.cc']
# Directories that test GNU extensions, TS's, ABI or tooling rather than the standard.
config.excludes = ['ext', 'tr1', 'tr2', 'backward', 'experimental', 'decimal', 'abi', 'util', 'data',
                   'lib', 'config', 'libstdc++-abi', 'libstdc++-dg', 'libstdc++-prettyprinters',
                   'libstdc++-xmethods', 'performance']

sys.path.insert(0, os.path.join(repo, 'tests'))
from ycxxlit.libstdcxx_format import LibstdcxxFormat, build_support_lib

# Configuration macros of the libstdc++ build that some tests branch on; both features exist here.
flags = ['-I' + os.path.join(tests_root, 'util'), '-I' + os.path.join(repo, 'tests', 'libstdcxx', 'shim'),
         '-O2', '-D_GLIBCXX_USE_CHAR8_T=1', '-D_GLIBCXX_USE_WCHAR_T=1',
         # libstdc++ is configured with the new (C++11) ABI by default, so its testsuite runs with
         # _GLIBCXX_USE_CXX11_ABI=1; 64 tests branch on it (e.g. ios_base::failure deriving from
         # system_error), and the new-ABI branch is the one that describes the standard.
         '-D_GLIBCXX_USE_CXX11_ABI=1', '-w', '-fdiagnostics-color=never' if compiler == 'gcc' else '-fno-diagnostics-color']
from ycxxlit import sanitizers
sanitizer_list = sanitizers.parse(sanitizer)
flags += sanitizers.compile_flags(sanitizer_list)
if libdir:
    flags = ['--libdir=' + libdir] + flags
wrapper = os.path.join(repo, 'tools', 'ycxx-cxx')
# The testsuite's support library (DejaGnu's libtestc++.a): the helpers' out-of-line definitions.
support_lib = build_support_lib(wrapper, compiler, flags, tests_root, repo, config.test_exec_root)
# dg-require-namedlocale: the names libycxx accepts (tests/ycxxlit/locales.py), asked of the
# library under test through a program built once per run.
from ycxxlit import locales
locale_probe = locales.build_probe(wrapper, compiler, config.test_exec_root)
# Journaled: every finished test's result is kept even if the run is stopped (Ctrl-C).
from ycxxlit.journal import Journaled
# A sanitizer run: each program runs with the sanitizers' options (TSAN_OPTIONS: this suite's
# tests/libstdcxx/tsan.supp, each suppression with its reason; tests/ycxxlit/sanitizers.py).
config.test_format = Journaled(LibstdcxxFormat(wrapper, compiler, flags,
                                               os.path.join(repo, 'tests', 'libstdcxx', 'skip.txt'), locale_probe,
                                               support_lib, sanitizer_list,
                                               sanitizers.run_env(sanitizer_list, os.path.join(repo, 'tests', 'libstdcxx'))))
