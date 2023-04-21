#!/usr/bin/env bash

set -euo pipefail

function run_cmd() {
    echo "+++ $*"
    "$@" || exit 1
}

export PATH="$ANDROID_SDK_ROOT/tools/bin:${PATH}"
export PATH="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:${PATH}"

# create avd if it doesn't exist
if ! avdmanager list avd -c | grep -qF roc_device
then
    run_cmd device \
        --name roc_device --image "default" --arch "${ABI}" --api "${API}" \
        create
fi

# show avd list
run_cmd avdmanager list avd

# start emulator if it's not started
if ! adb devices | grep -qF emulator
then
    run_cmd device --name roc_device start
fi

# show device list
run_cmd adb devices
