java_sources = files(
    'src/Buffer.cpp',
    'src/ByteBuffer.cpp',
    'src/Class.cpp',
    'src/Color.cpp',
    'src/File.cpp',
    'src/FloatBuffer.cpp',
    'src/InputOutputStream/BufferedOutputStream.cpp',
    'src/InputOutputStream/BufferedReader.cpp',
    'src/InputOutputStream/ByteArrayInputStream.cpp',
    'src/InputOutputStream/ByteArrayOutputStream.cpp',
    'src/InputOutputStream/DataInputStream.cpp',
    'src/InputOutputStream/DataOutputStream.cpp',
    'src/InputOutputStream/FileInputStream.cpp',
    'src/InputOutputStream/FileOutputStream.cpp',
    'src/InputOutputStream/InputStream.cpp',
    'src/InputOutputStream/InputStreamReader.cpp',
    'src/IntBuffer.cpp',
    'src/JavaMath.cpp',
    'src/Random.cpp',
    'src/system.cpp',
    # 'src/Socket.cpp',
)

lib_java = static_library(
    'java',
    java_sources,
    dependencies: [util_dep],
    # TODO: remove the .. path once arrayWithLength are evaporated
    include_directories: include_directories('include', '..'),
    cpp_args: global_cpp_args + global_cpp_defs,
)

java_dep = declare_dependency(
    link_with: lib_java,
    include_directories: include_directories('include'),
)
