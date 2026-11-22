# Maintainer: vitalikr2x3 <vitalikr2x3@users.noreply.github.com>
pkgname=lyrics-term
pkgver=3.1.14
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
sha256sums=('fddadec3a04def9a5cf712c9193fb4ea3deae0d12af6525b88b71b930a55ebef')

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
