# Contributor: Esmil <esmil@mailme.dk>

pkgname=lua-csv
pkgver=$(date +%Y%m%d)
pkgrel=1
pkgdesc='A CSV parser for Lua'
arch=('i686' 'x86_64')
url='http://github.com/esmil/lua-csv'
license=('GPL')
depends=('lua')

build() {
  cd ${srcdir}
  ln -s .. $pkgname
  cd $pkgname

  make
}

package() {
  cd ${srcdir}/$pkgname

  make PREFIX=/usr DESTDIR=${pkgdir} install
}
