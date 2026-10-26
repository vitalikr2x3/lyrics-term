cmake_minimum_required(VERSION 3.16)
project(lyrics-term VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)  # popen/pclose need POSIX extensions

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS     REQUIRED IMPORTED_TARGET dbus-1)
pkg_check_modules(CURL     REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)
find_package(nlohmann_json 3.2 REQUIRED)
find_package(Threads REQUIRED)

add_executable(lyrics-term src/main.cpp)
target_compile_options(lyrics-term PRIVATE -Wall -Wextra)
target_link_libraries(lyrics-term PRIVATE
  PkgConfig::DBUS
  PkgConfig::CURL
  PkgConfig::FREETYPE
  nlohmann_json::nlohmann_json
  Threads::Threads
)

install(TARGETS lyrics-term RUNTIME DESTINATION bin)
install(FILES LICENSE README.md DESTINATION share/doc/lyrics-term)
