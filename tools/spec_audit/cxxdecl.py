"""Parse one draft declaration (decls.Decl) into its parts, and rewrite its types for a probe.

parse(d) gives the template parameters, the return type, the name, the parameters and the
qualifiers of a function, constructor or deduction guide. Rewriter.type() turns a type of the
draft into the C++ a probe can spell: template parameters and exposition-only names substituted,
class members named through the class's instantiation, and library names qualified.
"""
import re
import decls

KEYWORDS = {
    'const', 'volatile', 'int', 'char', 'bool', 'void', 'long', 'short', 'unsigned', 'signed', 'float',
    'double', 'wchar_t', 'char8_t', 'char16_t', 'char32_t', 'auto', 'decltype', 'typename', 'template',
    'sizeof', 'noexcept', 'true', 'false', 'nullptr', 'requires', 'operator', 'class', 'struct',
    'static_cast', 'this', 'alignof', 'nullptr_t',
}
SPECIFIERS = {'constexpr', 'consteval', 'inline', 'static', 'explicit', 'virtual', 'friend', 'extern',
              'constinit', 'mutable', 'thread_local'}


class Param:
    def __init__(self, toks):
        self.toks = toks
        eq = decls.top_level_index(toks, '=')
        self.default = toks[eq + 1:] if eq >= 0 else None
        body = toks[:eq] if eq >= 0 else toks
        self.pack = any(t.text == '...' for t in body)
        body = [t for t in body if t.text != '...']
        self.name = None
        # a declarator in parentheses: T (&a)[N]
        for k in range(len(body) - 3):
            if body[k].text == '(' and body[k + 1].text in ('&', '&&', '*') and body[k + 2].kind == 'id' and body[k + 3].text == ')':
                self.name = body[k + 2].text
                body = body[:k + 2] + body[k + 3:]
                break
        if self.name is None and len(body) >= 2 and body[-1].kind in ('id', 'expo') and body[-1].text not in KEYWORDS and \
                body[-2].text not in ('::', 'const', 'volatile', 'class', 'typename', 'struct', 'unsigned', 'signed', 'long', 'short') \
                and not (len(body) == 2 and body[0].text in ('const',)):
            self.name = body[-1].text
            body = body[:-1]
        self.type = body

    def __repr__(self):
        return f'Param({decls.join(self.type)!r}, {self.name!r}, pack={self.pack})'


class TParam:
    """A template parameter: kind tokens (class, typename, a concept, a type), name, default."""
    def __init__(self, toks):
        self.toks = toks
        eq = decls.top_level_index(toks, '=')
        self.default = toks[eq + 1:] if eq >= 0 else None
        body = toks[:eq] if eq >= 0 else toks
        self.pack = any(t.text == '...' for t in body)
        ids = [t for t in body if t.kind in ('id', 'expo') and t.text != '...']
        self.name = ids[-1].text if ids and len(body) > 1 else None
        self.kind = body[:-1] if self.name else body
        self.is_type = bool(body) and (body[0].text in ('class', 'typename') or
                                       (len(body) >= 2 and body[-2].kind in ('id', 'expo', 'op') and body[0].text not in ('size_t', 'bool', 'int', 'subrange_kind', 'auto', 'char', 'ptrdiff_t'))
                                       and not body[0].text in ('size_t', 'bool', 'int', 'subrange_kind', 'auto', 'ptrdiff_t', 'char'))

    def __repr__(self):
        return f'TParam({self.name!r}, pack={self.pack})'


def split_commas(toks):
    out, cur, i = [], [], 0
    while i < len(toks):
        t = toks[i]
        if t.text in ('(', '[', '{'):
            j = decls._skip_balanced(toks, i, t.text, {'(': ')', '[': ']', '{': '}'}[t.text])
            cur.extend(toks[i:j])
            i = j
            continue
        if t.text == '<' and i > 0 and toks[i - 1].kind in ('id', 'expo') and toks[i - 1].text != 'operator':
            j = decls._skip_template_args(toks, i)
            cur.extend(toks[i:j])
            i = j
            continue
        if t.text == ',':
            out.append(cur)
            cur = []
        else:
            cur.append(t)
        i += 1
    if cur:
        out.append(cur)
    return out


