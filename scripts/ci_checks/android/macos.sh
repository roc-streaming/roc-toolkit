#!/usr/bin/env bash

set -euxo pipefail

export PATH="$ANDROID_HOME/ndk/${NDK_VERSION}/toolchains/llvm/prebuilt/darwin-x86_64/bin:$PATH"

brew install scons ragel gengetopt

scons -Q \
      --enable-werror \
      --enable-tests \
      --disable-soversion \
      --disable-tools \
      --disable-openssl \
      --build-3rdparty=libuv,openfec,speexdsp,cpputest \
      --compiler=clang \
      --host="$TOOLCHAIN"
