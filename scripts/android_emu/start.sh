#!/usr/bin/env bash

set -euo pipefail

function color_msg() {
    printf '%s \033[1;35m%s\033[0m\n' "---" "$1"
}

function run_cmd() {
    echo "+++ $*"
    "$@" || exit 1
}

export PATH="$ANDROID_SDK_ROOT/tools/bin:${PATH}"
export PATH="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:${PATH}"

# create avd if it doesn't exist
if ! avdmanager list avd -c | grep -qF roc_device
then
    color_msg "creating avd"
    run_cmd device \
        --name roc_device --image "default" --arch "${ABI}" --api "${API}" \
        create
fi

# show avd list
color_msg "checking avd"
run_cmd avdmanager list avd

# start emulator if it's not started
if ! adb devices | grep -qF emulator
then
    color_msg "starting device"
    run_cmd device --name roc_device start
fi

# show device list
color_msg "checking device"
run_cmd adb devices
