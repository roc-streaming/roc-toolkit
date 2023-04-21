#!/usr/bin/env bash

set -euo pipefail

function run_cmd() {
    echo "+++ $*"
    "$@" || exit 1
}

action="${1:-}"

if [ -z "${ANDROID_NDK_ROOT:-}" ]
then
    export ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/${NDK_VERSION}"
fi

case "${OSTYPE}" in
    darwin*)
        toolchain_root="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/darwin-$(uname -m)"
        ;;
    linux*)
        toolchain_root="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-$(uname -m)"
        ;;
esac

case "${ABI}" in
    "x86_64")
        target_arch="x86_64"
        target_host="${ABI}-linux-android"
        target_toolchain="${target_host}${API}"
        sysroot="${toolchain_root}/sysroot/usr/lib/${target_host}"
        ;;
    "i686")
        target_arch="x86"
        target_host="${ABI}-linux-android"
        target_toolchain="${target_host}${API}"
        sysroot="${toolchain_root}/sysroot/usr/lib/${target_host}"
        ;;
    "aarch64")
        target_arch="arm64-v8a"
        target_host="${ABI}-linux-android"
        target_toolchain="${target_host}${API}"
        sysroot="${toolchain_root}/sysroot/usr/lib/${target_host}"
        ;;
    "armv7a")
        target_arch="armeabi-v7a"
        target_host="${ABI}-linux-androideabi"
        target_toolchain="${target_host}${API}"
        sysroot="${toolchain_root}/sysroot/usr/lib/arm-linux-androideabi"
        ;;
    *)
        echo "unrecognized android ABI: '${ABI}'"
        exit 1
        ;;
esac

export PATH="${ANDROID_SDK_ROOT}/tools/bin:${PATH}"
export PATH="${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin:${PATH}"
export PATH="${toolchain_root}/bin:${PATH}"

if [[ "${action}" == build ]]; then
    run_cmd scons -Q \
          --compiler=clang \
          --host="${target_toolchain}" \
          --enable-werror \
          --enable-tests \
          --disable-soversion \
          --disable-tools \
          --build-3rdparty=libuv,openfec,openssl,speexdsp,cpputest
fi

if [[ "${action}" == test ]]; then
    target_prefix="/data/local/tmp"

    run_cmd adb shell "su 0 mkdir -p ${target_prefix}/lib"
    run_cmd adb push "${sysroot}/libc++_shared.so" "${target_prefix}/lib/libc++_shared.so"

    # FIXME:
    #  https://github.com/roc-streaming/roc-toolkit/issues/435
    #  https://github.com/roc-streaming/roc-toolkit/issues/516
    #  https://github.com/roc-streaming/roc-toolkit/issues/518
    tests=( $(find "bin/${target_toolchain}" -name 'roc-test-*' \
                   -not -name 'roc-test-address' \
                   -not -name 'roc-test-ctl' \
                   -not -name 'roc-test-netio' \
                   -not -name 'roc-test-public-api') )

    for test_path in "${tests[@]}"; do
        test_name="$(basename ${test_path})"

        run_cmd adb push "$test_path" "${target_prefix}/${test_name}"

        run_cmd python scripts/scons_helpers/timeout-run.py 300 \
                adb shell "LD_LIBRARY_PATH=${target_prefix}/lib" \
                "${target_prefix}/${test_name}" </dev/null
    done
fi
