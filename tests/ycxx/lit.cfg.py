# libycxx's own conformance tests, written only from the working draft and cppreference.com.
#   tools/run-conformance ycxx gcc|clang [subdir...]
# With YCXX_STDLIB=libstdcxx (lit param stdlib=libstdcxx) the same tests build against the
# toolchain's libstdc++ instead (tools/ref-cxx): the reference runs of tests/ycxx/REFERENCE.md.
import os, sys

repo = lit_config.params['repo']
compiler = lit_config.params.get('compiler', 'clang')
sanitizer = lit_config.params.get('sanitizer', '')
stdlib = lit_config.params.get('stdlib', 'ycxx')
if stdlib not in ('ycxx', 'libstdcxx'):
    lit_config.fatal(f'unknown stdlib {stdlib!r}')
reference = stdlib == 'libstdcxx'

config.name = (f'libstdcxx-ref-{compiler}' if reference else f'libycxx-own-{compiler}')
config.test_source_root = os.path.join(repo, 'tests', 'ycxx')
config.test_exec_root = os.path.join(repo, 'build', ('lit-ref-libstdcxx-' if reference else 'lit-ycxx-') + compiler +
                                     (f'-{sanitizer.replace(",", "-")}' if sanitizer else ''))
config.suffixes = ['.cpp']
config.excludes = ['support']

sys.path.insert(0, os.path.join(repo, 'tests'))
from ycxxlit.ycxx_format import YcxxFormat

flags = ['-I' + os.path.join(repo, 'tests', 'ycxx', 'support'), '-Wall', '-Wextra']
if sanitizer:
    flags += ['-fsanitize=' + ','.join({'asan': 'address', 'ubsan': 'undefined', 'tsan': 'thread'}[s] for s in sanitizer.split(',')),
              '-fno-sanitize-recover=all', '-g']
    if 'tsan' in sanitizer.split(','):
        # The TSan runtime defines __cxa_guard_* too (interceptors); keep its definitions. Link
        # against a runtime built with -fsanitize=thread (YCXX_LIBDIR, see tools/ycxx-cxx) so
        # that its atomics are seen.
        flags += ['-Wl,--allow-multiple-definition']
wrapper = 'ref-cxx' if reference else 'ycxx-cxx'
config.test_format = YcxxFormat(os.path.join(repo, 'tools', wrapper), compiler, flags,
                                sanitizer.split(',') if sanitizer else ())
