project('lyrics-term', 'cpp',
  version : '2.0.0',
  default_options : [
    'cpp_std=gnu++17',  # popen/pclose need the GNU/POSIX extensions
    'warning_level=3',
    'buildtype=release',
  ],
)

dbus_dep     = dependency('dbus-1')
curl_dep     = dependency('libcurl')
freetype_dep = dependency('freetype2')
json_dep     = dependency('nlohmann_json', version : '>=3.2')
threads_dep  = dependency('threads')

executable(
  'lyrics-term',
  'src/main.cpp',
  dependencies : [dbus_dep, curl_dep, freetype_dep, json_dep, threads_dep],
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
    'tests/test_lrc.cpp',
    include_directories : include_directories('src'),
    dependencies : [curl_dep, json_dep, threads_dep],
  )
  test('lrc', test_exe)
endif
