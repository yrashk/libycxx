"""Split the draft's synopses and class definitions into declarations (tools/spec_audit).

`declarations(text)` yields Decl records for one code block of the draft as extract_draft.py
wrote it (italic runs, exposition-only names, wrapped in @...@): the namespace and class it sits
in, its kind, its declared name, its tokens and the comment that follows it on its line (the
draft's `// freestanding`, `// hosted`, `// freestanding-deleted` marks).
"""
import re

_TOK = re.compile(r"""
    (?P<ws>\s+)
  | (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<pp>\#[ \t]*\w+[^\n]*)
  | (?P<ital>@[^@]*@)
  | (?P<str>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')
  | (?P<num>\d[\w.']*)
  | (?P<id>[A-Za-z_]\w*)
  | (?P<op>::|->\*|->|\.\.\.|<=>|<<=|>>=|[-+*/%^&|]=|&&|\|\||==|!=|<=|>=|\+\+|--|<<|[-+*/%^&|~!=<>?:;,.(){}\[\]])
""", re.S | re.X)


class Tok:
    __slots__ = ('kind', 'text', 'line')

    def __init__(self, kind, text, line):
        self.kind, self.text, self.line = kind, text, line

    def __repr__(self):
        return self.text


def tokenize(text):
    toks, line = [], 1
    pos = 0
    while pos < len(text):
        m = _TOK.match(text, pos)
        if not m:
            pos += 1
            continue
        k = m.lastgroup
        s = m.group()
        if k == 'ital':
            # an exposition-only name, or a placeholder (`see below`, `unspecified`)
            inner = s[1:-1]
            if inner.strip():
                toks.append(Tok('expo', inner, line))
        elif k != 'ws':
            toks.append(Tok(k, s, line))
        line += s.count('\n')
        pos = m.end()
    return toks


class Decl:
    cls_decls = ()
    has_body = False

    def __init__(self, ns, cls, toks, comment):
        self.ns = ns              # e.g. 'std::ranges'
        self.cls = cls            # enclosing class names, outermost first
        self.toks = toks
        self.comment = comment
        self.kind, self.name = classify(toks, cls)

    @property
    def text(self):
        return join(self.toks)

    def __repr__(self):
        return f'<{self.kind} {self.ns}::{"::".join(self.cls)}::{self.name}: {self.text}>'


def join(toks):
    out = ''
    prev = None
    for t in toks:
        s = t.text if t.kind != 'expo' else '@' + t.text + '@'
        if prev is not None:
            if (prev.kind in ('id', 'num', 'expo') and t.kind in ('id', 'num', 'expo')) or \
               (prev.text in (',',) ) or (t.text in ('=', '->', '&&', '||', '?', '{') or prev.text in ('=', '->', '&&', '||', '?', '{', ';')):
                out += ' '
            elif prev.text == '>' and t.kind in ('id', 'expo'):
                out += ' '
            elif prev.text in (')', ']') and t.kind in ('id', 'expo'):
                out += ' '
            elif prev.text in ('&', '*') and t.kind in ('id', 'expo') and t.text not in ('operator',):
                out += ' '
        out += s
        prev = t
    return out


def _skip_balanced(toks, i, open_, close):
    depth = 0
    while i < len(toks):
        if toks[i].text == open_:
            depth += 1
        elif toks[i].text == close:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return i


def _skip_template_args(toks, i):
    """toks[i] is '<': the index after the matching '>' (parentheses nest; >> closes two)."""
    depth = 0
    while i < len(toks):
        t = toks[i].text
        if t in ('(', '[', '{'):
            i = _skip_balanced(toks, i, t, {'(': ')', '[': ']', '{': '}'}[t])
            continue
        if t == '<':
            depth += 1
        elif t == '>':
            depth -= 1
            if depth == 0:
                return i + 1
        elif t == '>>':
            depth -= 2
            if depth <= 0:
                return i + 1
        elif t in (';',):
            return i
        i += 1
    return i


def _strip_targs(s):
    while True:
        t = re.sub(r'<[^<>]*>', '', s)
        if t == s:
            return s
        s = t


def decls_join(toks):
    return join(toks)


