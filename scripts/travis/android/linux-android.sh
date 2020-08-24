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

device create --name roc-device-$host$API --image default --api $API --arch $arch
device start --name roc-device-$host$API

# add route for multicast traffic
nifaces=( $(adb shell "ip a" | grep -Po "[0-9]+: \K([0-9a-z]+)(?=.*state UP)") )
for niface in "${nifaces[@]}"; do
    adb shell "su 0 ip route add 224.0.0.0/4 dev ${niface} table local" || continue
done

adb shell "su 0 mkdir -p ${target_prefix}/lib"
adb push "$sysroot/libc++_shared.so" $target_prefix/lib/libc++_shared.so 1> /dev/null
adb push "bin/$host$API/libroc.so" $target_prefix/lib/libroc.so 1> /dev/null

readarray -d '' tests < <(find bin/$host$API -name 'roc-test-*' -print0)

mkdir -p tests/$host$API
log_lines=800

for t in "${tests[@]}"; do
    filename=$(basename "${t}")
    [ "${filename}" = "roc-test-library" ] && [ "${ABI}" = "i686" ] && continue

    logfile="tests/$host$API/$filename.log"
    adb push $t $target_prefix/$filename 1> /dev/null

    echo -e "     \033[0;32mTEST\033[0m   ${filename//-/_}"
    python2 scripts/build/timeout.py 300 \
        adb shell "LD_LIBRARY_PATH=${target_prefix}/lib" \
            "${target_prefix}/${filename} -v" > >(tee -ia $logfile | tail -n 2) 2>> $logfile || \
                (error=$?; tail -n $log_lines $logfile; exit $error)
done
