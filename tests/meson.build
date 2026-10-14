thread_dep = dependency('threads')

test_exe = executable(
  'test_debounce',
  files('test_debounce.c'),
  dependencies: [debounce_dep],
  include_directories: [public_headers, generated_headers],
)

test('debounce module', test_exe)

test_callbacks_exe = executable(
  'test_debounce_callbacks',
  files('test_debounce_callbacks.c'),
  dependencies: [debounce_dep],
  include_directories: [public_headers, generated_headers],
  c_args: ['-DDEBOUNCE_ENABLE_CALLBACKS=1'],
)

test('debounce callbacks', test_callbacks_exe)

# The concurrency test is the one that exercises shared-state access from two
# threads, so it is the meaningful place to run ThreadSanitizer.  TSAN is opt-in
# (-Dtsan=true) because it requires libtsan and is x86/host-only; the C2000
# target never builds the test suite.
concurrency_override = []
if get_option('tsan')
  concurrency_override = ['b_sanitize=thread']
endif

test_concurrency_exe = executable(
  'test_debounce_concurrency',
  files('test_debounce_concurrency.c'),
  dependencies: [debounce_dep, thread_dep],
  include_directories: [public_headers, generated_headers],
  override_options: concurrency_override,
)

test('debounce concurrency', test_concurrency_exe)

# Without an explicit flag the header picks C11 atomics, and falls back to
# volatile when the compiler defines __STDC_NO_ATOMICS__.
cc = meson.get_compiler('c')
probe_src = files('atomic_mode_probe.c')
foreach mode : [[[], 'C11'], [['-D__STDC_NO_ATOMICS__=1'], 'VOLATILE']]
  assert(
    cc.compiles(
      probe_src,
      args: mode[0] + ['-DDEBOUNCE_PROBE_EXPECT_MODE=DEBOUNCE_ATOMIC_MODE_' + mode[1]],
      include_directories: public_headers,
      name: 'debounce auto-detects ' + mode[1],
    ),
    'debounce.h must default to @0@ for @1@'.format(mode[1], mode[0]),
  )
endforeach

# The C11 mode rejects a target whose 8-bit and 16-bit atomics need a lock.
# Clang can compile for a 16-bit target without a cross toolchain, so the
# check is exercised here: C11 must fail with the debounce diagnostic, and
# volatile must compile.
clang = find_program('clang', required: false)
if clang.found()
  probe_args = [
    '--target=msp430',
    '-ffreestanding',
    '-std=c11',
    '-fsyntax-only',
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Werror',
    '-I' + (meson.project_source_root() / 'include'),
    meson.current_source_dir() / 'atomic_mode_probe.c',
  ]
  probe = run_command(clang, '-DDEBOUNCE_USE_C11_ATOMIC', probe_args, check: false)
  assert(
    probe.returncode() != 0 and probe.stderr().contains('debounce: uint16_t atomics are not')
    and probe.stderr().contains('debounce: bool atomics are not'),
    'DEBOUNCE_USE_C11_ATOMIC must reject a target without lock-free 8-bit and 16-bit atomics',
  )
  probe = run_command(clang, '-DDEBOUNCE_USE_VOLATILE_ATOMIC', probe_args, check: false)
  assert(
    probe.returncode() == 0,
    'DEBOUNCE_USE_VOLATILE_ATOMIC must compile on a 16-bit target: ' + probe.stderr(),
  )
endif
