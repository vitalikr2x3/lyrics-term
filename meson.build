project('lyrics-term', ['c', 'cpp'],
  version : '3.1.4',
  default_options : [
    'cpp_std=c++17',
    'warning_level=3',
    'buildtype=release',
  ],
)

curl_dep     = dependency('libcurl')
freetype_dep = dependency('freetype2')
cxx = meson.get_compiler('cpp')
json_dep = dependency('nlohmann_json', version : '>=3.2', required : false)
if not json_dep.found()
  # nlohmann-json is header-only: accept the header even when pkg-config does not know the package
  if not cxx.has_header('nlohmann/json.hpp')
    error('nlohmann-json not found: install it (Arch: sudo pacman -S nlohmann-json; macOS: brew install nlohmann-json)')
  endif
  json_dep = declare_dependency()
endif

# ckdl (KDL parser): built by subprojects/packagefiles/ckdl/meson.build through the wrap file
ckdl_sub = subproject('ckdl')
ckdl_dep = ckdl_sub.get_variable('ckdl_dep')
add_project_arguments('-DKDL_STATIC_LIB', language : 'cpp')
threads_dep  = dependency('threads')

# One source set per platform: the player and the terminal
host = host_machine.system()
if host == 'linux'
  player_src = ['src/platform_posix.cpp', 'src/player_linux.cpp']
  player_deps = [dependency('dbus-1')]
elif host == 'windows'
  player_src = ['src/platform_windows.cpp', 'src/player_windows.cpp']
  player_deps = [meson.get_compiler('cpp').find_library('windowsapp')]
  # windows.h defines min/max macros, which break std::min and std::max
  add_project_arguments(['-DNOMINMAX', '-D_CRT_SECURE_NO_WARNINGS'], language : 'cpp')
else
  error('unsupported platform: ' + host + ' (supported: linux, windows)')
endif

executable(
  'lyrics-term',
  ['src/main.cpp', 'src/paths.cpp', 'src/config.cpp'] + player_src,
  dependencies : [curl_dep, freetype_dep, json_dep, threads_dep, ckdl_dep] + player_deps,
  install : true,
)

install_data(
  'LICENSE', 'README.md',
  install_dir : get_option('datadir') / 'doc' / 'lyrics-term',
)
install_man('man/lyrics-term.1')

if get_option('tests')
  test_exe = executable(
    'test_lrc',
    ['tests/test_lrc.cpp', 'src/paths.cpp'],
    include_directories : include_directories('src'),
    dependencies : [curl_dep, json_dep, threads_dep],
  )
  test('lrc', test_exe)
endif
