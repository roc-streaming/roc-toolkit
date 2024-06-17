#!/usr/bin/env bash

set -euo pipefail

function run_cmd() {
    echo "+++ $*"
    "$@" || exit 1
}

# setup default values
: "${JAVA_VERSION:=8}"
: "${API:=28}"
: "${ABI:=x86_64}"
: "${NDK_VERSION:=21.1.6352462}"
: "${BUILD_TOOLS_VERSION:=28.0.3}"
: "${CMAKE_VERSION:=3.10.2.4988404}"

# go to project root
cd "$(dirname "$0")"/..

# parse arguments
action="${1:-}"
case "${action}" in
    build|test|clean|purge)
        ;;
    *)
        echo "usage: $(basename $0) build|test|clean|purge" >&2
        exit 1
        ;;
esac

# remove docker stuff we've created
if [[ "${action}" = purge ]]
then
    if docker ps -a --format '{{.Names}}' | grep -qF roc_toolkit_android
    then
        run_cmd docker rm -f roc_toolkit_android
    fi

    if docker volume ls --format '{{.Name}}' | grep -qF roc_toolkit_android_sdk
    then
        run_cmd docker volume rm roc_toolkit_android_sdk
    fi
fi

# remove build artifcats
if [[ "${action}" = purge || "${action}" = clean ]]
then
    run_cmd rm -rf bin
    run_cmd rm -rf build
    run_cmd rm -rf .scon*

    exit
fi

# if container exists, but was mounted to different location, remove it
if docker ps -a --format '{{.Names}}' | grep -q roc_toolkit_android
then
    mount_point="$(docker container inspect \
        --format '{{ range .Mounts }}{{ .Destination }}:{{ end }}' roc_toolkit_android \
            | tr ':' '\n' | grep -E . | grep -vE '^/sdk|\.gradle$' | head -1)"

    if [ "$mount_point" != "${PWD}" ]
    then
        run_cmd docker rm -f roc_toolkit_android
    fi
fi

# if container exists, but was based on different image, remove it
if docker ps -a --format '{{.Names}}' | grep -q roc_toolkit_android
then
    image="$(docker container inspect --format '{{.Config.Image}}' roc_toolkit_android)"

    if [ "$image" != "rocstreaming/env-android:jdk${JAVA_VERSION}" ]
    then
        run_cmd docker rm -f roc_toolkit_android
    fi
fi

# if container exists, but was started with different parameters, remove it
if docker ps -a --format '{{.Names}}' | grep -q roc_toolkit_android
then
    for var in API ABI NDK_VERSION BUILD_TOOLS_VERSION CMAKE_VERSION
    do
        expected="${!var}"
        actual="$(docker inspect \
                --format '{{range .Config.Env}}{{println .}}{{end}}' roc_toolkit_android \
             | grep "^${var}=" | cut -d= -f2 || true)"

        if [ "$expected" != "$actual" ]
        then
            run_cmd docker rm -f roc_toolkit_android
            break
        fi
    done
fi

# if container does not exist, create it
if ! docker ps -a --format '{{.Names}}' | grep -q roc_toolkit_android
then
    docker_args=(
        --env CI="${CI:-false}"
        --env API="${API}"
        --env ABI="${ABI}"
        --env NDK_VERSION="${NDK_VERSION}"
        --env BUILD_TOOLS_VERSION="${BUILD_TOOLS_VERSION}"
        --env CMAKE_VERSION="${CMAKE_VERSION}"
        -v "${PWD}:${PWD}"
        -v roc_toolkit_android_sdk:/sdk
        -w "${PWD}"
    )

    # for hardware acceleration in emulator
    if [[ -e "/dev/kvm" ]]
    then
        docker_args+=( --privileged --device /dev/kvm )
    fi

    # start container in background mode, ignore its entrypoint
    run_cmd docker run --name roc_toolkit_android \
            --net host \
            -d --entrypoint "" \
            "${docker_args[@]}" \
           rocstreaming/env-android:jdk"${JAVA_VERSION}" \
           sleep infinity

    # explicitly execute and wait entrypoint, which installs Android components
    run_cmd docker exec roc_toolkit_android \
            /opt/entrypoint.sh true

    # add user with the same uid as on host
    run_cmd docker exec roc_toolkit_android \
            useradd -ms /bin/bash -u${UID} user
fi

# if container is not running, start it
if [[ "$(docker inspect -f '{{.State.Running}}' roc_toolkit_android)" != "true" ]]
then
    run_cmd docker start roc_toolkit_android
fi

# build
run_cmd docker exec roc_toolkit_android \
        su -Ppc "scripts/android_emu/project.sh build" user

# run tests on emulator
if [[ "${action}" = test ]]
then
    run_cmd docker exec roc_toolkit_android \
            "scripts/android_emu/emulator.sh start_avd"

    run_cmd docker exec roc_toolkit_android \
            su -Ppc "scripts/android_emu/emulator.sh create_routes" user

    run_cmd docker exec roc_toolkit_android \
            su -Ppc "scripts/android_emu/project.sh run_tests" user
fi
