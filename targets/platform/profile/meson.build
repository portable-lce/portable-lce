platform_profile_stub_sources = files('stub/StubProfile.cpp')

lib_platform_profile_stub = static_library('platform_profile_stub',
    files('stub/StubProfile.cpp'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_profile_stub_dep = declare_dependency(
    link_with: lib_platform_profile_stub,
)