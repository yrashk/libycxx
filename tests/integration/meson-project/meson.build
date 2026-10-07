# A Meson project (tests/integration/run.sh): meson setup <build> --native-file <libycxx>/share/libycxx/meson-native.ini
project('ycxx_integration_meson', 'cpp', 'c', version: '1.0', default_options: ['cpp_std=c++17'])

cxx = meson.get_compiler('cpp')
# A configure-time check through the same compiler.
if not cxx.has_header('optional')
  error('no <optional>')
endif
threads = dependency('threads')
lib = shared_library('mgreet', 'lib.cpp', gnu_symbol_visibility: 'default')
exe = executable('app', 'main.cpp', 'cpart.c', link_with: lib, dependencies: threads)
test('app', exe)
