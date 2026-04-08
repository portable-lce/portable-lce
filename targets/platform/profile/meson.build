lib_platform_profile_stub = static_library(
  'platform_profile_stub',
  files('stub/StubProfile.cpp'),
  include_directories: [platform_inc, include_directories('../..')],
  dependencies: [_threads],
  cpp_args: global_cpp_args + global_cpp_defs,
)

profile_dep = declare_dependency(
  link_with: lib_platform_profile_stub,
  include_directories: [platform_inc],
)