def template_heads(toks):
    """The template parameter lists in front of a declaration, outermost first."""
    heads = []
    i = 0
    while i < len(toks) and toks[i].text == 'template' and i + 1 < len(toks) and toks[i + 1].text == '<':
        j = decls._skip_template_args(toks, i + 1)
        heads.append([TParam(p) for p in split_commas(toks[i + 2:j - 1])])
        i = j
        if i < len(toks) and toks[i].text == 'requires':
            i = decls._skip_requires(toks, i + 1)
    return heads


class Func:
    pass


def parse(d):
    """Parse a function-like declaration; None when it is not one."""
    toks = [t for t in d.toks if t.kind != 'comment']
    f = Func()
    f.decl = d
    f.tparams = [p for h in template_heads(toks) for p in h]
    body = decls.strip_template_heads(toks)
    if body and body[0].text == 'friend':
        body = body[1:]
        f.friend = True
    else:
        f.friend = False
    # attributes
    while body and body[0].text == '[' and len(body) > 1 and body[1].text == '[':
        j = decls._skip_balanced(body, 0, '[', ']')
        body = body[j:]
    f.specs = set()
    i = 0
    while i < len(body) and (body[i].text in SPECIFIERS):
        f.specs.add(body[i].text)
        i += 1
        if body[i - 1].text == 'explicit' and i < len(body) and body[i].text == '(':
            i = decls._skip_balanced(body, i, '(', ')')
            f.specs.add('explicit(cond)')
    body = body[i:]
    # the declarator-id: the name before the parameter list
    if 'operator' in [t.text for t in body]:
        k = [t.text for t in body].index('operator')
        q = k + 1
        if q < len(body) and body[q].text in ('(', '['):
            q += 2
        else:
            while q < len(body) and body[q].text != '(':
                q += 1
        p = q
        f.name = 'operator' + ''.join(t.text if t.kind == 'op' else ' ' + t.text for t in body[k + 1:q])
        f.name = re.sub(r'^operator (\w)', r'operator \1', f.name)
        f.name_start = k
        f.operator = True
    else:
        p = -1
        q = 0
        while True:
            p = decls.top_level_index(body, '(', q)
            if p <= 0 or body[p - 1].text not in ('decltype', 'noexcept', 'sizeof', 'alignas', 'explicit', 'requires', 'alignof'):
                break
            q = decls._skip_balanced(body, p, '(', ')')
        if p < 0:
            return None
        k = p - 1
        if k >= 0 and body[k].text == '>':
            depth = 0
            while k >= 0:
                if body[k].text == '>':
                    depth += 1
                elif body[k].text == '<':
                    depth -= 1
                    if depth == 0:
                        k -= 1
                        break
                k -= 1
        if k < 0:
            return None
        f.name = body[k].text
        f.name_start = k
        f.operator = False
        if k >= 1 and body[k - 1].text == '~':
            f.name = '~' + f.name
            f.name_start = k - 1
    # qualified name (ranges::enable_view<...>, a member defined outside)
    f.qualifier = []
    s = f.name_start
    while s >= 2 and body[s - 1].text == '::':
        f.qualifier.insert(0, body[s - 2].text)
        s -= 2
    f.ret = body[:s]
    f.name_targs = body[f.name_start + 1:p] if not f.operator and body[p - 1].text == '>' else []
    close = decls._skip_balanced(body, p, '(', ')')
    f.params = [Param(x) for x in split_commas(body[p + 1:close - 1])]
    if len(f.params) == 1 and [t.text for t in f.params[0].toks] == ['void']:
        f.params = []
    rest = body[close:]
    f.const = f.volatile = False
    f.ref = ''
    f.noexcept = None
    f.trailing = None
    f.deleted = f.defaulted = False
    f.requires = None
    j = 0
    while j < len(rest):
        t = rest[j].text
        if t == 'const':
            f.const = True
        elif t == 'volatile':
            f.volatile = True
        elif t in ('&', '&&'):
            f.ref = t
        elif t == 'noexcept':
            if j + 1 < len(rest) and rest[j + 1].text == '(':
                e = decls._skip_balanced(rest, j + 1, '(', ')')
                f.noexcept = rest[j + 2:e - 1]
                j = e
                continue
            f.noexcept = True
        elif t == '->':
            e = j + 1
            stop = decls.top_level_index(rest, 'requires', e)
            eq = decls.top_level_index(rest, '=', e)
            ends = [x for x in (stop, eq) if x >= 0]
            end = min(ends) if ends else len(rest)
            f.trailing = rest[e:end]
            j = end
            continue
        elif t == 'requires':
            f.requires = rest[j + 1:]
            break
        elif t == '=':
            if j + 1 < len(rest):
                f.deleted = rest[j + 1].text == 'delete'
                f.defaulted = rest[j + 1].text == 'default'
            break
        elif t == ':':
            break           # a constructor's mem-initializer list
        j += 1
    return f


