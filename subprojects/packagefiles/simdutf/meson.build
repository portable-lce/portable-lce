project(
    'simdutf',
    'cpp',
    default_options: ['cpp_std=c++20'],
    version: '8.2.0',
    meson_version: '>= 1.1',
    license: ['MIT'],
)

pymod = import('python')
python = pymod.find_installation('python3', required: true)

simdutf_amalgamate_args = [
    '--no-zip',
    '--no-readme',
    '--source-dir',
    meson.project_source_root() / 'src',
    '--include-dir',
    meson.project_source_root() / 'include',
    '--output-dir',
    meson.project_build_root(),
]

if get_option('utf8')
    simdutf_amalgamate_args += ['--with-utf8']
endif
if get_option('utf16')
    simdutf_amalgamate_args += ['--with-utf16']
endif
if get_option('utf32')
    simdutf_amalgamate_args += ['--with-utf32']
endif
if get_option('ascii')
    simdutf_amalgamate_args += ['--with-ascii']
endif
if get_option('latin1')
    simdutf_amalgamate_args += ['--with-latin1']
endif
if get_option('base64')
    simdutf_amalgamate_args += ['--with-base64']
endif
if get_option('detect-enc')
    simdutf_amalgamate_args += ['--with-detect-enc']
endif

simdutf_amalgamate = custom_target(
    'amalgamate',
    input: [],
    output: ['simdutf.h', 'simdutf.cpp'],
    command: [
        python,
        files('singleheader' / 'amalgamate.py'),
    ] + simdutf_amalgamate_args,
)

simdutf_lib = static_library(
    'simdutf',
    sources: simdutf_amalgamate,
)

simdutf_dep = declare_dependency(
    include_directories: include_directories('./include'),
    link_with: simdutf_lib,
)

meson.override_dependency('simdutf', simdutf_dep)
