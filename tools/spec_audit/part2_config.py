"""Audit part 2 (docs/SPEC_COVERAGE.md): [containers], [iterators], [ranges], [algorithms], [strings].

The configuration of tools/spec_audit/gen_probes.py for this part: how each class of the clauses
is instantiated in a probe and which template arguments stand for the draft's template parameters.
The probes use only what the draft specifies; p2:: names are the probes' own helper types.
"""
import os, re

CLAUSES = ['containers', 'iterators', 'ranges', 'algorithms', 'strings']

HEADERS = ['algorithm', 'array', 'deque', 'flat_map', 'flat_set', 'forward_list', 'generator', 'hive',
           'inplace_vector', 'iterator', 'list', 'map', 'mdspan', 'numeric', 'queue', 'ranges', 'set', 'span',
           'stack', 'string', 'string_view', 'unordered_map', 'unordered_set', 'vector', 'cstring', 'memory',
           'execution', 'random', 'format', 'sstream', 'istream', 'ostream', 'tuple', 'functional',
           'utility', 'concepts', 'type_traits', 'cstddef']

# the header each synopsis declares
SECTION_HEADER = {
    'array.syn': 'array', 'deque.syn': 'deque', 'forward.list.syn': 'forward_list', 'hive.syn': 'hive',
    'list.syn': 'list', 'vector.syn': 'vector', 'inplace.vector.syn': 'inplace_vector',
    'associative.map.syn': 'map', 'associative.set.syn': 'set', 'unord.map.syn': 'unordered_map',
    'unord.set.syn': 'unordered_set', 'queue.syn': 'queue', 'stack.syn': 'stack', 'flat.map.syn': 'flat_map',
    'flat.set.syn': 'flat_set', 'span.syn': 'span', 'mdspan.syn': 'mdspan', 'iterator.synopsis': 'iterator',
    'syn': 'ranges', 'generator.syn': 'generator', 'algorithm.syn': 'algorithm',
    'numeric.ops.overview': 'numeric', 'specialized.algorithms.general': 'memory',
    'string.view.synop': 'string_view', 'string.syn': 'string', 'cstring.syn': 'cstring',
}

# a name each header the synopses #include declares (to check that the #include is there)
HEADER_REPRESENTATIVE = {
    'compare': 'std::strong_ordering', 'initializer_list': 'std::initializer_list<int>',
    'iterator': 'std::reverse_iterator<int*>', 'concepts': 'expr:std::integral<int>',
}

# exposition-only alias templates the prelude defines, as the draft does
EXPO = {
    'maybe-const': 'p2::maybe_const',
    'iter-value-type': 'std::iter_value_t',
    'iter-key-type': 'p2::iter_key_type',
    'iter-mapped-type': 'p2::iter_mapped_type',
    'iter-to-alloc-type': 'p2::iter_to_alloc_type',
    'range-key-type': 'p2::range_key_type',
    'range-mapped-type': 'p2::range_mapped_type',
    'range-to-alloc-type': 'p2::range_to_alloc_type',
    'iter-const-reference-t': 'std::iter_const_reference_t',
}

EXTRA_LOOKUP = {
    'allocator': 'std::allocator', 'polymorphic_allocator': 'std::pmr::polymorphic_allocator',
}

V = 'p2::V'           # std::ranges::ref_view<std::vector<int>>
NCV = 'p2::NCV'       # a view whose sentinel is not its iterator
VEC = 'std::vector<int>'

NS_SUBST = {
    'std': {},
    'std::ranges': {},
    'std::ranges::views': {},
}


def default_targ(tp, f):
    """The template argument for a function's (or specialization's) own template parameter."""
    n = tp.name
    if n is None:
        return None
    if tp.pack:
        return {'Args': ['int'], 'Rs': ['std::vector<int>&'], 'Views': [V, V], 'Ts': ['int'],
                'OtherIndexTypes': ['std::size_t', 'std::size_t'], 'Extents': ['std::dynamic_extent', 'std::dynamic_extent'],
                'SliceSpecifiers': ['std::full_extent_t', 'std::full_extent_t'], 'U': ['int'],
                'Fs': ['p2::AnyFn'], 'Vs': [V]}.get(n)
    kind = ' '.join(t.text for t in tp.kind)
    if tp.is_type and re.search(r'predicate|relation|invocable|strict_weak_order|equivalence|indirectly_unary|copy_constructible F', kind):
        return 'p2::AnyFn'
    if not tp.is_type:
        k = ' '.join(t.text for t in tp.kind)
        if 'bool' in k:
            return 'false'
        if 'subrange_kind' in k:
            return 'std::ranges::subrange_kind::sized'
        return {'N': '2', 'I': '0', 'Extent': 'std::dynamic_extent', 'Rank': '2'}.get(n, '1')
    if re.fullmatch(r'(NoThrow)?(Input|Forward|Bidirectional|RandomAccess|Contiguous|Output)Iterator\d?', n):
        return 'int*'
    if re.fullmatch(r'(I|I1|I2|O|O1|O2|OutI|S|S1|S2|OutS|It|Iter|Iterator|Iterator1|Iterator2|End|Sent)', n):
        return 'int*'
    if re.fullmatch(r'(R|R1|R2|OutR|Range)', n):
        return 'std::vector<int>&'
    if re.fullmatch(r'(Predicate|BinaryPredicate|UnaryPredicate|Compare|Function|UnaryOperation|BinaryOperation\d?|'
                    r'Generator|Pred|Comp|Fun|F|F2|Op|BinaryOp|UnaryOp|Fn|Callable)', n):
        return 'p2::AnyFn'
    if n in ('Proj', 'Proj1', 'Proj2'):
        return 'std::identity'
    if n in ('ExecutionPolicy', 'Ep'):
        return 'const std::execution::parallel_policy&'
    if n in ('Gen', 'UniformRandomBitGenerator', 'URBG'):
        return 'std::mt19937&'
    if n in ('T', 'U', 'Tp', 'Size', 'Distance', 'M', 'K', 'Up', 'W', 'Bound', 'Integer', 'X', 'Y', 'E'):
        return 'int'
    if n in ('C', 'Container', 'Cont'):
        return 'std::vector<int>'
    if n in ('charT', 'CharT'):
        return 'char'
    if n in ('traits', 'Traits'):
        return 'std::char_traits<char>'
    if n == 'Allocator' or n == 'Alloc':
        return 'std::allocator<int>'
    if n in ('V',):
        return V
    if n in ('ElementType',):
        return 'int'
    if n in ('IndexType', 'OtherIndexType', 'SizeType'):
        return 'std::size_t'
    if n in ('Src', 'Dst'):
        return 'p2::MDS'
    return None


