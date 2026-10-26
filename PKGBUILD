# Maintainer: USERNAME <you@example.com>
pkgname=lyrics-term
pkgver=1.0.0
pkgrel=1
pkgdesc='Terminal synced lyrics for any MPRIS-compatible music player'
arch=('x86_64')
url="https://github.com/USERNAME/lyrics-term"
license=('MIT')
depends=('dbus' 'curl' 'freetype2' 'nlohmann-json')
makedepends=('cmake' 'pkgconf')
optdepends=(
  'kitty: graphical renderer (--renderer kitty)'
  'noto-fonts: default font for the kitty renderer'
  'ttf-dejavu: fallback font for the kitty renderer'
)
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
  cmake -B build -S "$pkgname-$pkgver" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
  cmake --build build
}

package() {
  DESTDIR="$pkgdir" cmake --install build
}
