#! /bin/bash
set -euxo pipefail

build="darwin-x86_64"
toolchain="$ANDROID_HOME/ndk/${CI_NDK}/toolchains/llvm/prebuilt/$build"

case "$CI_ABI" in
    "x86_64")
        arch=$CI_ABI
        host=$CI_ABI-linux-android
        sysroot="$toolchain/sysroot/usr/lib/$host"
        ;;
    "i686")
        arch=x86
        host=$CI_ABI-linux-android
        sysroot="$toolchain/sysroot/usr/lib/$host"
        ;;
    "aarch64")
        arch=arm64-v8a
        host=$CI_ABI-linux-android
        sysroot="$toolchain/sysroot/usr/lib/$host"
        ;;
    "armv7a")
        arch=armeabi-v7a
        host=$CI_ABI-linux-androideabi
        sysroot="$toolchain/sysroot/usr/lib/arm-linux-androideabi"
        ;;
    *) echo "unrecognized android ABI: '$CI_ABI'"; exit 1;;
esac

export PATH="$ANDROID_HOME/tools/bin:${PATH}"
export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:${PATH}"
export PATH="$toolchain/bin:${PATH}"

scons -Q clean

scons -Q \
      --enable-werror \
      --disable-soversion \
      --disable-tools \
      --enable-tests \
      --build-3rdparty=libuv,openfec,speexdsp,cpputest \
      --compiler=clang \
      --host=$host$CI_API

echo no | avdmanager create avd --name roc --package "$CI_AVD"

( cd "$ANDROID_HOME/tools" &&
      ./emulator -avd roc -no-audio -no-boot-anim -no-window -gpu off -accel on >/dev/null 2>&1 & )

boot_completed="0"
while [ "$boot_completed" != "1" ]; do
    boot_completed=$(adb wait-for-device shell getprop sys.boot_completed | tr -d '\r')
    sleep 5
done

target_prefix="/data/local/tmp"

adb shell "su 0 mkdir -p ${target_prefix}/lib"
adb push "$sysroot/libc++_shared.so" $target_prefix/lib/libc++_shared.so 1> /dev/null
adb push "bin/$host$CI_API/libroc.so" $target_prefix/lib/libroc.so 1> /dev/null

mkdir -p tests/$host$CI_API
log_lines=800

tests=( $(find bin/$host$CI_API -name 'roc-test-*') )

# https://github.com/roc-streaming/roc-toolkit/issues/424
if [ "$CI_ABI" = i686 ]; then
    tests=( $(find bin/$host$CI_API -name 'roc-test-*' -not -name 'roc-test-library') )
fi

for t in "${tests[@]}"; do
    filename=$(basename "${t}")

    logfile="tests/$host$CI_API/$filename.log"
    adb push $t $target_prefix/$filename 1> /dev/null
    adb shell echo

    python2 scripts/build/timeout.py 300 \
        adb shell "LD_LIBRARY_PATH=${target_prefix}/lib" \
        "${target_prefix}/${filename} -v" >$logfile 2>/dev/null && \
        (tail -2 $logfile) || \
            (error=$?; tail -n $log_lines $logfile; exit $error)
done