def _view(inst, **subst):
    s = {'V': V, 'Pred': 'p2::AnyFn', 'F': 'p2::AnyFn', 'Const': 'false', '@Base@': None}
    s.update(subst)
    if s['@Base@'] is None:
        s['@Base@'] = s['V']
    return {'inst': inst, 'subst': s}


def _views():
    c = {}

    def add(name, args, sect_args=None, **subst):
        """A view class, its iterator and its sentinel (in a non-common instantiation)."""
        inst = f'std::ranges::{name}<{args}>'
        it = f'std::ranges::iterator_t<{inst}>'
        c[name] = _view(inst, **dict(subst, **{'@iterator@': it}))
        c[f'{name}<{_params[name]}>::@iterator@'] = _view(it, **dict(subst, **{'@iterator@': it}))
        if sect_args is not None:
            ninst = f'std::ranges::{name}<{sect_args}>'
            nv = {'p2::VV': 'p2::NCVV', 'p2::TV': 'p2::NCTV'}.get(subst.get('V', V), NCV)
            s2 = dict(subst, V=nv)
            if 'Views' in s2:
                s2['Views'] = [NCV, NCV]
            s2['@iterator@'] = f'std::ranges::iterator_t<{ninst}>'
            s2['@sentinel@'] = f'std::ranges::sentinel_t<{ninst}>'
            c[f'{name}<{_params[name]}>::@sentinel@'] = _view(s2['@sentinel@'], **s2)
    _params = {
        'iota_view': 'W, Bound', 'repeat_view': 'T, Bound', 'basic_istream_view': 'Val, CharT, Traits',
        'filter_view': 'V, Pred', 'transform_view': 'V, F', 'take_view': 'V', 'take_while_view': 'V, Pred',
        'join_view': 'V', 'join_with_view': 'V, Pattern', 'split_view': 'V, Pattern',
        'concat_view': 'Views...', 'elements_view': 'V, N', 'enumerate_view': 'V', 'zip_view': 'Views...',
        'zip_transform_view': 'F, Views...', 'adjacent_view': 'V, N', 'adjacent_transform_view': 'V, F, N',
        'chunk_view': 'V', 'slide_view': 'V', 'chunk_by_view': 'V, Pred', 'stride_view': 'V',
        'cartesian_product_view': 'First, Vs...', 'cache_latest_view': 'V', 'as_input_view': 'V',
        'lazy_split_view': 'V, Pattern',
    }
    add('iota_view', 'int, int', None, W='int', Bound='int')
    c['iota_view<W, Bound>::@sentinel@'] = _view('std::ranges::sentinel_t<std::ranges::iota_view<int, long>>', W='int', Bound='long')
    add('repeat_view', 'int, int', None, T='int', Bound='int')
    add('basic_istream_view', 'int, char', None, Val='int', CharT='char', Traits='std::char_traits<char>')
    add('filter_view', f'{V}, p2::AnyFn', f'{NCV}, p2::AnyFn')
    add('transform_view', f'{V}, p2::AnyFn', f'{NCV}, p2::AnyFn')
    add('take_view', V, None)
    c['take_view<V>::@sentinel@'] = _view(f'std::ranges::sentinel_t<std::ranges::take_view<{NCV}>>', V=NCV)
    add('take_while_view', f'{V}, p2::AnyFn', None)
    c['take_while_view<V, Pred>::@sentinel@'] = _view(f'std::ranges::sentinel_t<std::ranges::take_while_view<{V}, p2::AnyFn>>')
    add('join_view', 'p2::VV', 'p2::NCVV', V='p2::VV')
    add('join_with_view', 'p2::VV, std::ranges::single_view<int>', 'p2::NCVV, std::ranges::single_view<int>',
        V='p2::VV', Pattern='std::ranges::single_view<int>')
    add('split_view', f'{V}, std::ranges::single_view<int>', None, Pattern='std::ranges::single_view<int>')
    c['split_view<V, Pattern>::@sentinel@'] = _view(f'std::ranges::sentinel_t<std::ranges::split_view<{NCV}, std::ranges::single_view<int>>>',
                                                   V=NCV, Pattern='std::ranges::single_view<int>')
    add('lazy_split_view', f'{V}, std::ranges::single_view<int>', None, Pattern='std::ranges::single_view<int>')
    c['lazy_split_view<V, Pattern>::@outer-iterator@'] = _view(
        f'std::ranges::iterator_t<std::ranges::lazy_split_view<{V}, std::ranges::single_view<int>>>', Pattern='std::ranges::single_view<int>')
    c['lazy_split_view<V, Pattern>::@outer-iterator@<Const>::value_type'] = _view(
        f'std::ranges::range_value_t<std::ranges::lazy_split_view<{V}, std::ranges::single_view<int>>>', Pattern='std::ranges::single_view<int>')
    c['lazy_split_view<V, Pattern>::@inner-iterator@'] = _view(
        f'std::ranges::iterator_t<std::ranges::range_value_t<std::ranges::lazy_split_view<{V}, std::ranges::single_view<int>>>>',
        Pattern='std::ranges::single_view<int>')
    add('concat_view', f'{V}, {V}', None, Views=[V, V])
    add('elements_view', 'p2::TV, 0', 'p2::NCTV, 0', V='p2::TV', N='0')
    add('enumerate_view', V, NCV)
    add('zip_view', f'{V}, {V}', f'{NCV}, {NCV}', Views=[V, V])
    add('zip_transform_view', f'p2::AnyFn, {V}, {V}', f'p2::AnyFn, {NCV}, {NCV}', Views=[V, V])
    add('adjacent_view', f'{V}, 2', f'{NCV}, 2', N='2')
    add('adjacent_transform_view', f'{V}, p2::AnyFn, 2', f'{NCV}, p2::AnyFn, 2', N='2')
    add('slide_view', V, None)
    c['slide_view<V>::@sentinel@'] = _view(f'std::ranges::sentinel_t<std::ranges::slide_view<{NCV}>>', V=NCV)
    add('chunk_by_view', f'{V}, p2::AnyFn', None)
    add('stride_view', V, None)
    add('cartesian_product_view', f'{V}, {V}', None, First=V, Vs=[V])
    add('cache_latest_view', V, NCV)
    add('as_input_view', V, None)
    # chunk_view: the input-range form (over an input view) and the forward form
    c['range.chunk.view.input|chunk_view'] = _view('std::ranges::chunk_view<p2::IV>', V='p2::IV')
    c['chunk_view<V>::@outer-iterator@'] = _view('std::ranges::iterator_t<std::ranges::chunk_view<p2::IV>>', V='p2::IV')
    c['chunk_view<V>::@outer-iterator@::value_type'] = _view('std::ranges::range_value_t<std::ranges::chunk_view<p2::IV>>', V='p2::IV')
    c['chunk_view<V>::@inner-iterator@'] = _view(
        'std::ranges::iterator_t<std::ranges::range_value_t<std::ranges::chunk_view<p2::IV>>>', V='p2::IV')
    c['chunk_view<V>'] = _view(f'std::ranges::chunk_view<{V}>')
    c['chunk_view<V>::@iterator@'] = _view(f'std::ranges::iterator_t<std::ranges::chunk_view<{V}>>')
    for name, args in [('ref_view', VEC), ('owning_view', VEC), ('as_rvalue_view', V), ('common_view', NCV),
                       ('reverse_view', V), ('as_const_view', V), ('drop_view', V), ('drop_while_view', f'{V}, p2::AnyFn'),
                       ('single_view', 'int'), ('empty_view', 'int')]:
        c[name] = _view(f'std::ranges::{name}<{args}>', R=VEC, T='int', **({'V': NCV} if name == 'common_view' else {}))
    c['subrange'] = {'inst': 'std::ranges::subrange<int*>', 'subst': {'I': 'int*', 'S': 'int*', 'K': 'std::ranges::subrange_kind::sized'}}
    c['view_interface'] = {'inst': V, 'subst': {'D': V}}
    c['dangling'] = {'inst': 'std::ranges::dangling', 'subst': {}}
    c['elements_of'] = {'inst': 'std::ranges::elements_of<std::vector<int>&>', 'subst': {'R': 'std::vector<int>&', 'Allocator': 'std::allocator<std::byte>'}}
    c['generator'] = {'inst': 'std::generator<int>', 'subst': {'Ref': 'int', 'Val': 'void', 'Allocator': 'void',
                                                              '@reference@': 'int&&', '@value@': 'int', '@yielded@': 'int&&'}}
    c['generator<Ref, Val, Allocator>::promise_type'] = {'inst': 'std::generator<int>::promise_type', 'subst': {
        'Ref': 'int', 'Val': 'void', 'Allocator': 'void', 'R2': 'int', 'V2': 'void', 'Alloc2': 'void', 'Unused': 'void',
        'R': 'std::vector<int>&', '@yielded@': 'int&&'}}
    c['generator<Ref, Val, Allocator>::@iterator@'] = {'inst': 'std::ranges::iterator_t<std::generator<int>>', 'subst': {
        'Ref': 'int', 'Val': 'void', 'Allocator': 'void', '@reference@': 'int&&', '@value@': 'int'}}
    return c


