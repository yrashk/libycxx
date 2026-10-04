# libycxx's own conformance tests, written only from the working draft and cppreference.com.
#   tools/run-conformance ycxx gcc|clang [subdir...]
import os, sys

repo = lit_config.params['repo']
compiler = lit_config.params.get('compiler', 'clang')
sanitizer = lit_config.params.get('sanitizer', '')

config.name = f'libycxx-own-{compiler}'
config.test_source_root = os.path.join(repo, 'tests', 'ycxx')
config.test_exec_root = os.path.join(repo, 'build', f'lit-ycxx-{compiler}' + (f'-{sanitizer}' if sanitizer else ''))
config.suffixes = ['.cpp']
config.excludes = ['support']

sys.path.insert(0, os.path.join(repo, 'tests'))
from ycxxlit.ycxx_format import YcxxFormat

flags = ['-I' + os.path.join(repo, 'tests', 'ycxx', 'support'), '-Wall', '-Wextra']
if sanitizer:
    flags += ['-fsanitize=' + ','.join({'asan': 'address', 'ubsan': 'undefined'}[s] for s in sanitizer.split(',')),
              '-fno-sanitize-recover=all', '-g']
config.test_format = YcxxFormat(os.path.join(repo, 'tools', 'ycxx-cxx'), compiler, flags)
