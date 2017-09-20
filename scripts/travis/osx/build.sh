#! /bin/bash
set -xe

brew tap Homebrew/bundle
brew bundle --file=scripts/travis/osx/Brewfile

scons -Q clean
scons -Q --enable-werror --enable-sanitizers --with-3rdparty=openfec,cpputest \
    variant=debug test

scons -Q clean
scons -Q --enable-werror --with-3rdparty=openfec,cpputest variant=release test
