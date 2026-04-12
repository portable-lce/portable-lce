patch_sources = []
patch_deps = []

if host_machine.system() == 'darwin'
    patch_sources += files('sdl2/SDL2CursorPatch.m')
    patch_deps += dependency('appleframeworks', modules: ['AppKit'])
endif

lib_platform_input_sdl2 = static_library('platform_input_sdl2',
    files('sdl2/SDL2Input.cpp') + patch_sources,
    dependencies: [dependency('sdl2')] + patch_deps,
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_input_sdl2_dep = declare_dependency(
    link_with: lib_platform_input_sdl2,
)
