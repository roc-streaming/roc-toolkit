#! /bin/bash
set -xe
TOOLCHAIN="arm-linux-gnueabihf"
SYSROOT="/opt/toolchains/${TOOLCHAIN}/${TOOLCHAIN}"
CPU="cortex-a15" # armv7
scons -Q clean
for v in debug release
do
  PATH="/opt/toolchains/${TOOLCHAIN}/bin:${PATH}" \
    scons -Q --enable-werror --with-3rdparty=uv,openfec,sox,cpputest \
      host=${TOOLCHAIN} variant=$v

  for t in bin/${TOOLCHAIN}/roc-test-*
  do
    LD_LIBRARY_PATH="${SYSROOT}/lib" qemu-arm -L "${SYSROOT}" -cpu ${CPU} $t
  done
done
