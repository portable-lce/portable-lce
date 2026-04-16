# MARK: build configuration

project(
    'portable-lce',
    ['cpp', 'c'],
    version: '0.1.0',
    meson_version: '>= 1.3',
    default_options: [
        'cpp_std=c++20',
        'warning_level=0',
        'unity=on',  # merge source files per target
        'unity_size=8',  # TODO: mess around with this
        'buildtype=debug',
        'force_fallback_for=[\'miniaudio\',\'stb\']',
        'b_pch=true',  # precompiled headers
        'default_library=static',  # static linkage to subprojects
            # 'b_lto=true', # link-time optimisation (ThinLTO under clang+lld)
            # 'b_ndebug=if-release', # drop assert() in --buildtype=release
    ],
)

# there is a little hack for SDL2 on darwin... upgrading to SDL3 will make this irrelevant
if host_machine.system() == 'darwin'
    add_languages('objc', native: false)
endif
# This makes sure unity builds work on MSVC
if meson.get_compiler('cpp').get_id() == 'msvc'
    add_project_arguments('/bigobj', language: 'cpp')
endif

pymod = import('python')
python = pymod.find_installation('python3', required: true)

cc = meson.get_compiler('cpp')

global_cpp_args = []
global_cpp_defs = [
    '-DSPLIT_SAVES',
    '-D_LARGE_WORLDS',
    '-D_EXTENDED_ACHIEVEMENTS',
    '-DMULTITHREAD_ENABLE',  # always-on threading flag (formerly in App_Defines.h)
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

if get_option('buildtype') == 'debugoptimized'
    global_cpp_defs += ['-D_FORTIFY_SOURCE=2']
endif

# MARK: meson options

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

if get_option('occlusion_culling') == 'off'
    global_cpp_defs += ['-DOCCLUSION_MODE_NONE']
elif get_option('occlusion_culling') == 'frustum'
    global_cpp_defs += ['-DOCCLUSION_MODE_FRUSTUM']
elif get_option('occlusion_culling') == 'bfs'
    global_cpp_defs += ['-DOCCLUSION_MODE_BFS', '-DUSE_OCCLUSION_CULLING']
elif get_option('occlusion_culling') == 'hardware'
    global_cpp_defs += ['-DOCCLUSION_MODE_HARDWARE', '-DUSE_OCCLUSION_CULLING']
endif

add_project_arguments(global_cpp_defs, language: ['cpp', 'c'])
add_project_arguments(global_cpp_args, language: 'cpp')

subdir('targets/util')
subdir('targets/java')
subdir('targets/nbt')
subdir('targets/platform')

# MARK: platform configuration

if host_machine.system() in ['linux', 'windows', 'darwin']
    platform_fs_dep = platform_fs_std_dep
    platform_game_dep = platform_game_stub_dep
    platform_input_dep = platform_input_sdl2_dep
    platform_leaderboard_dep = platform_leaderboard_stub_dep
    platform_network_dep = platform_network_stub_dep
    platform_profile_dep = platform_profile_stub_dep
    platform_renderer_dep = platform_renderer_gl_dep
    platform_sound_dep = platform_sound_miniaudio_dep
    platform_storage_dep = platform_storage_stub_dep
    platform_thread_dep = platform_thread_dep  # standardized backend (for now)

    app_platform_sources = files('targets/app/desktop/main.cpp')
endif

subdir('targets/resources')
subdir('targets/minecraft')
subdir('targets/app')
