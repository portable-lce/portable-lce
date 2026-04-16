lib_platform_game_stub = static_library(
    'platform_game_stub',
    files('stub/StubPlatformGame.cpp'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_game_stub_dep = declare_dependency(
    link_with: lib_platform_game_stub,
)
