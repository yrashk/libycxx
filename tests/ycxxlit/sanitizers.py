"""Sanitizer settings shared by the lit configurations (tests/{ycxx,libcxx,libstdcxx}/lit.cfg.py).

A sanitizer run (lit param sanitizer=asan,ubsan,tsan) compiles every test with the sanitizers,
links it with libycxx built with the same ones (DECISIONS §6.8), and runs each program with the
sanitizers' options set on its command line, through env(1), so that every transcript shows them.
"""
import os

FLAGS = {'asan': 'address', 'ubsan': 'undefined', 'tsan': 'thread'}


def parse(param):
    """The sanitizers of the lit param sanitizer= ("asan,ubsan" -> ['asan', 'ubsan'])."""
    return [s for s in param.split(',') if s] if param else []


def compile_flags(sanitizers):
    """The compile and link flags of a run with these sanitizers (asan, ubsan, tsan)."""
    if not sanitizers:
        return []
    return ['-fsanitize=' + ','.join(FLAGS[s] for s in sanitizers), '-fno-sanitize-recover=all', '-g']


def run_env(sanitizers, suite_dir):
    """The variables set for each test program of a run with these sanitizers.

    ThreadSanitizer's options follow any of the caller's own TSAN_OPTIONS:
    - suppressions=<suite_dir>/tsan.supp, when the suite has one: its false positives, each with
      its reason (a race in the library is fixed, never suppressed);
    - allocator_may_return_null=1: malloc returns null for a size it cannot serve, as C requires,
      instead of ending the program. libycxx's operator new (which serves the program under TSan:
      tools/ycxx-cxx) then calls the new_handler and throws bad_alloc, which tests check.
    """
    env = {}
    if 'tsan' in sanitizers:
        supp = os.path.join(suite_dir, 'tsan.supp')
        env['TSAN_OPTIONS'] = ':'.join(o for o in (os.environ.get('TSAN_OPTIONS', ''),
                                                   'suppressions=' + supp if os.path.exists(supp) else '',
                                                   'allocator_may_return_null=1') if o)
    return env


def run_prefix(env):
    """The command prefix that sets env for a test program: env(1) and the assignments."""
    return ['env'] + [f'{k}={v}' for k, v in env.items()] if env else []


def run_timeout(sanitizers, plain):
    """A test program's time limit: three times the plain one under a sanitizer (ThreadSanitizer
    slows programs down 5-15 times and adds a start-up to every process)."""
    return plain * 3 if sanitizers else plain
