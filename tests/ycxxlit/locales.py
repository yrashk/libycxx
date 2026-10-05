"""Which named locales the machine has, for the suites' locale requirements (libstdc++'s
dg-require-namedlocale, libc++'s locale.<name> lit features).

A name is available when the C library accepts it (setlocale; the tests' expectations are the C
library's conventions, while libycxx itself accepts only the "C" names, DECISIONS §7): the answer is probed once per name and lit run, in a child process so that the
probe cannot change the harness's own locale. CI generates the locales the suites name
(.github/actions/setup-linux); a machine without them reports the tests UNSUPPORTED.
"""
import functools, re, subprocess, sys

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
