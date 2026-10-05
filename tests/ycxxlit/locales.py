"""Which named locales the machine has, for the suites' locale requirements (libstdc++'s
dg-require-namedlocale, libc++'s locale.<name> lit features).

A name is available when the C library accepts it (setlocale, as libycxx's named locales are the
C library's): the answer is probed once per name and lit run, in a child process so that the
probe cannot change the harness's own locale. CI generates the locales the suites name
(.github/actions/setup-linux); a machine without them reports the tests UNSUPPORTED.
"""
import functools, subprocess, sys

# The locales libc++'s tests require as lit features (`locale.<name>`).
LIBCXX_LOCALES = ['en_US.UTF-8', 'fr_FR.UTF-8', 'ja_JP.UTF-8', 'ru_RU.UTF-8', 'zh_CN.UTF-8',
                  'fr_CA.ISO8859-1', 'cs_CZ.ISO8859-2']

_PROBE = 'import locale, sys\ntry:\n    locale.setlocale(locale.LC_ALL, sys.argv[1])\nexcept locale.Error:\n    sys.exit(1)\n'


@functools.lru_cache(maxsize=None)
def available(name):
    try:
        return subprocess.run([sys.executable, '-c', _PROBE, name], timeout=30).returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return False