class Unresolved(Exception):
    pass


class Rewriter:
    """Rewrites the draft's types into a probe's.

    subst: template parameter (or exposition-only name, '@name@') -> replacement text, or a list
    of texts for a pack. members: names declared in the class (spelled INST::name). inst: the
    class's instantiation. lookup: name -> qualified name of the library (innermost namespace
    first). expo: exposition-only alias templates the prelude defines (name -> probe name).
    """
    def __init__(self, subst, members, inst, lookup, expo, ns):
        self.subst = subst
        self.members = members
        self.inst = inst
        self.lookup = lookup
        self.expo = expo
        self.ns = ns

    def type(self, toks, pack_index=None):
        out = []
        i = 0
        while i < len(toks):
            t = toks[i]
            prev = toks[i - 1].text if i else ''
            if t.kind == 'expo':
                key = '@' + t.text + '@'
                if key in self.subst:
                    out.append(self._sub(self.subst[key], pack_index))
                    if i + 1 < len(toks) and toks[i + 1].text == '<':
                        # a nested class template (`@iterator@<Const>`): the instantiation stands for it
                        i = decls._skip_template_args(toks, i + 1)
                        continue
                elif t.text in self.expo:
                    out.append(self.expo[t.text])
                else:
                    raise Unresolved(t.text)
            elif t.kind == 'id' and prev != '::' and prev != '.':
                n = t.text
                nxt = toks[i + 1].text if i + 1 < len(toks) else ''
                if n in getattr(self, 'params', {}):
                    out.append(self.params[n])
                elif n in self.subst:
                    out.append(self._sub(self.subst[n], pack_index))
                elif n in KEYWORDS or n in ('size_t', 'ptrdiff_t') and False:
                    out.append(n)
                elif n == 'std':
                    out.append('std')
                elif n == getattr(self, 'cls_name', None) and self.inst and nxt != '<':
                    out.append(self.inst)          # the injected-class-name
                elif n in self.members and self.inst:
                    out.append(self.inst + '::' + n)
                elif n in getattr(self, 'outer_members', ()) and getattr(self, 'outer_inst', None):
                    out.append(self.outer_inst + '::' + n)
                elif nxt == '::' and n in ('ranges', 'views', 'execution', 'chrono', 'this_thread', 'pmr', 'filesystem'):
                    out.append('std::' + n)
                elif n in self.lookup:
                    out.append(self.lookup[n])
                elif n in ('nothrow', 'nothrow_t', 'size_t', 'ptrdiff_t', 'nullptr_t', 'byte'):
                    out.append('std::' + n)
                else:
                    raise Unresolved(n)
            else:
                out.append(t.text)
            i += 1
        s = ''
        for x in out:
            if s and (s[-1].isalnum() or s[-1] in '_>') and (x[:1].isalnum() or x[:1] == '_'):
                s += ' '
            s += x
        return s.replace('...', '')

    def _sub(self, v, pack_index):
        if isinstance(v, list):
            if pack_index is None:
                return ', '.join(self._wrap(x) for x in v)
            return self._wrap(v[pack_index])
        return self._wrap(v)

    @staticmethod
    def _wrap(v):
        # `const T&` with T = int* is `int* const&`, and `R&&` with R = vector<int>& is
        # `vector<int>&`: spelled through type_identity_t, the declarator applies to the type
        if '&' in v or '*' in v:
            return f'std::type_identity_t<{v}>'
        return v
