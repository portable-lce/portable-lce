nbt_sources = files(
    'src/NbtIo.cpp',
    'src/Tag.cpp',
)

lib_nbt = static_library(
    'nbt',
    nbt_sources,
    dependencies: [java_dep],
    include_directories: include_directories('include'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

nbt_dep = declare_dependency(
    link_with: lib_nbt,
    include_directories: include_directories('include'),
)
