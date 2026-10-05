# libycxx's own conformance tests, written only from the working draft and cppreference.com.
#   tools/run-conformance ycxx gcc|clang [subdir...]
# With YCXX_STDLIB=libstdcxx (lit param stdlib=libstdcxx) the same tests build against the
# toolchain's libstdc++ instead (tools/ref-cxx): the reference runs of tests/ycxx/REFERENCE.md.
# Configurations (each with its own exec root, log and baseline, named by tools/run-conformance):
#   hardened=1         compile every test with -DYCXX_HARDENED=1 (run-time precondition checks;
#                      lit feature "hardened", which the death tests in precondition/ require)
#   cxxflags="..."     extra compiler flags appended to every test's, e.g. -fno-exceptions (lit
#                      feature "exceptions" is then absent), -fno-rtti ("rtti" absent), -O2
#   config=<name>      the short name of that configuration (exec root and log suffix)
import os, platform, shlex, sys

repo = lit_config.params['repo']
compiler = lit_config.params.get('compiler', 'clang')
sanitizer = lit_config.params.get('sanitizer', '')
stdlib = lit_config.params.get('stdlib', 'ycxx')
hardened = lit_config.params.get('hardened', '0') not in ('', '0', 'false', 'no', 'off')
cxxflags = shlex.split(lit_config.params.get('cxxflags', ''))
config_name = lit_config.params.get('config', '')
if stdlib not in ('ycxx', 'libstdcxx'):
    lit_config.fatal(f'unknown stdlib {stdlib!r}')
if cxxflags and not config_name:
    lit_config.fatal('cxxflags needs a configuration name (lit param config=<name>; tools/run-conformance names one)')
reference = stdlib == 'libstdcxx'

config.name = (f'libstdcxx-ref-{compiler}' if reference else f'libycxx-own-{compiler}')
config.test_source_root = os.path.join(repo, 'tests', 'ycxx')
config.test_exec_root = os.path.join(repo, 'build', ('lit-ref-libstdcxx-' if reference else 'lit-ycxx-') + compiler +
                                     (f'-{sanitizer.replace(",", "-")}' if sanitizer else '') +
                                     ('-hardened' if hardened else '') + (f'-{config_name}' if config_name else ''))
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
if hardened:
    flags += ['-DYCXX_HARDENED=1']
flags += cxxflags

# Lit features for `// REQUIRES:` (tests/ycxxlit/ycxx_format.py). The last of -fexceptions /
# -fno-exceptions (-frtti / -fno-rtti) wins, as on the compiler's command line.
def enabled(on, off):
    last = [f for f in cxxflags if f in (on, off)]
    return not last or last[-1] == on

features = {compiler, platform.system().lower()} | set(sanitizer.split(',') if sanitizer else ())
if hardened:
    features.add('hardened')
if enabled('-fexceptions', '-fno-exceptions'):
    features.add('exceptions')
if enabled('-frtti', '-fno-rtti'):
    features.add('rtti')
config.available_features = features

wrapper = 'ref-cxx' if reference else 'ycxx-cxx'
config.test_format = YcxxFormat(os.path.join(repo, 'tools', wrapper), compiler, flags,
                                sanitizer.split(',') if sanitizer else (), features)
