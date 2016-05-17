#! /bin/bash
set -xe
scons -Q clean
for v in debug release
do
  PATH="/opt/raspberry/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin:${PATH}" \
    scons -Q --enable-werror --with-3rdparty=uv,openfec,sox,cpputest \
      host=arm-bcm2708hardfp-linux-gnueabi variant=$v

  for t in bin/arm-bcm2708hardfp-linux-gnueabi/roc-test-*
  do
    LD_LIBRARY_PATH="/opt/raspberry/arm-bcm2708/arm-bcm2708-linux-gnueabi/arm-bcm2708-linux-gnueabi/sysroot/lib" \
      qemu-arm -L "${LD_LIBRARY_PATH}" -cpu arm1176 $t # armv6
  done
done