def strip_template_heads(toks):
    """Drop leading template<...> heads and a following requires-clause before the declaration."""
    i = 0
    while i < len(toks) and toks[i].text == 'template' and i + 1 < len(toks) and toks[i + 1].text == '<':
        i = _skip_template_args(toks, i + 1)
        if i < len(toks) and toks[i].text == 'requires':
            i = _skip_requires(toks, i + 1)
    return toks[i:]


def _skip_requires(toks, i):
    """Skip a requires-clause's constraint-logical-or-expression starting at toks[i]."""
    while i < len(toks):
        t = toks[i]
        if t.text == 'requires':
            # a requires-expression: requires (params) { requirements }
            i += 1
            if i < len(toks) and toks[i].text == '(':
                i = _skip_balanced(toks, i, '(', ')')
            if i < len(toks) and toks[i].text == '{':
                i = _skip_balanced(toks, i, '{', '}')
        elif t.text == '(':
            i = _skip_balanced(toks, i, '(', ')')
        elif t.kind in ('id', 'expo') or t.text == '::':
            i += 1
            while i < len(toks) and toks[i].text == '::':
                i += 2
            if i < len(toks) and toks[i].text == '<':
                i = _skip_template_args(toks, i)
        elif t.text == '!':
            i += 1
            continue
        else:
            return i
        if i < len(toks) and toks[i].text in ('&&', '||'):
            i += 1
            continue
        return i
    return i


def top_level_index(toks, text, start=0):
    depth = 0
    i = start
    while i < len(toks):
        t = toks[i].text
        if t in ('(', '[', '{'):
            if t == text and depth == 0:
                return i
            i = _skip_balanced(toks, i, t, {'(': ')', '[': ']', '{': '}'}[t])
            continue
        if t == '<' and i > 0 and toks[i - 1].kind in ('id', 'expo') and toks[i - 1].text != 'operator':
            if text == '<' and depth == 0:
                return i
            i = _skip_template_args(toks, i)
            continue
        if t == text:
            return i
        i += 1
    return -1


SPECIFIERS = {'constexpr', 'consteval', 'inline', 'static', 'explicit', 'virtual', 'friend', 'extern',
              'constinit', 'typename', 'mutable', 'thread_local'}


