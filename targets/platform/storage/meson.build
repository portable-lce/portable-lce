lib_platform_storage_stub = static_library(
  'platform_storage_stub',
  files('stub/StubStorage.cpp'),
  include_directories: [platform_inc, include_directories('../..')],
  dependencies: [_threads],
  cpp_args: global_cpp_args + global_cpp_defs,
)

storage_dep = declare_dependency(
  link_with: lib_platform_storage_stub,
  include_directories: [platform_inc],
)
