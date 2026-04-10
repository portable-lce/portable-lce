lib_platform_leaderboard_stub = static_library('platform_leaderboard_stub',
    files('stub/StubLeaderboard.cpp'),
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_leaderboard_stub_dep = declare_dependency(
    link_with: lib_platform_leaderboard_stub,
)
