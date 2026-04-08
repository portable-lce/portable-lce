project(
  '4jcraft',
  ['cpp', 'c'],
  version: '0.1.0',
  meson_version: '>= 1.3',
  default_options: [
    'cpp_std=c++23',
    'warning_level=0',
    'unity=on', # merge source files per target
    'unity_size=8', # TODO: mess around with this
    'b_pch=true', # precompiled headers
    'b_lto=true', # link-time optimisation (ThinLTO under clang+lld)
    'b_ndebug=if-release', # drop assert() in --buildtype=release
  ],
)

pymod = import('python')
python = pymod.find_installation('python3', required: true)

cc = meson.get_compiler('cpp')

global_cpp_defs = [
  '-DSPLIT_SAVES',
  '-D_LARGE_WORLDS',
  '-D_EXTENDED_ACHIEVEMENTS',
  '-D_FORTIFY_SOURCE=2',
]

# Debug-only defines: keep for debug + debugoptimized so iteration is unchanged.
# --buildtype=release strips them so shipping builds don't carry debug logging,
# debug menu UI, or assert-driven sanity checks.
if get_option('buildtype') in ['debug', 'debugoptimized']
  global_cpp_defs += [
    '-D_DEBUG',
    '-DDEBUG',
    '-D_DEBUG_MENUS_ENABLED',
  ]
endif

if host_machine.system() == 'linux'
  global_cpp_defs += ['-Dlinux', '-D__linux', '-D__linux__']
endif

if get_option('renderer') == 'gles'
  global_cpp_defs += ['-DGLES']
  gl_dep = dependency('glesv2', required: true)
  glu_dep = dependency('', required: false)
else
  gl_dep = dependency('gl', required: true)
  glu_dep = dependency('glu', required: true)
endif

if get_option('enable_vsync')
  global_cpp_defs += ['-DENABLE_VSYNC']
endif

if get_option('classic_panorama')
    global_cpp_defs += '-DCLASSIC_PANORAMA'
endif

if get_option('enable_frame_profiler')
  global_cpp_defs += ['-DENABLE_FRAME_PROFILER']
endif

if get_option('ui_backend') == 'shiggy'
  global_cpp_defs += ['-D_ENABLEIGGY']
endif

if get_option('ui_backend') == 'java'
  global_cpp_defs += '-DENABLE_JAVA_GUIS'
endif

add_project_arguments(global_cpp_defs, language: ['cpp', 'c'])

global_cpp_args = [
  '-Wshift-count-overflow',
  '-pipe',
]
add_project_arguments(global_cpp_args, language: 'cpp')

sdl2_dep = dependency('sdl2')
thread_dep = dependency('threads')
dl_dep = cc.find_library('dl', required: true)
glm_dep = dependency('glm')

stb = subproject('stb').get_variable('stb_inc')
stb_dep = declare_dependency(include_directories: stb)
simdutf_dep = dependency('simdutf',
  fallback: ['simdutf', 'simdutf_dep'],
  default_options: ['utf8=true', 'utf16=true', 'utf32=true']
)
miniaudio_dep = dependency('miniaudio')

subdir('targets/util')
subdir('targets/java')
subdir('targets/nbt')
subdir('targets/platform')

subdir('targets/resources')
subdir('targets/minecraft')
subdir('targets/app')