def _containers():
    c = {}
    seq = {'T': 'int', 'Allocator': 'std::allocator<int>', 'R': 'std::vector<int>', 'InputIterator': 'int*', 'Args': ['int'],
           'U': 'int', 'Predicate': 'p2::AnyFn', 'K': 'int'}
    for n in ('vector', 'deque', 'list', 'forward_list', 'hive'):
        c[n] = {'inst': f'std::{n}<int>', 'subst': dict(seq)}
    c['inplace_vector'] = {'inst': 'std::inplace_vector<int, 4>', 'subst': dict(seq, N='4')}
    c['array'] = {'inst': 'std::array<int, 3>', 'subst': dict(seq, N='3')}
    c['vector<bool, Allocator>'] = {'inst': 'std::vector<bool>', 'subst': dict(seq, T='bool', Allocator='std::allocator<bool>',
                                                                             R='std::vector<bool>', InputIterator='bool*', Args=['bool'])}
    c['vector<bool, Allocator>::reference'] = {'inst': 'std::vector<bool>::reference', 'subst': {'Allocator': 'std::allocator<bool>'}}
    c['hive_limits'] = {'inst': 'std::hive_limits', 'subst': {}}
    assoc = {'Key': 'int', 'T': 'p2::M', 'Compare': 'std::less<>', 'Allocator': 'std::allocator<std::pair<const int, p2::M>>',
             'InputIterator': 'std::pair<int, p2::M>*', 'R': 'std::vector<std::pair<int, p2::M>>', 'Args': ['int', 'int'],
             'M': 'int', 'K': 'long', 'P': 'std::pair<int, p2::M>', 'Predicate': 'p2::AnyFn', 'C2': 'std::greater<>'}
    for n in ('map', 'multimap'):
        c[n] = {'inst': f'std::{n}<int, p2::M, std::less<>>', 'subst': dict(assoc),
                'guide_subst': {'Compare': 'std::less<int>', 'Allocator': 'std::allocator<std::pair<const int, p2::M>>',
                                'R': 'std::vector<std::pair<int, p2::M>>'}}
        c[f'{n}::value_compare'] = {'inst': f'std::{n}<int, p2::M, std::less<>>::value_compare', 'subst': dict(assoc)}
    sets = {'Key': 'int', 'Compare': 'std::less<>', 'Allocator': 'std::allocator<int>', 'InputIterator': 'int*',
            'R': 'std::vector<int>', 'Args': ['int'], 'K': 'long', 'Predicate': 'p2::AnyFn', 'C2': 'std::greater<>'}
    for n in ('set', 'multiset'):
        c[n] = {'inst': f'std::{n}<int, std::less<>>', 'subst': dict(sets), 'guide_subst': {'Compare': 'std::less<int>'}}
    uassoc = dict(assoc, Hash='p2::THash', Pred='std::equal_to<>', H2='std::hash<int>', P2='std::equal_to<int>')
    del uassoc['Compare']
    for n in ('unordered_map', 'unordered_multimap'):
        c[n] = {'inst': f'std::{n}<int, p2::M, p2::THash, std::equal_to<>>', 'subst': dict(uassoc),
                'guide_subst': {'Hash': 'std::hash<int>', 'Pred': 'std::equal_to<int>',
                                'Allocator': 'std::allocator<std::pair<const int, p2::M>>'}}
    usets = dict(sets, Hash='p2::THash', Pred='std::equal_to<>', H2='std::hash<int>', P2='std::equal_to<int>')
    del usets['Compare']
    for n in ('unordered_set', 'unordered_multiset'):
        c[n] = {'inst': f'std::{n}<int, p2::THash, std::equal_to<>>', 'subst': dict(usets),
                'guide_subst': {'Hash': 'std::hash<int>', 'Pred': 'std::equal_to<int>'}}
    ad = {'T': 'int', 'Container': 'std::deque<int>', 'InputIterator': 'int*', 'R': 'std::vector<int>', 'Alloc': 'std::allocator<int>',
          'Args': ['int']}
    c['queue'] = {'inst': 'std::queue<int>', 'subst': dict(ad)}
    c['stack'] = {'inst': 'std::stack<int>', 'subst': dict(ad)}
    c['priority_queue'] = {'inst': 'std::priority_queue<int>', 'subst': dict(ad, Container='std::vector<int>', Compare='std::less<int>')}
    fm = {'Key': 'int', 'T': 'p2::M', 'Compare': 'std::less<>', 'KeyContainer': 'std::vector<int>',
          'MappedContainer': 'std::vector<p2::M>', 'InputIterator': 'std::pair<int, p2::M>*', 'R': 'std::vector<std::pair<int, p2::M>>',
          'Alloc': 'std::allocator<int>', 'Args': ['int', 'int'], 'M': 'int', 'K': 'long', 'P': 'std::pair<int, p2::M>',
          'Predicate': 'p2::AnyFn'}
    for n in ('flat_map', 'flat_multimap'):
        c[n] = {'inst': f'std::{n}<int, p2::M, std::less<>>', 'subst': dict(fm)}
        c[f'{n}::value_compare'] = {'inst': f'std::{n}<int, p2::M, std::less<>>::value_compare', 'subst': dict(fm)}
        c[f'{n}::containers'] = {'inst': f'std::{n}<int, p2::M, std::less<>>::containers', 'subst': dict(fm)}
    fs = {'Key': 'int', 'Compare': 'std::less<>', 'KeyContainer': 'std::vector<int>', 'InputIterator': 'int*',
          'R': 'std::vector<int>', 'Alloc': 'std::allocator<int>', 'Args': ['int'], 'K': 'long', 'Predicate': 'p2::AnyFn'}
    for n in ('flat_set', 'flat_multiset'):
        c[n] = {'inst': f'std::{n}<int, std::less<>>', 'subst': dict(fs)}
    for n in ('sorted_unique_t', 'sorted_equivalent_t', 'full_extent_t', 'from_range_t'):
        c[n] = {'inst': f'std::{n}', 'subst': {}}
    sp = {'ElementType': 'int', 'Extent': 'std::dynamic_extent', 'It': 'int*', 'End': 'int*', 'R': 'std::vector<int>&',
          'OtherElementType': 'int', 'OtherExtent': 'std::dynamic_extent', 'Count': '1', 'Offset': '0', 'N': '3'}
    c['span'] = {'inst': 'std::span<int>', 'subst': sp}
    mds = {'IndexType': 'std::size_t', 'Extents': 'std::dextents<std::size_t, 2>', 'OtherIndexType': 'std::size_t',
           'OtherIndexTypes': ['std::size_t', 'std::size_t'], 'OtherExtents': 'std::dextents<std::size_t, 2>',
           'LayoutLeftPaddedMapping': 'std::layout_left_padded<4>::mapping<std::dextents<std::size_t, 2>>',
           'LayoutRightPaddedMapping': 'std::layout_right_padded<4>::mapping<std::dextents<std::size_t, 2>>',
           'StridedLayoutMapping': 'std::layout_stride::mapping<std::dextents<std::size_t, 2>>',
           'ElementType': 'int', 'OtherElementType': 'int', 'PaddingValue': '4', 'ByteAlignment': '16',
           'LayoutPolicy': 'std::layout_right', 'AccessorPolicy': 'std::default_accessor<int>', 'N': '2',
           'SizeType': 'std::size_t', 'OtherLayoutPolicy': 'std::layout_right', 'OtherAccessor': 'std::default_accessor<int>',
           'OtherMapping': 'std::layout_right::mapping<std::dextents<std::size_t, 2>>', 'Integrals': ['std::size_t', 'std::size_t'],
           'OtherAccessorPolicy': 'std::default_accessor<int>', 'Indices': ['std::size_t', 'std::size_t'],
           'SliceSpecifiers': ['std::full_extent_t', 'std::full_extent_t'], 'OtherByteAlignment': '16'}
    c['extents'] = {'inst': 'std::extents<std::size_t, std::dynamic_extent, 3>',
                    'subst': dict(mds, Extents=['std::dynamic_extent', '3'], OtherIndexTypes=['std::size_t'],
                                  OtherExtents=['std::dynamic_extent', '3'])}
    for l in ('left', 'right', 'stride'):
        c[f'layout_{l}'] = {'inst': f'std::layout_{l}', 'subst': mds}
        c[f'layout_{l}::mapping'] = {'inst': f'std::layout_{l}::mapping<std::dextents<std::size_t, 2>>', 'subst': mds}
    for l in ('left', 'right'):
        c[f'layout_{l}_padded'] = {'inst': f'std::layout_{l}_padded<4>', 'subst': mds}
        c[f'layout_{l}_padded<PaddingValue>::mapping'] = {'inst': f'std::layout_{l}_padded<4>::mapping<std::dextents<std::size_t, 2>>', 'subst': mds}
    c['default_accessor'] = {'inst': 'std::default_accessor<int>', 'subst': mds}
    c['aligned_accessor'] = {'inst': 'std::aligned_accessor<int, 16>', 'subst': mds}
    c['mdspan'] = {'inst': 'std::mdspan<int, std::dextents<std::size_t, 2>>', 'subst': mds}
    c['extent_slice'] = {'inst': 'std::extent_slice<int, int, int>', 'subst': {'OffsetType': 'int', 'ExtentType': 'int', 'StrideType': 'int'}}
    c['range_slice'] = {'inst': 'std::range_slice<int, int, int>', 'subst': {'FirstType': 'int', 'LastType': 'int', 'StrideType': 'int'}}
    c['submdspan_mapping_result'] = {'inst': 'std::submdspan_mapping_result<std::layout_right::mapping<std::dextents<std::size_t, 2>>>',
                                     'subst': {'LayoutMapping': 'std::layout_right::mapping<std::dextents<std::size_t, 2>>'}}
    return c


