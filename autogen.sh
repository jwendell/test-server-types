#!/bin/sh -e

mkdir -p build-aux
autoreconf -fi;
rm -Rf autom4te*.cache;

./configure $*
