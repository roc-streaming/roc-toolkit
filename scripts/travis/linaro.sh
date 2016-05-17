#! /bin/bash
set -xe
scons -Q clean
for v in debug release
do
  PATH="/opt/linaro/bin:${PATH}" \
    scons -Q --enable-werror --with-3rdparty=uv,openfec,sox,cpputest \
      host=arm-linux-gnueabihf variant=$v

  for t in bin/arm-linux-gnueabihf/roc-test-*
  do
    LD_LIBRARY_PATH="/opt/linaro/arm-linux-gnueabihf/libc/lib" \
      qemu-arm -L "${LD_LIBRARY_PATH}" -cpu cortex-a15 $t # armv7
  done
done
