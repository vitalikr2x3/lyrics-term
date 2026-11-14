project('lyrics-term', 'cpp',
  version : '3.0.1',
  default_options : [
    'cpp_std=gnu++17',  # popen/pclose and POSIX calls need the GNU extensions on Linux and macOS
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
threads_dep  = dependency('threads')

# One source set per platform: the player and the terminal
host = host_machine.system()
if host == 'linux'
  player_src = ['src/platform_posix.cpp', 'src/player_linux.cpp']
  player_deps = [dependency('dbus-1')]
elif host == 'darwin'
  player_src = ['src/platform_posix.cpp', 'src/player_macos.cpp']
  player_deps = []
elif host == 'windows'
  player_src = ['src/platform_windows.cpp', 'src/player_windows.cpp']
  player_deps = [meson.get_compiler('cpp').find_library('windowsapp')]
else
  error('unsupported platform: ' + host)
endif

executable(
  'lyrics-term',
  ['src/main.cpp', 'src/paths.cpp'] + player_src,
  dependencies : [curl_dep, freetype_dep, json_dep, threads_dep] + player_deps,
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