def classify(toks, cls):
    body = strip_template_heads(toks)
    if not body:
        return 'empty', ''
    texts = [t.text for t in body]
    first = texts[0]
    if first in ('public', 'private', 'protected'):
        return 'access', ''
    if first == 'using':
        if len(texts) > 2 and texts[2] == '=':
            return 'alias', texts[1]
        if 'namespace' in texts[:2]:
            return 'using-directive', ''
        # using-declaration: the last name
        names = [t.text for t in body if t.kind == 'id']
        return 'using-decl', names[-1] if names else ''
    if first == 'concept':
        return 'concept', texts[1]
    if first == 'namespace':
        return 'namespace-alias', texts[1]
    if first in ('class', 'struct', 'union'):
        # class-head: a name, maybe with template arguments (a specialization)
        j = 1
        while j < len(body) and body[j].text in ('[[', 'alignas'):
            j += 1
        name_toks = []
        while j < len(body) and (body[j].kind in ('id', 'expo') or body[j].text == '::') and body[j].text not in ('final',):
            name_toks.append(body[j].text)
            j += 1
        name = ''.join(name_toks)
        spec = j < len(body) and body[j].text == '<'
        if body[0] is not toks[0] and texts[-1:] != ['{'] and 'template' in [t.text for t in toks[:1]] and \
           len(toks) > 2 and toks[1].text == '<' and toks[2].text == '>':
            return 'class-spec', name
        if spec:
            return 'class-spec', name
        return 'class', name
    if first == 'enum':
        j = 1
        if texts[1] in ('class', 'struct'):
            j = 2
        return 'enum', texts[j] if j < len(texts) else ''
    if first == 'friend':
        k, n = classify(body[1:], cls)
        return 'friend-' + k, n
    if first == 'static_assert':
        return 'static_assert', ''
    if first == 'typedef':
        return 'alias', [t.text for t in body if t.kind == 'id'][-1]
    # a function, a deduction guide or a variable: the declarator-id before the first top-level (
    p = top_level_index(body, '(')
    while p > 0 and body[p - 1].text in ('decltype', 'noexcept', 'sizeof', 'alignas', 'explicit', 'alignof'):
        p = top_level_index(body, '(', _skip_balanced(body, p, '(', ')'))
    eq = top_level_index(body, '=')
    if 'operator' in texts:
        k = texts.index('operator')
        q = k + 1
        if q < len(body) and body[q].text in ('(', '['):
            q += 2                      # operator() / operator[]
        else:
            while q < len(body) and body[q].text != '(':
                q += 1
        name = 'operator' + ('' if body[k + 1].kind == 'op' else ' ') + ' '.join(t.text for t in body[k + 1:q]).replace(' ', '') if body[k + 1].kind == 'op' else 'operator ' + decls_join(body[k + 1:q])
        if k > 0 and body[k - 1].text == '::':
            return 'qualified-function', name
        if texts[0] == 'explicit' or k == 0 or all(t.text in SPECIFIERS for t in body[:k]):
            return 'conversion' if body[k + 1].kind != 'op' else 'function', name
        return 'function', name
    if p >= 0 and (eq < 0 or p < eq):
        # name before '('
        j = p - 1
        if j >= 0 and body[j].text == '>' :
            # a template-id, e.g. a specialization or a conversion `operator T<...>`
            depth = 0
            while j >= 0:
                if body[j].text == '>':
                    depth += 1
                elif body[j].text == '<':
                    depth -= 1
                    if depth == 0:
                        j -= 1
                        break
                j -= 1
        if j >= 0:
            # operator names
            k = j
            while k >= 0 and body[k].text != 'operator' and j - k < 4 and body[k].kind != 'id' or (k == j and body[k].kind != 'id'):
                k -= 1
            if k >= 0 and body[k].text == 'operator':
                name = 'operator' + ''.join(t.text for t in body[k + 1:p])
                return ('function', name)
            name = body[j].text
            if j >= 1 and body[j - 1].text == '~':
                return 'destructor', '~' + name
            if j >= 1 and body[j - 1].text == 'operator':
                return 'function', 'operator ' + name
            # deduction guide: Name(params) -> ...;  (no return type before the name)
            close = _skip_balanced(body, p, '(', ')')
            pre = [t.text for t in body[:j]]
            if 'explicit' in pre and '(' in pre:
                pre = pre[:pre.index('explicit')] + pre[pre.index(')') + 1:]
            pre = [t for t in pre if t != 'explicit']
            if close < len(body) and body[close].text == '->' and not pre:
                return 'deduction-guide', name
            if cls and name == _strip_targs(cls[-1]).split('::')[-1].strip('@') and not [t for t in pre if t not in SPECIFIERS]:
                return 'constructor', name
            if not pre and not cls:
                return 'function', name
            qual = j >= 2 and body[j - 1].text == '::'
            if qual:
                return 'qualified-function', ''.join(t.text for t in body[max(0, j - 2):j + 1])
            return 'function', name
    # variable: the identifier before '=' or ';' or '{' (maybe a specialization ranges::enable_view<...>)
    end = eq if eq >= 0 else len(body)
    br = top_level_index(body, '{')
    if br >= 0 and br < end:
        end = br
    j = end - 1
    while j >= 0 and body[j].text in (';', '}'):
        j -= 1
    if j >= 0 and body[j].text == '>':
        depth = 0
        while j >= 0:
            if body[j].text == '>':
                depth += 1
            elif body[j].text == '<':
                depth -= 1
                if depth == 0:
                    j -= 1
                    break
            j -= 1
        name = body[j].text if j >= 0 else ''
        q = ''
        if j >= 2 and body[j - 1].text == '::':
            q = body[j - 2].text + '::'
        return 'variable-spec', q + name
    if j >= 0 and body[j].text == ']':
        j = top_level_index(body, '[') - 1
    if j >= 0:
        return 'variable', body[j].text
    return 'other', ''


def declarations(text):
    toks = tokenize(text)
    out = []
    _parse(toks, 0, len(toks), [], [], out, [])
    return out