def _iterators():
    c = {}
    it = {'Iterator': 'int*', 'U': 'int*', 'Iterator2': 'int*', 'I': 'int*', 'I2': 'int*', 'S': 'int*', 'S2': 'int*',
          'Container': 'std::vector<int>', 'T': 'int', 'charT': 'char', 'traits': 'std::char_traits<char>',
          'Distance': 'std::ptrdiff_t', 'Iterator1': 'int*', 'Sent': 'int*', 'Other': 'int*'}
    c['reverse_iterator'] = {'inst': 'std::reverse_iterator<int*>', 'subst': it}
    c['move_iterator'] = {'inst': 'std::move_iterator<int*>', 'subst': it}
    c['move_sentinel'] = {'inst': 'std::move_sentinel<int*>', 'subst': it}
    c['basic_const_iterator'] = {'inst': 'std::basic_const_iterator<int*>', 'subst': dict(it, Other='int*', T='int*')}
    c['common_iterator'] = {'inst': 'std::common_iterator<int*, std::unreachable_sentinel_t>',
                            'subst': dict(it, S='std::unreachable_sentinel_t', S2='std::unreachable_sentinel_t')}
    c['counted_iterator'] = {'inst': 'std::counted_iterator<int*>', 'subst': it}
    for n in ('back_insert_iterator', 'front_insert_iterator', 'insert_iterator'):
        c[n] = {'inst': f'std::{n}<std::deque<int>>', 'subst': dict(it, Container='std::deque<int>')}
    c['istream_iterator'] = {'inst': 'std::istream_iterator<int>', 'subst': it}
    c['ostream_iterator'] = {'inst': 'std::ostream_iterator<int>', 'subst': it}
    c['istreambuf_iterator'] = {'inst': 'std::istreambuf_iterator<char>', 'subst': it}
    c['ostreambuf_iterator'] = {'inst': 'std::ostreambuf_iterator<char>', 'subst': it}
    c['unreachable_sentinel_t'] = {'inst': 'std::unreachable_sentinel_t', 'subst': it}
    for n in ('in_fun_result', 'in_in_result', 'in_out_result', 'in_in_out_result', 'in_out_out_result', 'min_max_result',
              'in_found_result', 'in_value_result', 'out_value_result'):
        args = {'in_fun_result': 'int*, p2::AnyFn', 'in_in_result': 'int*, int*', 'in_out_result': 'int*, int*',
                'in_in_out_result': 'int*, int*, int*', 'in_out_out_result': 'int*, int*, int*', 'min_max_result': 'int',
                'in_found_result': 'int*', 'in_value_result': 'int*, int', 'out_value_result': 'int*, int'}[n]
        c[n] = {'inst': f'std::ranges::{n}<{args}>', 'subst': {'I': 'int*', 'I1': 'int*', 'I2': 'int*', 'O': 'int*', 'O1': 'int*',
                                                                'O2': 'int*', 'F': 'p2::AnyFn', 'T': 'int'}}
    ci = {'I': 'int*', 'S': 'std::unreachable_sentinel_t', 'T': 'int'}
    c['incrementable_traits<T*>'] = {'inst': 'std::incrementable_traits<int*>', 'subst': ci}
    c['iterator_traits<T*>'] = {'inst': 'std::iterator_traits<int*>', 'subst': ci}
    c['iterator_traits<common_iterator<I, S>>'] = {'inst': 'std::iterator_traits<std::common_iterator<int*, std::unreachable_sentinel_t>>', 'subst': ci}
    c['incrementable_traits<common_iterator<I, S>>'] = {'inst': 'std::incrementable_traits<std::common_iterator<int*, std::unreachable_sentinel_t>>', 'subst': ci}
    c['iterator_traits<counted_iterator<I>>'] = {'inst': 'std::iterator_traits<std::counted_iterator<int*>>', 'subst': ci}
    sr = {'I': 'int*', 'S': 'int*', 'K': 'std::ranges::subrange_kind::sized'}
    for k in ('0', '1'):
        for cv in ('', 'const '):
            c[f'tuple_element<{k}, {cv}ranges::subrange<I, S, K>>'] = {'inst': f'std::tuple_element<{k}, {cv}std::ranges::subrange<int*>>', 'subst': sr}
    c['formatter<T, charT>'] = {'inst': 'std::formatter<std::vector<bool>::reference, char>', 'subst': {'charT': 'char', 'T': 'std::vector<bool>::reference'}}
    for ch in ('char', 'char8_t', 'char16_t', 'char32_t', 'wchar_t'):
        c[f'char_traits<{ch}>'] = {'inst': f'std::char_traits<{ch}>', 'subst': {}}
    sv = {'charT': 'char', 'traits': 'std::char_traits<char>', 'It': 'const char*', 'End': 'const char*', 'R': 'std::vector<char>&'}
    c['basic_string_view'] = {'inst': 'std::string_view', 'subst': sv}
    c['basic_string'] = {'inst': 'std::string', 'subst': {
        'charT': 'char', 'traits': 'std::char_traits<char>', 'Allocator': 'std::allocator<char>', 'InputIterator': 'const char*',
        'R': 'std::vector<char>', 'T': 'std::string_view', 'Operation': 'p2::ResizeOp'}}
    return c


