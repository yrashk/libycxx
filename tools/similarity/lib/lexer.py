"""A small, dependency-free C/C++ lexer used by every metric in this audit.

It is deliberately approximate (no real preprocessing), but it is applied in
exactly the same way to every implementation, which is what matters for a
comparative measurement.

Token kinds: 'id', 'kw', 'num', 'str', 'chr', 'punct', 'comment', 'pp'
(a whole preprocessor directive line, continuation lines included).
"""
from __future__ import annotations

import re
from dataclasses import dataclass

KEYWORDS = frozenset(
    """alignas alignof and and_eq asm auto bitand bitor bool break case catch char
    char8_t char16_t char32_t class compl concept const consteval constexpr constinit
    const_cast continue co_await co_return co_yield decltype default delete do double
    dynamic_cast else enum explicit export extern false float for friend goto if inline
    int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private
    protected public register reinterpret_cast requires return short signed sizeof static
    static_assert static_cast struct switch template this thread_local throw true try
    typedef typeid typename union unsigned using virtual void volatile wchar_t while xor
    xor_eq final override import module pre post contract_assert""".split()
)

_RAW = r'(?:u8|u|U|L)?R"(?P<rd>[^()\\\s]{0,16})\((?:.|\n)*?\)(?P=rd)"'
_STR = r'(?:u8|u|U|L)?"(?:[^"\\\n]|\\.|\\\n)*"'
_CHR = r"(?:u8|u|U|L)?'(?:[^'\\\n]|\\.)+'"
_NUM = r"\.?[0-9](?:'?[0-9a-zA-Z_]|[eEpP][+-]|\.)*"
_ID = r"[A-Za-z_$][A-Za-z0-9_$]*"
PUNCT_RE = (
    r"<=>|->\*|\.\.\.|<<=|>>=|::|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|/=|%=|&=|\|=|\^=|\.\*|##|"
    r"[{}\[\]()<>;:,.?~!%^&*+\-=|/#@\\]"
)
_TOKEN_RE = re.compile(
    "|".join(
        [
            r"(?P<lc>//(?:[^\n\\]|\\.|\\\n)*)",
            r"(?P<bc>/\*(?:.|\n)*?\*/)",
            r"(?P<raw>" + _RAW + ")",
            r"(?P<str>" + _STR + ")",
            r"(?P<chr>" + _CHR + ")",
            r"(?P<num>" + _NUM + ")",
            r"(?P<id>" + _ID + ")",
            r"(?P<punct>" + PUNCT_RE + ")",
            r"(?P<nl>\n)",
            r"(?P<ws>[ \t\r\f\v]+)",
            r"(?P<other>.)",
        ]
    )
)


@dataclass(slots=True)
class Tok:
    kind: str
    text: str
    line: int  # 1-based line of the first character
    off: int = 0  # character offset in the source


def lex(src: str) -> list[Tok]:
    """Tokenise C/C++ text. Preprocessor directives become a single 'pp' token."""
    out: list[Tok] = []
    line = 1
    at_line_start = True
    pos = 0
    n = len(src)
    while pos < n:
        if at_line_start:
            # preprocessor directive?
            m = re.match(r"[ \t]*#", src[pos:pos + 256])
            if m:
                end = pos
                # consume until an unescaped newline, skipping comments
                while end < n:
                    c = src[end]
                    if c == "\\" and end + 1 < n and src[end + 1] == "\n":
                        end += 2
                        continue
                    if src.startswith("/*", end):
                        e = src.find("*/", end + 2)
                        end = n if e < 0 else e + 2
                        continue
                    if src.startswith("//", end):
                        e = src.find("\n", end)
                        end = n if e < 0 else e
                        continue
                    if c == "\n":
                        break
                    end += 1
                text = src[pos:end]
                out.append(Tok("pp", text, line, pos))
                line += text.count("\n")
                pos = end
                continue
        m = _TOKEN_RE.match(src, pos)
        kind = m.lastgroup
        text = m.group(kind)
        if kind == "nl":
            line += 1
            at_line_start = True
        elif kind == "ws":
            pass
        else:
            at_line_start = False
            if kind in ("lc", "bc"):
                out.append(Tok("comment", text, line, pos))
            elif kind == "raw":
                out.append(Tok("str", text, line, pos))
            elif kind == "id":
                out.append(Tok("kw" if text in KEYWORDS else "id", text, line, pos))
            elif kind == "other":
                pass
            else:
                out.append(Tok(kind, text, line, pos))
            nlc = text.count("\n")
            if nlc:
                line += nlc
        pos = m.end()
    return out


# ---------------------------------------------------------------------------
# Normalisation shared by all implementations

