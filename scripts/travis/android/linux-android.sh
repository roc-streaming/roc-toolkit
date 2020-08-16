#! /bin/bash
set -euxo pipefail

case "$ABI" in
    "x86_64")
        arch=$ABI
        host=$ABI-linux-android
        sysroot="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/$host"
        ;;
    "i686")
        arch=x86
        host=$ABI-linux-android
        sysroot="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/$host"
        ;;
    "aarch64")
        arch=arm64-v8a
        host=$ABI-linux-android
        sysroot="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/$host"
        ;;
    "armv7a")
        arch=armeabi-v7a
        host=$ABI-linux-androideabi
        sysroot="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi"
        ;;
    *) echo "unrecognized android ABI."; exit 1;;
esac

scons -Q clean

scons -Q \
    --enable-werror \
    --disable-soversion \
    --disable-tools \
    --enable-tests \
    --build-3rdparty=libuv,openfec,speexdsp,cpputest \
    --compiler=clang \
    --host=$host$API

# skip testing on ARM based platforms
[ "$ABI" = "aarch64" ] || [ "$ABI" = "armv7a" ] && exit 0

target_prefix="/data/local/tmp"

emulator -accel-check

device create --name roc-device-$host$API --image default --api $API --arch $arch
device start --name roc-device-$host$API

adb shell "ip maddr"
adb shell "ip a"

adb shell "su 0 mkdir -p ${target_prefix}/lib"
adb push "$sysroot/libc++_shared.so" $target_prefix/lib/libc++_shared.so 1> /dev/null
adb push "bin/$host$API/libroc.so" $target_prefix/lib/libroc.so 1> /dev/null

readarray -d '' tests < <(find bin/$host$API -name 'roc-test-*' -print0)

for t in "${tests[@]}"; do
    filename=$(basename "${t}")
    [ "$filename" = "roc-test-netio" ] && args="-v" || args=""

    adb push $t $target_prefix/$filename 1> /dev/null
    python2 scripts/wrappers/timeout.py 300 \
        adb shell "LD_LIBRARY_PATH=${target_prefix}/lib" \
            "${target_prefix}/${filename} ${args}"
done