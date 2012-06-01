#!/bin/bash

# Download and compile protojs
if [ ! -d static/protojs ]; then
  cd static
  git clone https://github.com/sirikata/protojs.git protojs || exit $?
  cd protojs
  ./bootstrap.sh || exit $?
  make
  cd ..
fi
cd static/protojs
git pull
make
cd ../..


# Download flot (javascript graphing library)
if [ ! -d static/flot ]; then
  cd static
  svn checkout http://flot.googlecode.com/svn/trunk/ flot || exit $?
  cd ..
fi
cd static/flot
svn update
cd ../..

if [ ! -d static/gflags ]; then
  cd static
  svn checkout http://gflags.googlecode.com/svn/trunk/ gflags || exit $?
  cd gflags
  ./configure && make || exit $?
  cd ../..
fi
