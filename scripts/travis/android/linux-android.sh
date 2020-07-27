#! /bin/bash
set -euxo pipefail

[ "$ABI" = "armv7a" ] && host=$ABI-linux-androideabi || host=$ABI-linux-android
sysroot="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/$host"

case "$ABI" in
    "x86_64")
        arch=$ABI
        ;;
    "i686")
        arch=x86
        ;;
    "aarch64")
        arch=arm64-v8a
        ;;
    "armv7a")
        arch=armeabi-v7a
        sysroot="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi"
        ;;
    *) echo "unrecognized android ABI."; exit 1;;
esac

scons -Q clean

scons -Q \
    --enable-werror \
    --disable-tools \
    --enable-tests \
    --build-3rdparty=libuv,openfec,speexdsp,cpputest \
    --compiler=clang \
    --host=$host$API

# skip testing on ARM based platforms
[ "$ABI" = "aarch64" ] || [ "$ABI" = "armv7a" ] && exit 0

target_prefix="/data/local/tmp"

device create --name roc-device-$host$API --image default --api $API --arch $arch
device start --name roc-device-$host$API

adb shell "su 0 mkdir -p ${target_prefix}/lib"
adb push "$sysroot/libc++_shared.so" $target_prefix/lib/libc++_shared.so 1> /dev/null

readarray -d '' tests < <(find bin/$host$API \
        -name 'roc-test-*' -not -name 'roc-test-library' -print0)

for t in "${tests[@]}"; do
    filename=$(basename "${t}")
    adb push $t $target_prefix/$filename 1> /dev/null
    python2 scripts/wrappers/timeout.py 300 \
        adb shell "LD_LIBRARY_PATH=${target_prefix}/lib" \
            "${target_prefix}/${filename}"
done