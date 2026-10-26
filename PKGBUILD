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
sha256sums=('99bea52f6401f5355f32d1a59b5831ee0c454a332a5b92c67df406622acc511c')

build() {
  arch-meson "$pkgname-$pkgver" build
  meson compile -C build
}

package() {
  meson install -C build --destdir "$pkgdir"
}
