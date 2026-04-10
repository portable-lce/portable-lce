lib_util = static_library('util',
  files('StringHelpers.cpp', 'FrameProfiler.cpp'),
  dependencies: [dependency('simdutf')],
  include_directories : include_directories('.', '..'),
  cpp_args : global_cpp_args + global_cpp_defs,
)

util_dep = declare_dependency(
  link_with : lib_util,
  include_directories : include_directories('.'),
)
