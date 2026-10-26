# Maintainer: vitalikr2x3 <you@example.com>
pkgname=lyrics-term
pkgver=1.1.1
pkgrel=1
pkgdesc='Terminal synced lyrics for any MPRIS-compatible music player'
arch=('x86_64')
url="https://github.com/vitalikr2x3/lyrics-term"
license=('MIT')
options=('!debug')
depends=('dbus' 'curl' 'freetype2' 'nlohmann-json')
makedepends=('cmake' 'pkgconf')
optdepends=(
  'kitty: graphical renderer (--renderer kitty)'
  'noto-fonts: default font for the kitty renderer'
  'ttf-dejavu: fallback font for the kitty renderer'
)
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('3248d51ff06b34aad14ab091f518a81d6d46bfcc6a73b70eda8137c0438832a7')

build() {
  cmake -B build -S "$pkgname-$pkgver" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
