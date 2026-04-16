lib_platform_thread = static_library(
    'platform_thread',
    files('C4JThread.cpp', 'ShutdownManager.cpp'),
    dependencies: [dependency('threads')],
    include_directories: include_directories('../../'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

platform_thread_dep = declare_dependency(
    link_with: lib_platform_thread,
)
