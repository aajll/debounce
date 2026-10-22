# SPDX-License-Identifier: MIT

project(
  'debounce',
  'c',
  version: '1.2.0',
  meson_version: '>=0.57.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
  license: 'MIT',
)

project_description = 'Generic saturating counter debounce primitive.'

# === Version ==================================================================
# include/debounce.h defines the version macros, so a copy-in build has them.
# Configuration fails when they differ from the project version.

fs = import('fs')
version_parts = meson.project_version().split('.')
version_macros = {
  'DEBOUNCE_VERSION_MAJOR': version_parts[0],
  'DEBOUNCE_VERSION_MINOR': version_parts[1],
  'DEBOUNCE_VERSION_PATCH': version_parts[2],
  'DEBOUNCE_VERSION': '"' + meson.project_version() + '"',
}
header_lines = []
foreach line : fs.read('include' / 'debounce.h').split('\n')
  header_lines += ' '.join(line.split())
endforeach
foreach name, value : version_macros
  if '#define @0@ @1@'.format(name, value) not in header_lines
    error('include/debounce.h must define @0@ as @1@'.format(name, value))
  endif
endforeach

# The header-only code compiles in each consumer's translation units, so the
# option defines travel with debounce_dep and debounce.pc.
debounce_args = []
atomicity = get_option('atomicity_mode')
if atomicity == 'c11'
  debounce_args += '-DDEBOUNCE_USE_C11_ATOMIC'
elif atomicity == 'volatile'
  debounce_args += '-DDEBOUNCE_USE_VOLATILE_ATOMIC'
endif

if get_option('enable_callbacks')
  debounce_args += '-DDEBOUNCE_ENABLE_CALLBACKS=1'
endif

public_headers = include_directories('include')

debounce_dep = declare_dependency(
  include_directories: [public_headers],
  compile_args: debounce_args,
)

meson.override_dependency('debounce', debounce_dep)

install_headers(
  'include/debounce.h',
  'include/debounce_conf.h',
  subdir: 'debounce',
)

pkg = import('pkgconfig')
pkg.generate(
  name: 'debounce',
  description: project_description,
  filebase: 'debounce',
  version: meson.project_version(),
  extra_cflags: debounce_args,
)

if get_option('build_tests')
  subdir('tests')
endif
