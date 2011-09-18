#!/bin/bash

if [ ! -d static/protojs ]; then
  pushd static
  git clone https://github.com/sirikata/protojs.git protojs
  cd protojs
  ./bootstrap.sh
  make
  popd
fi

if [ ! -d static/flot ]; then
  pushd static
  svn checkout http://flot.googlecode.com/svn/trunk/ flot
  popd
fi
