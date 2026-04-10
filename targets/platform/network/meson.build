lib_platform_network_stub = static_library('platform_network_stub',
    files('stub/StubNetworkPlayer.cpp', 'stub/StubPlatformNetwork.cpp'),
    include_directories: include_directories('../../'),
    dependencies: java_dep,
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_network_stub_dep = declare_dependency(
    link_with: lib_platform_network_stub,
)