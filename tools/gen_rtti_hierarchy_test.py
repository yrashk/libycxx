#!/usr/bin/env python3
# Generates tests/ycxx/rtti/random_hierarchies_model.pass.cpp: random class hierarchies (virtual
# and non-virtual bases, public/protected/private derivation) with dynamic_cast and handler
# expectations computed from a model of [expr.dynamic.cast]/9 and [except.handle]/3.2.
#   SIZES=5,6,7,8 tools/gen_rtti_hierarchy_test.py 3838 20 tests/ycxx/rtti/random_hierarchies_model.pass.cpp
# Hierarchies that g++-16 or clang++-23 reject (an inaccessible virtual base) are skipped.
import random, subprocess, sys, os, tempfile

ACC = ['public', 'protected', 'private']
SCR = tempfile.mkdtemp()


def gen_graph(rng, n):
    classes = []
    anc = []  # transitive base sets
    for i in range(n):
        bases = []
        if i > 0 and not (i < 2 or rng.random() < 0.15):
            k = rng.choice([1, 1, 2, 2, 2, 3])
            cand = list(range(i))
            rng.shuffle(cand)
            chosen = []
            for b in cand:
                if len(chosen) == k:
                    break
                # no direct base may be a base of another direct base (keeps to_X unambiguous)
                if any(b in anc[c] or c in anc[b] for c in chosen):
                    continue
                chosen.append(b)
            for b in chosen:
                virt = rng.random() < 0.45
                acc = rng.choices(ACC, weights=[6, 2, 2])[0]
                bases.append((b, virt, acc))
        classes.append(bases)
        a = set()
        for (b, _, _) in bases:
            a.add(b)
            a |= anc[b]
        anc.append(a)
    return classes, anc


class Obj:
    """subobject DAG of a complete object of class m"""

    def __init__(self, classes, m):
        self.classes = classes
        self.nodes = []  # (type, key)
        self.index = {}
        self.edges = {}  # node -> list of (child, access)
        self.path = {}  # node -> list of base class indices from m (canonical)
        root = self.node(m, ('root',), [])
        self.root = root
        self.expand(root)

    def node(self, t, key, path):
        if key in self.index:
            return self.index[key]
        nid = len(self.nodes)
        self.nodes.append(t)
        self.index[key] = nid
        self.edges[nid] = []
        self.path[nid] = path
        return nid

    def expand(self, nid):
        t = self.nodes[nid]
        for (b, virt, acc) in self.classes[t]:
            key = ('v', b) if virt else ('n', nid, b)
            new = key not in self.index
            child = self.node(b, key, self.path[nid] + [b])
            self.edges[nid].append((child, acc))
            if new:
                self.expand(child)

    def paths_access(self, a, b):
        """(reachable, public_reachable) from node a to node b"""
        memo = {}

        def go(x):
            if x == b:
                return (True, True)
            if x in memo:
                return memo[x]
            r = (False, False)
            for (c, acc) in self.edges[x]:
                rr, pp = go(c)
                r = (r[0] or rr, r[1] or (pp and acc == 'public'))
            memo[x] = r
            return r

        return go(a)

    def cast(self, s, c):
        """expected node of dynamic_cast<C*>(pointer to node s), or None"""
        cs = [x for x in range(len(self.nodes)) if self.nodes[x] == c and self.paths_access(x, s)[0]]
        if len(cs) == 1 and self.paths_access(cs[0], s)[1]:
            return cs[0]
        if self.paths_access(self.root, s)[1]:
            allc = [x for x in range(len(self.nodes)) if self.nodes[x] == c]
            if len(allc) == 1 and self.paths_access(self.root, allc[0])[1]:
                return allc[0]
        return None

    def handler(self, t):
        if self.nodes[self.root] == t:
            return True
        allt = [x for x in range(len(self.nodes)) if self.nodes[x] == t]
        return len(allt) == 1 and self.paths_access(self.root, allt[0])[1]


def class_decls(ns, classes):
    out = [f'namespace {ns} {{']
    for i, bases in enumerate(classes):
        bl = ', '.join(f'{acc} {"virtual " if virt else ""}C{b}' for (b, virt, acc) in bases)
        out.append(f'struct C{i}{" : " + bl if bl else ""} {{')
        out.append(f'  int f{i} = {i};')
        if not bases:
            out.append(f'  virtual ~C{i}() = default;')
        for (b, _, _) in bases:
            out.append(f'  C{b}* to_C{b}() {{ return this; }}')
        out.append('};')
    return out


def ptr_expr(var, path):
    e = f'(&{var})'
    for b in path:
        e = f'{e}->to_C{b}()'
    return e


