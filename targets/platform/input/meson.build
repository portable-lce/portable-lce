lib_platform_input_sdl2 = static_library(
  'platform_input_sdl2',
  files('sdl2/SDL2Input.cpp'),
  include_directories: [platform_inc, include_directories('../..')],
  dependencies: [_sdl2, _threads],
  cpp_args: global_cpp_args + global_cpp_defs,
)

input_dep = declare_dependency(
  link_with: lib_platform_input_sdl2,
  include_directories: [platform_inc],
  dependencies: [_sdl2],
)
