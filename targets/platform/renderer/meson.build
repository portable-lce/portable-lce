lib_platform_renderer_gl = static_library(
  'platform_renderer_gl',
  files('gl/GLRenderer.cpp', 'gl/render_stubs.cpp'),
  include_directories: [platform_inc, include_directories('../..')],
  dependencies: [_sdl2, _gl, _threads, glm_dep, stb_dep, java_dep],
  cpp_args: _defs + global_cpp_args + global_cpp_defs,
)

render_dep = declare_dependency(
  link_with: lib_platform_renderer_gl,
  include_directories: [platform_inc],
  dependencies: [_sdl2, _gl, _threads, glm_dep],
)
