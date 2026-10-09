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
#   sanitizer=<list>   asan, ubsan, tsan (comma-separated): compile the tests with them
#   libdir=<dir>       the libycxx build to link (tools/ycxx-cxx --libdir): for a sanitizer run,
#                      tools/run-conformance passes the build instrumented with the same
#                      sanitizers, build/<compiler>-<sanitizers> (DECISIONS §6.8)
import os, platform, shlex, subprocess, sys

repo = lit_config.params['repo']
compiler = lit_config.params.get('compiler', 'clang')
sanitizer = lit_config.params.get('sanitizer', '')
libdir = lit_config.params.get('libdir', '')
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
from ycxxlit import sanitizers
flags += sanitizers.compile_flags(sanitizers.parse(sanitizer))
if libdir:
    flags = ['--libdir=' + libdir] + flags
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

# A negative test of extended floating-point arguments needs the actual type. A compiler's
# absence of that type is not evidence that the library rejected an extended-type operation.
wrapper = 'ref-cxx' if reference else 'ycxx-cxx'
macro_probe = subprocess.run([os.path.join(repo, 'tools', wrapper), compiler, '-dM', '-E',
                              '-x', 'c++', '-'] + flags,
                             input='#include <stdfloat>\n', capture_output=True, text=True, timeout=30)
if macro_probe.returncode:
    lit_config.fatal('cannot detect extended floating-point types:\n' + macro_probe.stderr)
if '__STDCPP_FLOAT32_T__' in macro_probe.stdout:
    features.add('extended-float32')
config.available_features = features

# The sanitizers' options, set on each test program's command line (its transcript): for
# ThreadSanitizer the suppressions (tests/ycxx/tsan.supp, each with its reason) and
# allocator_may_return_null=1 (tests/ycxxlit/sanitizers.py).
run_env = sanitizers.run_env(sanitizers.parse(sanitizer), os.path.join(repo, 'tests', 'ycxx'))

# Journaled: every finished test's result is kept even if the run is stopped (Ctrl-C).
from ycxxlit.journal import Journaled
config.test_format = Journaled(YcxxFormat(os.path.join(repo, 'tools', wrapper), compiler, flags,
                                          sanitizer.split(',') if sanitizer else (), features, run_env))
