# Maintainer: vitalikr2x3 <vitalikr2x3@users.noreply.github.com>
pkgname=lyrics-term
pkgver=2.0.0
pkgrel=1
pkgdesc='Terminal synced lyrics for any MPRIS-compatible music player'
arch=('x86_64')
url="https://github.com/vitalikr2x3/lyrics-term"
license=('MIT')
options=('!debug')
depends=('dbus' 'curl' 'freetype2' 'nlohmann-json')
makedepends=('meson' 'ninja' 'pkgconf')
optdepends=(
  'kitty: graphical renderer (--renderer kitty)'
  'noto-fonts: default font for the kitty renderer'
  'ttf-dejavu: fallback font for the kitty renderer'
  'mpv-mpris: MPRIS support for mpv'
)
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')  # run updpkgsums once the v2.0.0 tag exists

build() {
  arch-meson "$pkgname-$pkgver" build
  meson compile -C build
}

check() {
  meson test -C build --print-errorlogs
}

package() {
  meson install -C build --destdir "$pkgdir"
}
