# Maintainer: vitalikr2x3 <vitalikr2x3@users.noreply.github.com>
pkgname=lyrics-term
pkgver=3.1.14
pkgrel=1
pkgdesc='Terminal synced lyrics for MPRIS players (Linux) and the Windows media session'
arch=('x86_64')
url="https://github.com/vitalikr2x3/lyrics-term"
license=('MIT')
depends=('dbus' 'curl' 'freetype2' 'nlohmann-json')
makedepends=('meson' 'ninja' 'pkgconf' 'git')
optdepends=(
  'kitty: graphical renderer (--renderer kitty) and palette colors (--from-kitty)'
  'noto-fonts: default font for the kitty renderer'
  'ttf-dejavu: fallback font for the kitty renderer'
)
options=('!debug')
source=(
  "$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz"
  "ckdl-main.tar.gz::https://github.com/tjol/ckdl/archive/refs/heads/main.tar.gz"
)
sha256sums=('fddadec3a04def9a5cf712c9193fb4ea3deae0d12af6525b88b71b930a55ebef'
            '646edfd76a29155c435743d753c5e488f3b7a7a701232d5619c8953b4b901bcc')

prepare() {
  # ckdl is supplied as a source file: the build has no network access, so Meson cannot fetch it.
  # The Meson build file for ckdl ships with the project (subprojects/packagefiles/ckdl)
  cd "$pkgname-$pkgver"
  rm -rf subprojects/ckdl
  mv "$srcdir/ckdl-main" subprojects/ckdl
  cp subprojects/packagefiles/ckdl/meson.build subprojects/ckdl/meson.build
}

build() {
  cd "$pkgname-$pkgver"
  arch-meson build --buildtype=release
  meson compile -C build
}

package() {
  cd "$pkgname-$pkgver"
  meson install -C build --destdir "$pkgdir"
  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