# Configuration and attribute macros: every implementation spells them as reserved names in
# capitals (export, visibility, constexpr-since, nodiscard and namespace-version macros, the
# assertion macros). They carry no structure, so they are dropped, with their parenthesised
# arguments, from every implementation alike. The rule is a spelling class, not a list of names:
# a reserved identifier with no lower-case letter that is not a predefined __X__ macro.
CONFIG_MACRO_RE = re.compile(r"^_(?:_?[A-Z])[A-Z0-9_]{2,}$")


def is_config_macro(name: str) -> bool:
    return bool(CONFIG_MACRO_RE.match(name)) and not (name.startswith("__") and name.endswith("__"))


# Names whose spelling is forced on every implementation by the compiler, the language, a
# platform or a published ABI: compiler builtins and predicates, predefined macros, C keywords,
# the Itanium C++ ABI's runtime interface and type_info classes, the unwinder interface, and
# compiler intrinsics. Again spelling classes (prefixes), not names taken from an implementation.
BUILTIN_RE = re.compile(
    r"^(?:__builtin_\w*|__has_\w*|__atomic_\w*|__sync_\w*|__c11_atomic\w*|__[A-Za-z0-9_]+__|"
    r"_(?:Atomic|Bool|Complex|Imaginary|Noreturn|Alignas|Alignof|Static_assert|Thread_local|Generic|Pragma|BitInt)|"
    r"_Float\d+x?|__float\d+|__bf16|__ibm128|__int\d+|__restrict\w*|__cpp_\w*|__cplusplus|__func__|"
    r"__cxa_\w*|__gxx_\w*|__gcc_\w*|_Unwind_\w*|_URC_\w*|_UA_\w*|__cxxabiv1|__dynamic_cast|__dso_handle|"
    r"__\w*_type_info|__\w*_mask|__flags|__base_count|__base_info|__offset_flags|__offset_shift|__pointee|"
    r"__context|__errno\w*|__declspec|__cdecl|__stdcall|__fastcall|__vectorcall|__thiscall|__forceinline|"
    r"__assume|__debugbreak|__fastfail|_Interlocked\w*|_BitScan\w*|__popcnt\w*|__lzcnt\w*|_tzcnt\w*|_lzcnt\w*|"
    r"_mm\w*|__m\d+\w*|_umul\w*|_mul\w*|__umulh|__mulh|_addcarry\w*|_subborrow\w*|__shiftleft\w*|"
    r"__shiftright\w*|_rotl\w*|_rotr\w*|_byteswap\w*|__iso_volatile\w*|__dmb|__yield|__ldrex\w*|__strex\w*|"
    r"_ReadWriteBarrier|__nop|_Exit|_exit|__label__)$"
)

# Generic one-word internal names (namespaces and the like) that carry no signal.
GENERIC_CORES = frozenset("""detail details impl internal priv private aux base helper helpers util utils
    tmp temp ret res result data value val ptr buf buffer size len count index idx node nodes first last
    begin end next prev left right lhs rhs other self state storage frames""".split())


def norm_ident(name: str) -> str:
    """Reduce an identifier to its 'core' so that the reserved-name conventions
    of the four libraries compare equal: __foo_, _M_foo, _S_foo, _Foo, _Myfoo -> foo."""
    s = name
    s = re.sub(r"^_+", "", s)
    s = re.sub(r"_+$", "", s)
    s = re.sub(r"^(?:M|S)_(?=[A-Za-z0-9])", "", s)  # libstdc++ member / static prefixes
    s = re.sub(r"^My(?=[a-z])", "", s)  # MSVC STL member prefix
    return s.lower()


def is_reserved(name: str) -> bool:
    return name.startswith("__") or bool(re.match(r"^_[A-Z]", name))


# The style control (docs/similarity/METHOD.md): with SIM_STYLE=1 the normalisation also drops
# what is style rather than content, for every library alike: the specifiers constexpr,
# consteval, constinit, inline and explicit, noexcept with its condition, typename (and class in a
# template-parameter position), and every qualification (a name followed by ::, a leading ::, and
# this->).
STYLE_DROP = frozenset({"constexpr", "consteval", "constinit", "inline", "explicit", "typename"})


def _style() -> bool:
    import os
    return os.environ.get("SIM_STYLE") == "1"


