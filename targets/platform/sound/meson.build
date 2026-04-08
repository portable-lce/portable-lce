_miniaudio_dep = dependency('miniaudio')

lib_platform_sound_miniaudio = static_library(
  'platform_sound_miniaudio',
  files('miniaudio/MiniaudioSound.cpp'),
  include_directories: [platform_inc, include_directories('../..')],
  dependencies: [_threads, _miniaudio_dep],
  cpp_args: global_cpp_args + global_cpp_defs,
)

sound_dep = declare_dependency(
  link_with: lib_platform_sound_miniaudio,
  include_directories: [platform_inc],
  dependencies: [_miniaudio_dep],
)
