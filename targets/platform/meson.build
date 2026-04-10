platform_inc = include_directories('.')
_threads = dependency('threads')
_sdl2 = dependency('sdl2')

if get_option('renderer') == 'gles'
  _gl = dependency('glesv2', required: true)
  _defs = ['-DGLES']
else
  _gl = dependency('gl', required: true)
  _defs = []
endif

# Per-subsystem source lists. Each subdir owns its own list of backend
# .cpp files via a `platform_<name>_sources` variable. The library
# build is centralised so we get one library per platform target rather
# than one per subsystem; subsystems vary together (an SDL2 platform
# wants SDL2 input + GL renderer + miniaudio together) and a per-
# subsystem split inflates link units without buying flexibility.
subdir('input')
subdir('profile')
subdir('storage')
subdir('fs')
subdir('renderer')
subdir('sound')
subdir('network')
subdir('leaderboard')
subdir('game')
subdir('thread')

# Core: thread + shutdown plumbing, no backend.
lib_platform_core = static_library('platform_core',
  platform_thread_sources,
  include_directories: [platform_inc, include_directories('..')],
  dependencies: [_threads],
  cpp_args: global_cpp_args + global_cpp_defs,
)

platform_dep = declare_dependency(
  link_with: lib_platform_core,
  include_directories: platform_inc,
)

lib_platform_sdl2 = static_library('platform_sdl2',
  platform_input_sources
    + platform_profile_sources
    + platform_storage_sources
    + platform_fs_sources
    + platform_renderer_sources
    + platform_sound_sources
    + platform_network_sources
    + platform_leaderboard_sources
    + platform_game_sources,
  include_directories: [platform_inc, include_directories('..')],
  dependencies: [_sdl2, _gl, _threads, glm_dep, stb_dep, java_dep,
                 miniaudio_dep],
  cpp_args: _defs + global_cpp_args + global_cpp_defs,
)

# Single dep for the whole platform_sdl2 library. Aliased per-subsystem
# so consumer meson files can keep asking for `input_dep` etc. without
# caring that they all resolve to the same library object today.
platform_sdl2_dep = declare_dependency(
  link_with: lib_platform_sdl2,
  include_directories: [platform_inc],
  dependencies: [_sdl2, _gl, _threads, glm_dep, miniaudio_dep],
)

input_dep = platform_sdl2_dep
profile_dep = platform_sdl2_dep
storage_dep = platform_sdl2_dep
fs_dep = platform_sdl2_dep
render_dep = platform_sdl2_dep
sound_dep = platform_sdl2_dep
network_dep = platform_sdl2_dep
leaderboard_dep = platform_sdl2_dep
game_dep = platform_sdl2_dep
