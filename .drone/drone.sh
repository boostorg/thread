#!/bin/bash

# Copyright 2020 Rene Rivera, Sam Darwin
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.txt or copy at http://boost.org/LICENSE_1_0.txt)

set -e
export TRAVIS_BUILD_DIR=$(pwd)
export DRONE_BUILD_DIR=$(pwd)
export TRAVIS_BRANCH=$DRONE_BRANCH
export VCS_COMMIT_ID=$DRONE_COMMIT
export GIT_COMMIT=$DRONE_COMMIT
export REPO_NAME=$DRONE_REPO
export USER=$(whoami)
export CC=${CC:-gcc}
export PATH=~/.local/bin:/usr/local/bin:$PATH

if [ "$DRONE_JOB_BUILDTYPE" == "boost" ]; then

echo '==================================> INSTALL'

GIT_FETCH_JOBS=8
BOOST_BRANCH=develop
if [ "$TRAVIS_BRANCH" = "master" ]; then BOOST_BRANCH=master; fi
cd ..
git clone -b $BOOST_BRANCH --depth 1 https://github.com/boostorg/boost.git boost-root
cd boost-root
git submodule init tools/build
git submodule init libs/config
git submodule init tools/boostdep
git submodule update --jobs $GIT_FETCH_JOBS
mkdir -p libs/thread
cp -r $TRAVIS_BUILD_DIR/* libs/thread
python tools/boostdep/depinst/depinst.py --git_args "--jobs $GIT_FETCH_JOBS" thread
./bootstrap.sh
./b2 headers

echo '==================================> SCRIPT'

echo "using $TOOLSET : : $TRAVIS_COMPILER ;" > ~/user-config.jam
./b2 -j3 -l60 libs/thread/test${HEADERS_ONLY:+//test_self_contained_headers} toolset=$TOOLSET cxxstd=$CXXSTD

fi
