lib_platform_fs_std = static_library('platform_fs_std',
    files('std/StdFilesystem.cpp'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_fs_std_dep = declare_dependency(
    link_with: lib_platform_fs_std,
)