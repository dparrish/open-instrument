#!/bin/bash

mkdir -p deps
mkdir -p static

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

if [ ! -d deps/gflags ]; then
  cd deps
  svn checkout http://gflags.googlecode.com/svn/trunk/ gflags || exit $?
  cd gflags
  ./configure && make && sudo make install || exit $?
  cd ../..
fi

if [ ! -d deps/glog ]; then
  cd deps
  svn checkout http://google-glog.googlecode.com/svn/trunk/ glog || exit $?
  cd glog
  ./configure && make && sudo make install || exit $?
  cd ../..
fi

if [ ! -d deps/sympy ]; then
  cd deps
  git clone git://github.com/sympy/sympy.git || exit $?
  cd sympy
  sudo python setup.py install
  cd ../..
fi
