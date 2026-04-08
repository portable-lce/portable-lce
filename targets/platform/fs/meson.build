lib_platform_fs_std = static_library(
  'platform_fs_std',
  files('std/StdFilesystem.cpp'),
  include_directories: [platform_inc, include_directories('../..')],
  dependencies: [_threads],
  cpp_args: global_cpp_args + global_cpp_defs,
)

fs_dep = declare_dependency(
  link_with: lib_platform_fs_std,
  include_directories: [platform_inc],
)