def body(ns, classes, anc):
    out = [f'void run_{ns}() {{', f'  using namespace {ns};']
    n = len(classes)
    for m in range(n):
        o = Obj(classes, m)
        out.append('  {')
        out.append(f'    C{m} m;')
        seen = set()
        for s in range(len(o.nodes)):
            st = o.nodes[s]
            sp = ptr_expr('m', o.path[s])
            out.append(f'    {{ C{st}* s = {sp};')
            out.append(f'      CHECK(dynamic_cast<void*>(s) == static_cast<void*>(&m));')
            for c in range(n):
                if c == st or c in anc[st]:
                    continue  # same type or an upcast: not a runtime check
                r = o.cast(s, c)
                want = 'nullptr' if r is None else ptr_expr('m', o.path[r])
                out.append(f'      CHECK(dynamic_cast<C{c}*>(s) == {want});')
            out.append('    }')
        out.append('  }')
        for t in range(n):
            if t != m and t not in anc[m]:
                continue
            exp = 'true' if o.handler(t) else 'false'
            out.append(f'  CHECK(caught<C{m}, C{t}>() == {exp});')
            out.append(f'  CHECK(caught_ptr<C{m}, C{t}>() == {exp});')
    out.append('}')
    return out


HEADER = '''// Randomly generated class hierarchies (virtual and non-virtual bases, public, protected and
// private derivation, repeated and shared bases) with every runtime-checked dynamic_cast from
// every subobject of every complete object, and every handler for a base of every thrown type,
// against a model of the rules:
// [expr.dynamic.cast]/8: dynamic_cast<void*> yields the most derived object; /9.1 "If, in the
// most derived object pointed (referred) to by v, v points (refers) to a public base class
// subobject of a C object, and if only one object of type C is derived from the subobject
// pointed (referred) to by v, the result points (refers) to that C object"; /9.2 "Otherwise,
// if v points (refers) to a public base class subobject of the most derived object, and the
// type of the most derived object has a base class, of type C, that is unambiguous and public,
// the result points (refers) to the C subobject of the most derived object"; /9.3 otherwise
// null. [except.handle]/3.2: a handler for T matches E if T is an unambiguous public base of E
// (and /3.3 likewise for a handler const T* and a thrown E*).
// [class.paths]/1: a base reached by several paths has the access of the most accessible one,
// so a subobject is a public base subobject when some path to it is public throughout.
// Generated by tools/gen_rtti_hierarchy_test.py (seed 3838, SIZES=5,6,7,8, 20 hierarchies);
// hierarchies that a compiler rejects (an inaccessible virtual base) were left out.
#include "check.hpp"

template <class E, class T>
bool caught() {
  try {
    throw E();
  } catch (T&) {
    return true;
  } catch (...) {
    return false;
  }
}

// [except.handle]/3.3: a pointer handler matches by a standard pointer conversion "not involving
// conversions to pointers to private or protected or ambiguous classes"
template <class E, class T>
bool caught_ptr() {
  static E e;
  try {
    throw &e;
  } catch (const T*) {
    return true;
  } catch (...) {
    return false;
  }
}
'''


def compiles(lines):
    src = os.path.join(SCR, 'gen_probe.cpp')
    with open(src, 'w') as f:
        f.write('\n'.join(lines) + '\nint main() {}\n')
    for cc in ['g++-16', 'clang++-23']:
        r = subprocess.run([cc, '-std=c++26', '-fsyntax-only', '-w', src], capture_output=True)
        if r.returncode != 0:
            return False
    return True


def main():
    rng = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 38)
    want = int(sys.argv[2]) if len(sys.argv) > 2 else 14
    out_path = sys.argv[3]
    decls, bodies, names = [], [], []
    tries = 0
    while len(names) < want and tries < 400:
        tries += 1
        n = rng.choice([int(x) for x in os.environ.get('SIZES', '4,5,6,6,7').split(',')])
        classes, anc = gen_graph(rng, n)
        if not any(any(v for (_, v, _) in b) for b in classes):
            continue
        if not any(any(a != 'public' for (_, _, a) in b) for b in classes):
            continue
        ns = f'h{len(names)}'
        d = class_decls(ns, classes) + ['}']
        # instantiate construction and copies of every class
        probe = d + [f'void probe_{ns}() {{'] + [f'  {{ {ns}::C{i} x; {ns}::C{i} y = x; (void)y; }}' for i in range(n)] + ['}']
        if not compiles(probe):
            continue
        decls += d
        bodies += body(ns, classes, anc)
        names.append(ns)
    with open(out_path, 'w') as f:
        f.write(HEADER)
        f.write('\n'.join(decls) + '\n\n')
        f.write('\n'.join(bodies) + '\n\n')
        f.write('int main() {\n')
        f.write('  for (int round = 0; round < 2; ++round) {\n')
        for nm in names:
            f.write(f'    run_{nm}();\n')
        f.write('  }\n}\n')
    print(len(names), 'hierarchies after', tries, 'tries')


main()
