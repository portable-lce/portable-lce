lib_platform_input_sdl2 = static_library('platform_input_sdl2',
    files('sdl2/SDL2Input.cpp'),
    dependencies: dependency('sdl2'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_input_sdl2_dep = declare_dependency(
    link_with: lib_platform_input_sdl2,
)
