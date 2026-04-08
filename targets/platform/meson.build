platform_inc = include_directories('.')
_threads = dependency('threads')
_sdl2 = dependency('sdl2')

if get_option('renderer') == 'gles'
  _gl = dependency('glesv2', required: true)
  _defs = ['-DGLES']
else
  _gl = dependency('gl', required: true)
  _defs = []
endif

# Core: thread + shutdown plumbing, no backend.
lib_platform_core = static_library('platform_core',
  files('C4JThread.cpp', 'ShutdownManager.cpp'),
  include_directories: [platform_inc, include_directories('..')],
  dependencies: [_threads],
  cpp_args: global_cpp_args + global_cpp_defs,
)

platform_dep = declare_dependency(
  link_with: lib_platform_core,
  include_directories: platform_inc,
)

# Per-subsystem backends. Each lives in its own subdir and produces its
# own library + dep so consumers can ask for the subsystem they need
# without dragging the others in.
subdir('input')
subdir('profile')
subdir('storage')
subdir('fs')
subdir('renderer')
