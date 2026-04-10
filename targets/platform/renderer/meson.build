platform_renderer_gl_dependencies = [
    dependency('sdl2'),
    dependency('glm'),
    dependency('stb'),
    java_dep,
]
platform_renderer_gl_defs = []

if get_option('renderer') == 'gles'
    platform_renderer_gl_dependencies += dependency('glesv2')
    platform_renderer_gl_defs = ['-DGLES']
else
    platform_renderer_gl_dependencies += dependency('gl')
    platform_renderer_gl_defs = []
endif

lib_platform_renderer_gl = static_library('platform_renderer_gl',
    files('gl/GLRenderer.cpp', 'gl/render_stubs.cpp'),
    include_directories: include_directories('../../'),
    dependencies: platform_renderer_gl_dependencies,
    cpp_args: platform_renderer_gl_defs + global_cpp_args + global_cpp_defs,
)

platform_renderer_gl_dep = declare_dependency(
    link_with: lib_platform_renderer_gl,
)
