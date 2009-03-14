#!/bin/sh -x
libtoolize --copy --force
aclocal
autoheader
automake --add-missing
autoreconf --install
