lib_platform_storage_stub = static_library('platform_storage_stub',
    files('stub/StubStorage.cpp'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_storage_stub_dep = declare_dependency(
    link_with: lib_platform_storage_stub,
)
