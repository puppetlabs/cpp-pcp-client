#!/bin/bash
set -ev

get_gettext() {
  wget https://s3.amazonaws.com/kylo-pl-bucket/gettext-0.19.6_install.tar.bz2
  tar xjvf gettext-0.19.6_install.tar.bz2 --strip 1 -C $USERDIR
  rm -f ../locales/cpp-pcp-client.pot
  export MAKE_TARGET="all"
}

if [ ${TRAVIS_TARGET} == CPPCHECK ]; then
  # grab a pre-built cppcheck from s3
  wget https://s3.amazonaws.com/kylo-pl-bucket/pcre-8.36_install.tar.bz2
  tar xjvf pcre-8.36_install.tar.bz2 --strip 1 -C $USERDIR
  wget https://s3.amazonaws.com/kylo-pl-bucket/cppcheck-1.69_install.tar.bz2
  tar xjvf cppcheck-1.69_install.tar.bz2 --strip 1 -C $USERDIR
elif [ ${TRAVIS_TARGET} == DEBUG ]; then
  # Install coveralls.io update utility
  pip install --user cpp-coveralls
  get_gettext
else
  get_gettext
fi

# Generate build files
[ $1 == "debug" ] && export CMAKE_VARS="-DCMAKE_BUILD_TYPE=Debug -DCOVERALLS=ON"
[ $1 == "shared_release" ] && export CMAKE_VARS="-DBUILD_SHARED_LIBS=ON"
cmake $CMAKE_VARS .

if [ ${TRAVIS_TARGET} == CPPLINT ]; then
  make cpplint
elif [ ${TRAVIS_TARGET} == CPPCHECK ]; then
  make cppcheck
else
  make -j2
  make test ARGS=-V

  # Make sure installation succeeds
  mkdir dest
  make DESTDIR=`pwd`/dest install
fi

# If this is a release build, prepare an artifact for github
if [ ${TRAVIS_TARGET} == RELEASE ]; then
  cd dest/usr/local
  tar czvf $TRAVIS_BUILD_DIR/cpp-pcp-client.tar.gz `find . -type f -print`
fi