def _comment_after(toks, i, end):
    """The comment on the same line as toks[i-1], if one follows."""
    if i < end and toks[i].kind == 'comment' and toks[i].line == toks[i - 1].line:
        return toks[i].text
    return ''


def class_head(d):
    """The class-head-name of a class definition, with its template arguments when it is a
    specialization or a nested class defined outside (`filter_view<V, Pred>::@iterator@`)."""
    body = strip_template_heads(d.toks)
    j = 1
    out = []
    while j < len(body):
        t = body[j]
        if t.text in ('final', ':', '{') and not (t.text == ':' and False):
            break
        if t.text == '<':
            k = _skip_template_args(body, j)
            out.extend(body[j:k])
            j = k
            continue
        out.append(t)
        j += 1
    return join(out)


def _parse(toks, i, end, ns, cls, out, cls_decls=None):
    cls_decls = list(cls_decls or [])
    cur = []
    notes = []
    while i < end:
        t = toks[i]
        if t.kind == 'comment':
            i += 1
            continue
        if t.kind == 'pp':
            out.append(Decl('::'.join(ns), list(cls), [t], ''))
            out[-1].kind, out[-1].name = 'pp', t.text
            i += 1
            continue
        if t.text == '}' and not cur:
            return i + 1
        if t.text in ('public', 'private', 'protected') and i + 1 < end and toks[i + 1].text == ':' and not cur:
            i += 2
            continue
        if t.text == ';':
            if cur:
                d = Decl('::'.join(ns), list(cls), cur, ' '.join(notes + [_comment_after(toks, i + 1, end)]).strip())
                out.append(d)
            cur = []
            notes = []
            i += 1
            continue
        if t.text == '{':
            body = strip_template_heads(cur)
            texts = [x.text for x in body]
            if texts and (texts[0] == 'namespace' or texts[:2] == ['inline', 'namespace']):
                names = [x.text for x in body[1:] if x.kind == 'id' and x.text not in ('inline', 'namespace')]
                i = _parse(toks, i + 1, end, ns + ['::'.join(names)] if names else ns, cls, out)
                cur = []
                continue
            if texts and texts[0] == 'extern' and len(texts) > 1 and texts[1].startswith('"'):
                i = _parse(toks, i + 1, end, ns, cls, out)
                cur = []
                continue
            if texts and texts[0] in ('class', 'struct', 'union') and '=' not in texts and '(' not in texts[:3]:
                d = Decl('::'.join(ns), list(cls), cur, _comment_after(toks, i + 1, end))
                out.append(d)
                i = _parse(toks, i + 1, end, ns, cls + [class_head(d)], out, cls_decls + [d])
                # the class definition ends with ';' (possibly after declarators)
                while i < end and toks[i].text != ';':
                    i += 1
                i += 1
                cur = []
                continue
            if texts and texts[0] == 'enum':
                j = _skip_balanced(toks, i, '{', '}')
                cur = cur + toks[i:j]
                i = j
                continue
            # a function body, or a brace initializer / requires-expression / lambda
            is_func = top_level_index(body, '(') >= 0 and 'concept' not in texts and \
                (top_level_index(body, '=') < 0 or top_level_index(body, '(') < top_level_index(body, '=')
                 or body[top_level_index(body, '=') - 1].text == 'operator')
            j = _skip_balanced(toks, i, '{', '}')
            if is_func:
                d = Decl('::'.join(ns), list(cls), cur, _comment_after(toks, j, end))
                d.has_body = True
                out.append(d)
                cur = []
                i = j
                if i < end and toks[i].text == ';':
                    i += 1
                continue
            cur = cur + toks[i:j]
            i = j
            continue
        if t.text in ('(', '['):
            j = _skip_balanced(toks, i, t.text, ')' if t.text == '(' else ']')
            inner = toks[i:j]
            cur = cur + [x for x in inner if x.kind != 'comment']
            notes.extend(x.text for x in inner if x.kind == 'comment')
            i = j
            continue
        cur.append(t)
        i += 1
    if cur:
        out.append(Decl('::'.join(ns), list(cls), cur, ''))
    return i
