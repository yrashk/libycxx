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

config.name = f'libycxx-libcxx-{compiler}'
config.test_source_root = os.path.join(tests_root, 'std')
config.test_exec_root = os.path.join(repo, 'build', f'lit-libcxx-{compiler}' + (f'-{sanitizer}' if sanitizer else ''))
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
    'target=x86_64-pc-linux-gnu', 'linux', 'has-unix-headers', 'has-64-bit-atomics',
    # Atomics of any size work without libatomic (lock-based in libycxx's runtime).
    'has-1024-bit-atomics',
    'stdlib=libycxx', 'can-create-symlinks', 'has-fblocks-off',
}
if compiler == 'clang':
    features |= {'verify-support', 'clang-diagnostics', 'has-fconstexpr-steps'}
else:
    features |= {'gcc-style-warnings'}
if sanitizer:
    for s in sanitizer.split(','):
        features.add({'asan': 'asan', 'ubsan': 'ubsan'}[s])
config.available_features = features

base_flags = ['-I' + support, '-D_LIBCPP_DISABLE_DEPRECATION_WARNINGS', '-fno-diagnostics-color',
              '-Wno-deprecated-declarations', '-Wno-unused-command-line-argument'] if compiler == 'clang' else \
             ['-I' + support, '-fdiagnostics-color=never', '-Wno-deprecated-declarations']
if sanitizer:
    base_flags += ['-fsanitize=' + ('address' if 'asan' in sanitizer else '') +
                   (',' if 'asan' in sanitizer and 'ubsan' in sanitizer else '') +
                   ('undefined' if 'ubsan' in sanitizer else ''), '-fno-sanitize-recover=all', '-g']
    base_flags = [f for f in base_flags if f != '-fsanitize=']

import sys
sys.path.insert(0, os.path.join(repo, 'tests'))
from ycxxlit.libcxx_format import LibcxxFormat
config.test_format = LibcxxFormat(wrapper, compiler, base_flags, config.available_features,
                                  os.path.join(repo, 'tests', 'libcxx', 'skip.txt'))
