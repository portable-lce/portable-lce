lib_platform_storage_std = static_library('platform_storage_std',
    files('std/StdStorage.cpp'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_storage_std_dep = declare_dependency(
    link_with: lib_platform_storage_std,
)