CLASSES = {}
CLASSES.update(_containers())
CLASSES.update(_iterators())
CLASSES.update(_views())
for _k, _c in CLASSES.items():
    # an exposition-only nested class names itself (`const @iterator@& x`): its instantiation
    _m = re.search(r'::(@[\w-]+@)$', _k)
    if _m:
        _c['subst'].setdefault(_m.group(1), _c['inst'])
        if _m.group(1) == '@sentinel@':
            _c['subst'].setdefault('@iterator@', _c['inst'].replace('sentinel_t<', 'iterator_t<'))

# template arguments for one declaration (its text, whitespace collapsed)
# template arguments for the declarations of one subclause
SECTION_SUBST = {
    'string.syn': {'charT': 'char', 'traits': 'std::char_traits<char>', 'Allocator': 'std::allocator<char>'},
    'string.view.synop': {'charT': 'char', 'traits': 'std::char_traits<char>'},
    'vector.bool.pspc': {'Allocator': 'std::allocator<bool>'},
    'common.iterator': {'S': 'std::unreachable_sentinel_t', 'I': 'int*'},
    'syn': {'T': V, 'K': 'std::ranges::subrange_kind::sized', 'Args': [], 'V': V, 'Pred': 'p2::AnyFn', 'F': 'p2::AnyFn', 'Pattern': 'std::ranges::single_view<int>'},
    'iterator.synopsis': {'T': 'int*', 'U': 'const int*', 'S': 'std::unreachable_sentinel_t'},
}

