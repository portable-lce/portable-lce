lib_platform_sound_miniaudio = static_library(
    'platform_sound_miniaudio',
    files('miniaudio/MiniaudioSound.cpp'),
    dependencies: dependency('miniaudio'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_sound_miniaudio_dep = declare_dependency(
    link_with: lib_platform_sound_miniaudio,
)
