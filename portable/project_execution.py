"""Build the portable execution headers and scheduler from the libycxx sources."""

import argparse
from pathlib import Path
import re


NATIVE_ABI = 'namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {'
PORTABLE_ABI = 'namespace ycxx { namespace __portable_std {'
NATIVE_PRIVATE = 'namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx {'
PORTABLE_PRIVATE = 'namespace __ycxx_portable_exec {'
PORTABLE = '::ycxx::__portable_std'
NAMES = (
    'execution', 'this_thread', 'forwarding_query', 'forwarding_query_t',
    'get_allocator', 'get_allocator_t', 'get_stop_token', 'get_stop_token_t',
    'stop_token_of_t', 'stop_callback_for_t', 'never_stop_token',
    'inplace_stop_source', 'inplace_stop_token', 'inplace_stop_callback',
    'stoppable_token', 'unstoppable_token',
)
HOST_HEADERS = frozenset({
    'atomic_base.hpp', 'concepts.hpp', 'coroutine.hpp', 'exception.hpp', 'exception_base.hpp',
    'exception_ptr.hpp', 'execution_policy.hpp', 'integer_sequence.hpp', 'invoke.hpp',
    'memory_base.hpp', 'move.hpp', 'optional.hpp', 'pair.hpp', 'shared_ptr.hpp',
    'span.hpp', 'stop_token.hpp', 'tuple.hpp', 'type_traits.hpp', 'utility_base.hpp', 'variant.hpp',
})
NATIVE_NAMES = re.compile(r'(?:::)?\bstd::(' + '|'.join(NAMES) + r')\b')


def project_std_block(match):
    body = match[1]
    if re.search(r'^struct (?:common_type|tuple_size|tuple_element)<', body, re.M):
        # These specializations customize the host's templates for our types.
        body = re.sub(r'(?<!:)\bexecution::', f'{PORTABLE}::execution::', body)
        return 'namespace std { extern "C++" {' + body
    return PORTABLE_ABI + body


def project(source):
    """Relocate the execution declarations while preserving the native implementation bodies."""
    source = re.sub(
        re.escape(NATIVE_ABI) + r'(?=\n)([\s\S]*?^}} // namespace std\n)',
        project_std_block, source, flags=re.M,
    )
    source = source.replace(NATIVE_ABI, PORTABLE_ABI)
    source = source.replace(NATIVE_PRIVATE, PORTABLE_PRIVATE)
    source = re.sub(r'\b__ycxx(?=::)', '__ycxx_portable_exec', source)
    source = NATIVE_NAMES.sub(lambda match: f'{PORTABLE}::{match[1]}', source)
    return source.replace('::ycxx_pal_thread_yield()', '::std::this_thread::yield()')


def project_header(source):
    def include(match):
        header = match[1]
        if header == 'execution_wait.hpp':
            return '#include <ycxx/execution/detail/wait.hpp>\n'
        if header and header.startswith('exec_'):
            return f'#include <ycxx/execution/detail/{header}>\n'
        if header is None or header in HOST_HEADERS:
            # The portable umbrella supplies the corresponding host standard headers.
            return ''
        raise ValueError(f'portable execution has no adapter for ycxx/core/{header}')

    source = re.sub(r'^#include <ycxx/(?:config\.hpp|core/([^>]+))>\n', include, source, flags=re.M)
    return project(source)


def write(path, source):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('// Generated from libycxx sources by portable/project_execution.py.\n' + source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    core = args.source / 'include/ycxx/core'
    headers = args.output / 'include/ycxx/execution/detail'
    for header in sorted(core.glob('exec_*.hpp')):
        write(headers / header.name, project_header(header.read_text()))

    hosted = args.source / 'src/hosted'
    backend = (hosted / 'parallel_scheduler.cppm').read_text()
    linkage = '\nextern "C++" {\n'
    backend = linkage + backend.split(linkage, 1)[1]
    backend = backend.replace(
        'namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __abi {\n'
        'std::exception_ptr __parallel_scheduler_bad_alloc() noexcept;\n}}\n\n',
        '',
    ).replace('::__ycxx::__abi::__parallel_scheduler_bad_alloc()', 'std::make_exception_ptr(std::bad_alloc{})')
    preamble = '#include <ycxx/execution/execution.hpp>\n#include <condition_variable>\n#include <vector>\n'
    write(args.output / 'parallel_scheduler.cpp', preamble + project(backend))

    hook = (hosted / 'parallel_scheduler_query.cpp').read_text()
    hook = hook.replace('#include <execution>', '#include <ycxx/execution/execution.hpp>')
    write(args.output / 'parallel_scheduler_query.cpp', project(hook))


if __name__ == '__main__':
    main()
