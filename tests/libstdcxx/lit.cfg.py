# lit configuration: run the libstdc++ testsuite against libycxx (run only; never edited).
#   tools/run-conformance libstdcxx gcc|clang [subdir...]
import os, sys

repo = lit_config.params['repo']
compiler = lit_config.params.get('compiler', 'gcc')
tests_root = lit_config.params.get('tests', '/opt/src/libstdcxx-testsuite')
sanitizer = lit_config.params.get('sanitizer', '')

config.name = f'libycxx-libstdcxx-{compiler}'
config.test_source_root = tests_root
config.test_exec_root = os.path.join(repo, 'build', f'lit-libstdcxx-{compiler}' + (f'-{sanitizer}' if sanitizer else ''))
config.suffixes = ['.cc']
# Directories that test GNU extensions, TS's, ABI or tooling rather than the standard.
config.excludes = ['ext', 'tr1', 'tr2', 'backward', 'experimental', 'decimal', 'abi', 'util', 'data',
                   'lib', 'config', 'libstdc++-abi', 'libstdc++-dg', 'libstdc++-prettyprinters',
                   'libstdc++-xmethods', 'performance']

sys.path.insert(0, os.path.join(repo, 'tests'))
from ycxxlit.libstdcxx_format import LibstdcxxFormat

flags = ['-I' + os.path.join(tests_root, 'util'), '-I' + os.path.join(repo, 'tests', 'libstdcxx', 'shim'),
         '-O2', '-w', '-fdiagnostics-color=never' if compiler == 'gcc' else '-fno-diagnostics-color']
if sanitizer:
    flags += ['-fsanitize=' + ','.join({'asan': 'address', 'ubsan': 'undefined'}[s] for s in sanitizer.split(',')),
              '-fno-sanitize-recover=all', '-g']
config.test_format = LibstdcxxFormat(os.path.join(repo, 'tools', 'ycxx-cxx'), compiler, flags,
                                     os.path.join(repo, 'tests', 'libstdcxx', 'skip.txt'))