def _noise_mask(toks: list[Tok]) -> list[bool]:
    """True for tokens removed by normalisation: comments, directives, configuration macros
    (with their parenthesised arguments), attribute-specifiers [[...]] and __attribute__((...)).
    Applied identically to every implementation."""
    n = len(toks)
    drop = [False] * n

    def skip_group(j: int, open_: str, close: str) -> int:
        depth = 0
        while j < n:
            if toks[j].text == open_:
                depth += 1
            elif toks[j].text == close:
                depth -= 1
                if depth == 0:
                    return j
            j += 1
        return n - 1

    i = 0
    while i < n:
        t = toks[i]
        if t.kind in ("comment", "pp"):
            drop[i] = True
        elif t.kind == "id" and (is_config_macro(t.text) or t.text in ("__attribute__", "__declspec")):
            drop[i] = True
            if i + 1 < n and toks[i + 1].text == "(":
                e = skip_group(i + 1, "(", ")")
                for k in range(i + 1, e + 1):
                    drop[k] = True
                i = e
        elif t.text == "[" and i + 1 < n and toks[i + 1].text == "[":
            # attribute-specifier-seq [[ ... ]]
            depth, j = 0, i
            while j < n:
                if toks[j].text == "[":
                    depth += 1
                elif toks[j].text == "]":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            if j + 1 < n and toks[j].text == "]" and toks[j - 1].text == "]":
                for k in range(i, j + 1):
                    drop[k] = True
                i = j
        i += 1
    if _style():
        for i, t in enumerate(toks):
            if drop[i]:
                continue
            if t.kind == "kw" and t.text in STYLE_DROP:
                drop[i] = True
            elif t.kind == "kw" and t.text == "class" and i > 0 and toks[i - 1].text in ("<", ","):
                drop[i] = True
            elif t.kind == "kw" and t.text == "noexcept":
                drop[i] = True
                if i + 1 < n and toks[i + 1].text == "(":
                    for k in range(i + 1, skip_group(i + 1, "(", ")") + 1):
                        drop[k] = True
            elif t.text == "::":
                drop[i] = True
                if i > 0 and toks[i - 1].kind == "id":
                    drop[i - 1] = True
                elif i > 0 and toks[i - 1].text == ">":
                    # Qualifier ending in a template argument list: drop it back to its name.
                    depth, k = 0, i - 1
                    while k >= 0:
                        depth += toks[k].text == ">"
                        depth += 2 * (toks[k].text == ">>")
                        depth -= toks[k].text == "<"
                        if depth <= 0:
                            break
                        k -= 1
                    if k > 0 and toks[k - 1].kind == "id":
                        for m in range(k - 1, i):
                            drop[m] = True
            elif t.kind == "kw" and t.text == "this" and i + 1 < n and toks[i + 1].text == "->":
                drop[i] = drop[i + 1] = True
    return drop


def code_tokens(toks: list[Tok]) -> list[Tok]:
    """Drop comments, directives, configuration macros and attributes."""
    m = _noise_mask(toks)
    return [t for t, d in zip(toks, m) if not d]


def structural(toks: list[Tok]) -> list[str]:
    """JPlag-like abstraction: identifiers and literals lose their spelling."""
    out = []
    for t in toks:
        if t.kind == "id":
            out.append("I")
        elif t.kind == "num":
            out.append("N")
        elif t.kind in ("str", "chr"):
            out.append("S")
        else:
            out.append(t.text)
    return out


def template_params(toks: list[Tok]) -> set[str]:
    """Names declared as template type parameters (class X / typename X followed by , > = ...).
    Every implementation spells these with its own reserved convention,
    so they are collapsed to one placeholder in the lexical stream."""
    out = set()
    n = len(toks)
    for i, t in enumerate(toks):
        if t.kind == "kw" and t.text in ("class", "typename"):
            j = i + 1
            if j < n and toks[j].text == "...":
                j += 1
            if j + 1 < n and toks[j].kind == "id" and toks[j + 1].text in (",", ">", "=", ">>"):
                out.add(toks[j].text)
    return out


def lexical(toks: list[Tok], tparams: set[str] | None = None) -> list[str]:
    """Spelling-preserving stream, with reserved-name conventions normalised."""
    if tparams is None:
        tparams = template_params(toks)
    out = []
    for t in toks:
        if t.kind == "id":
            out.append("_T" if t.text in tparams else norm_ident(t.text))
        elif t.kind == "num":
            out.append(t.text.lower().rstrip("ul"))
        else:
            out.append(t.text)
    return out


def blank_noncode(src: str) -> str:
    """Return src with everything code_tokens() drops (comments, directives, configuration
    macros, attributes) replaced by spaces, keeping every newline so line numbers stay valid.
    Used to feed JPlag, whose C/C++ scanner also cannot read digit separators or raw strings,
    so those are neutralised the same way for every implementation."""
    chars = list(src)
    toks = lex(src)
    for t, d in zip(toks, _noise_mask(toks)):
        if d:
            for k in range(t.off, t.off + len(t.text)):
                if chars[k] != "\n":
                    chars[k] = " "
        elif t.kind == "num" and "'" in t.text:
            for k in range(t.off, t.off + len(t.text)):
                if chars[k] == "'":
                    chars[k] = "0"
        elif t.kind == "str" and (t.text.find('R"') in (0, 1, 2) or "\n" in t.text):
            body = [c if c == "\n" else " " for c in t.text]
            body[0], body[1] = '"', '"'
            chars[t.off:t.off + len(t.text)] = body
    return "".join(chars)