# template arguments for the declarations that match a pattern
SPEC_SUBST_RE = [
    (r'vector<bool, Allocator>', {'Allocator': 'std::allocator<bool>'}),
    (r'template<template<class\.\.\.> class C', {'C': 'std::vector'}),
    (r'enable_borrowed_range<common_view<T>>', {'T': 'p2::NCV'}),
    (r'enable_borrowed_range<elements_view<T, N>>', {'T': 'p2::TV', 'N': '0'}),
]

SPEC_SUBST = {
    # common_view's guide needs a range that is not common
    'template<class R> common_view(R && ) -> common_view<views::all_t<R>>': {'R': 'p2::NCV'},
    'template<class R> explicit join_view(R && ) -> join_view<views::all_t<R>>': {'R': 'std::vector<std::vector<int>>&'},
    'template<class R, class P> join_with_view(R && , P && ) -> join_with_view<views::all_t<R>, views::all_t<P>>':
        {'R': 'std::vector<std::vector<int>>&', 'P': 'std::ranges::single_view<int>'},
    'template<input_range R> join_with_view(R && , range_value_t<range_reference_t<R>>) -> join_with_view<views::all_t<R>, single_view<range_value_t<range_reference_t<R>>>>':
        {'R': 'std::vector<std::vector<int>>&'},
}


