#! /bin/bash
set -euxo pipefail

target_prefix="/data/local/tmp"

scons -Q clean

scons -Q \
    --enable-werror \
    --disable-tools \
    --enable-tests \
    --enable-benchmarks \
    --build-3rdparty=libuv,openfec,speexdsp,cpputest,google-benchmark \
    --compiler=clang \
    --host=i686-linux-android$API

device create --name roc-device-i686-linux-android$API --image default --api $API --arch x86
device start --name roc-device-i686-linux-android$API

adb shell "su 0 mkdir -p ${target_prefix}/lib"
adb push "$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/i686-linux-android/libc++_shared.so" \
        $target_prefix/lib/libc++_shared.so 1> /dev/null

readarray -d '' tests < <(find bin/i686-linux-android$API \
        -name 'roc-test-*' -not -name 'roc-test-library' -print0)

for t in "${tests[@]}"; do
    filename=$(basename "${t}")
    adb push $t $target_prefix/$filename 1> /dev/null
    python2 scripts/wrappers/timeout.py 300 \
        adb shell "LD_LIBRARY_PATH=${target_prefix}/lib" \
            "${target_prefix}/${filename}"
done