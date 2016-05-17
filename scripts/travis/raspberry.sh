#! /bin/bash
set -xe
scons -Q clean
SYSROOT="/opt/raspberry/arm-bcm2708/arm-bcm2708-linux-gnueabi/arm-bcm2708-linux-gnueabi/sysroot"
for v in debug release
do
  PATH="/opt/raspberry/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin:${PATH}" \
    scons -Q --enable-werror --with-3rdparty=uv,openfec,sox,cpputest \
      host=arm-bcm2708hardfp-linux-gnueabi variant=$v

  for t in bin/arm-bcm2708hardfp-linux-gnueabi/roc-test-*
  do
    LD_LIBRARY_PATH="${SYSROOT}/lib" qemu-arm -L "${SYSROOT}" -cpu arm1176 $t # armv6
  done
done