# declarations whose Constraints: (not a requires-clause) the probes' instantiation does not meet,
# or that need arguments a probe cannot spell: no check, with the reason
SKIP_TEXT = [
    (r'\(initializer_list<pair<Key, T>>', 'deduction guide from an initializer_list: CTAD from a braced list only'),
    (r'\(initializer_list<Key>', 'deduction guide from an initializer_list: CTAD from a braced list only'),
    (r'\(initializer_list<T>', 'deduction guide from an initializer_list: CTAD from a braced list only'),
    (r'yield_value\(ranges::elements_of<', 'needs a nested generator or range argument of a matching kind'),
    (r'mapping\(const layout_(right|left)::mapping<OtherExtents>&\)', 'Constraints: rank() <= 1, not met by the rank-2 instantiation'),
    (r'span\(const array<T, N>& arr\)', 'Constraints: const T convertible to element_type, not met by span<int>'),
    (r'operator PairLike', 'conversion template'),
    (r'mapping\(const Layout(Right|Left)PaddedMapping&\)', 'Constraints: rank() <= 1, not met by the rank-2 instantiation'),
]


def skip(sec, d):
    """Why a declaration gets no probe (None: it does)."""
    if d.name in ('keep_', 'sbuf_'):
        return 'exposition-only member'
    if any(c.startswith('@') and '::' not in c for c in d.cls):
        return 'member of an exposition-only class'
    t = ' '.join(d.text.split())
    for pat, why in SKIP_TEXT:
        if re.search(pat, t):
            return why
    if 'present only' in d.comment:
        return 'present only under a condition (' + ' '.join(d.comment.replace('//', ' ').split()) + ')'
    return None


PRELUDE = '''\
// Generated by tools/spec_audit/gen_probes.py from the working draft; do not edit.
// Spec-coverage probe of [{sec}] (docs/SPEC_COVERAGE.md, part 2). Compile only: each line ending
// in `// @E<n> <aspect>` checks one aspect of one entity (tools/spec_audit/part2/entities.tsv).
{includes}
namespace p2 {{
  // a callable that accepts anything: a predicate, a comparison, a projection, a function
  struct AnyFn {{ template<class... A> constexpr int operator()(A&&...) const noexcept {{ return 0; }} }};
  // a mapped type constructible from one or two ints (emplace, try_emplace)
  struct M {{
    int v = 0;
    constexpr M() = default;
    constexpr M(int a) : v(a) {{}}
    constexpr M(int a, int b) : v(a + b) {{}}
    friend constexpr bool operator==(const M&, const M&) = default;
    friend constexpr auto operator<=>(const M&, const M&) = default;
  }};
  struct THash {{
    using is_transparent = void;
    std::size_t operator()(long) const noexcept;
  }};
  // a declval whose type depends on D: a deleted function's use in a template is a substitution failure
  template<class D, class T> std::add_rvalue_reference_t<T> dv() noexcept;
  struct ResizeOp {{ std::size_t operator()(char*, std::size_t n) const {{ return n; }} }};
  struct Sent {{ friend constexpr bool operator==(const int*, Sent) noexcept {{ return false; }} }};
  struct SentV {{ friend constexpr bool operator==(const std::vector<int>*, SentV) noexcept {{ return false; }} }};
  struct SentT {{ friend constexpr bool operator==(const std::tuple<int, int>*, SentT) noexcept {{ return false; }} }};
  using V = std::ranges::ref_view<std::vector<int>>;
  using NCV = std::ranges::subrange<int*, Sent>;
  using VV = std::ranges::ref_view<std::vector<std::vector<int>>>;
  using NCVV = std::ranges::subrange<std::vector<int>*, SentV>;
  using TV = std::ranges::ref_view<std::vector<std::tuple<int, int>>>;
  using NCTV = std::ranges::subrange<std::tuple<int, int>*, SentT>;
  using IV = std::ranges::basic_istream_view<int, char>;
  using MDS = std::mdspan<int, std::dextents<std::size_t, 2>>;
  // exposition-only aliases, as [ranges.syn], [associative.general] and [container.alloc.reqmts] define them
  template<bool Const, class T> using maybe_const = std::conditional_t<Const, const T, T>;
  template<class I> using iter_key_type = std::remove_const_t<std::tuple_element_t<0, std::iter_value_t<I>>>;
  template<class I> using iter_mapped_type = std::tuple_element_t<1, std::iter_value_t<I>>;
  template<class I> using iter_to_alloc_type =
    std::pair<const std::tuple_element_t<0, std::iter_value_t<I>>, std::tuple_element_t<1, std::iter_value_t<I>>>;
  template<class R> using range_key_type = std::remove_const_t<typename std::ranges::range_value_t<R>::first_type>;
  template<class R> using range_mapped_type = typename std::ranges::range_value_t<R>::second_type;
  template<class R> using range_to_alloc_type =
    std::pair<const typename std::ranges::range_value_t<R>::first_type, typename std::ranges::range_value_t<R>::second_type>;
}}
'''


