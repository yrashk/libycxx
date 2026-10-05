"""Which named locales a test can use, for the suites' locale requirements (libstdc++'s
dg-require-namedlocale, libc++'s locale.<name> lit features).

A name is usable when the C library has it (setlocale, probed in a child process so that the
probe cannot change the harness's own locale) and libycxx accepts it: `std::locale(name)` does
not throw in a program built with the compiler under test. [locale.cons]/4 leaves the set of valid
names to the implementation; libycxx's is "C", "POSIX", "C.UTF-8", "" and every name the C
library's newlocale accepts (DECISIONS §7), so the second check fails only when the library is
broken or built without that support, and the test is then reported UNSUPPORTED with that reason.
Both answers are cached per name and lit process. CI generates the locales the suites name
(tools/ci/gen-locales); a machine without them reports the tests UNSUPPORTED.
"""
import functools, os, re, subprocess, sys

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


# Why a test needing a locale the C library has cannot run.
UNSUPPORTED_BY_LIBYCXX = ('the C library has it, but std::locale of the libycxx under test rejects it '
                          '(DECISIONS §7: every name newlocale accepts is valid)')

_ACCEPTS_SRC = r'''#include <locale>
int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    try {
      std::locale l(argv[i]);
    } catch (...) {
      return 1;
    }
  }
  return 0;
}
'''


def build_probe(wrapper, compiler, directory):
    """Builds, in `directory`, a program against libycxx that exits 0 iff std::locale accepts all
    its arguments; returns its path, or None if it cannot be built. Called once per lit run, by
    the suite's lit.cfg.py, so the program is built from the current library."""
    os.makedirs(directory, exist_ok=True)
    src, exe = os.path.join(directory, 'locale_probe.cpp'), os.path.join(directory, 'locale_probe')
    with open(src, 'w') as f:
        f.write(_ACCEPTS_SRC)
    try:
        r = subprocess.run([wrapper, compiler, src, '-o', exe], capture_output=True, timeout=300)
    except (OSError, subprocess.TimeoutExpired):
        return None
    return exe if r.returncode == 0 else None


@functools.lru_cache(maxsize=None)
def libycxx_accepts(name, probe):
    """Whether libycxx's std::locale accepts `name`; True without a probe, so a probe that could
    not be built never hides a test."""
    if probe is None:
        return True
    try:
        return subprocess.run([probe, name], capture_output=True, timeout=30).returncode == 0
    except (OSError, subprocess.TimeoutExpired):
        return True


def usable(name, probe):
    """None if a test may use the named locale, else why not."""
    if not available(name):
        return f'named locale {name} not installed'
    if not libycxx_accepts(name, probe):
        return f'needs the named locale {name}: ' + UNSUPPORTED_BY_LIBYCXX
    return None


# libc++'s `%{LOCALE_CONV_<LOCALE>_<FIELD>}` substitutions (in ADDITIONAL_COMPILE_FLAGS, e.g.
# -DFR_THOU_SEP=%{LOCALE_CONV_FR_FR_UTF_8_THOUSANDS_SEP}): the localeconv() field of the named
# locale as a wide string literal, so the tests compare against what the C library says.
CONV_SUBST = re.compile(r'%\{LOCALE_CONV_([A-Z]{2}_[A-Z]{2})_UTF_8_([A-Z_]+)\}')

_CONV = ('import locale, sys\nlocale.setlocale(locale.LC_ALL, sys.argv[1])\n'
         'sys.stdout.write(" ".join(str(ord(c)) for c in locale.localeconv()[sys.argv[2]]))\n')


@functools.lru_cache(maxsize=None)
def conv_literal(name, field):
    """L"..." holding localeconv()[field] of locale `name` (code points as UCNs); None on error."""
    try:
        r = subprocess.run([sys.executable, '-c', _CONV, name, field], timeout=30,
                           capture_output=True, text=True)
    except (OSError, subprocess.TimeoutExpired):
        return None
    if r.returncode != 0:
        return None
    out = ''
    for c in (int(x) for x in r.stdout.split()):
        if 0x20 <= c < 0x7f and chr(c) not in '"\\':
            out += chr(c)
        else:
            out += f'\\U{c:08X}'
    return f'L"{out}"'


def substitute_conv(flag):
    """`flag` with every LOCALE_CONV substitution expanded (unchanged if the locale is missing)."""
    def one(m):
        lang, field = m.group(1), m.group(2).lower()
        v = conv_literal(f'{lang[:2].lower()}_{lang[3:]}.UTF-8', field)
        return v if v is not None else m.group(0)
    return CONV_SUBST.sub(one, flag)