def prelude(sec):
    inc = '\n'.join(f'#include <{h}>' for h in HEADERS)
    return PRELUDE.format(sec=sec, includes=inc)


# the headers whose feature-test macros this part checks ([version.syn])
MACRO_HEADERS = {'algorithm', 'array', 'deque', 'flat_map', 'flat_set', 'forward_list', 'generator', 'hive',
                 'inplace_vector', 'iterator', 'list', 'map', 'mdspan', 'numeric', 'queue', 'ranges', 'set', 'span',
                 'stack', 'string', 'string_view', 'unordered_map', 'unordered_set', 'vector', 'cstring'}

FREESTANDING_HEADERS = {'array', 'inplace_vector', 'span', 'mdspan', 'iterator', 'ranges', 'algorithm', 'numeric',
                        'string_view', 'string', 'cstring', 'memory'}


def header_probes(g, ents, outdir):
    """Per-header probes: each synopsis's names with only its header included (aspect `header`),
    the headers its synopsis #includes, and the freestanding ones with -ffreestanding (`freestanding`)."""
    by_sec = {}
    for clause, sec, d in ents:
        by_sec.setdefault(sec, []).append(d)
    ids = {}
    for r in g.rows:
        ids.setdefault((r[1], r[3], r[4]), r[0])
    for sec, hdr in SECTION_HEADER.items():
        if sec not in by_sec:
            continue
        lines, fs_lines = [], []
        mostly_fs = False
        for d in by_sec[sec]:
            if d.kind == 'pp' or d.cls:
                continue
            if d.kind not in ('alias', 'class', 'enum', 'concept', 'variable', 'function', 'namespace-alias'):
                continue
            if not d.name or d.name.startswith(('@', 'operator', '{')):
                continue
            q = d.ns + '::' + d.name
            i = ids.get((sec, q, ' '.join(d.text.split())))
            if not i:
                continue
            if d.kind == 'namespace-alias':
                line = f'namespace {i}_h = {q};'
            else:
                line = f'namespace {i}_h {{ using {q}; }}'
            lines.append(f'{line} // @{i} header')
            note = d.comment
            fs = ('freestanding' in note and 'freestanding-deleted' not in note) or \
                 (hdr in FREESTANDING_HEADERS and 'hosted' not in note and 'freestanding-deleted' not in note
                  and sec not in ('iterator.synopsis', 'string.syn', 'cstring.syn'))
            if fs and hdr in FREESTANDING_HEADERS:
                fs_lines.append(f'{line.replace(i + "_h", i + "_f")} // @{i} freestanding')
        for (hh, inc_id, inc, rep) in [(h, a, b, c) for h, l in g.header_includes.items() for (a, b, c) in l if h == hdr]:
            if rep.startswith('expr:'):
                lines.append(f'static_assert({rep[5:]}); // @{inc_id} include')
            else:
                lines.append(f'namespace {inc_id}_h {{ using t = {rep}; }} // @{inc_id} include')
        head = (f'// Generated by tools/spec_audit/gen_probes.py; do not edit. Names of [{sec}] with only <{hdr}> included.\n'
                f'#include <{hdr}>\n')
        with open(os.path.join(outdir, sec + '.header.cpp'), 'w', encoding='utf-8') as o:
            o.write(head + '\n'.join(lines) + '\n')
        if fs_lines:
            for r in g.rows:
                pass
            with open(os.path.join(outdir, sec + '.freestanding.cpp'), 'w', encoding='utf-8') as o:
                o.write(f'// Generated by tools/spec_audit/gen_probes.py; do not edit. Freestanding names of [{sec}]\n'
                        f'// (-ffreestanding, [compliance]).\n#include <{hdr}>\n' + '\n'.join(fs_lines) + '\n')
