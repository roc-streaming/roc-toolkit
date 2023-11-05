#! /usr/bin/env bash

set -euo pipefail

function print_msg() {
    printf '\033[1;34m%s\033[0m\n' "$*" 1>&2
}

function run_cmd() {
    print_msg "-- $*"
    "$@" || exit 1
}

function docker_cmd() {
    run_cmd docker run --rm -t --pull always \
           -w "$(pwd)" \
           -v "$(pwd):$(pwd)" \
           "$@"
}

build_image="debian:bullseye"
check_images=(
    debian:oldstable
    debian:stable
    ubuntu:20.04
    ubuntu:22.04
    ubuntu:latest
)

cd "$(dirname "$0")/.."

action="${1:-}"

case "$action" in
    ""|build|check|build_in_docker|check_in_docker)
        ;;
    *)
        echo "usage: $0 [build|check]" 1>&2
        exit 1
        ;;
esac

if [ "$action" = "build" ] || [ "$action" = "" ] ; then
    run_cmd ./scripts/update_packages.py
    docker_cmd "$build_image" \
        scripts/make_deb.sh build_in_docker
    print_msg "Packages built."
fi

if [ "$action" = "check" ] || [ "$action" = "" ] ; then
    for image in "${check_images[@]}"; do
        docker_cmd "$image" \
            scripts/make_deb.sh check_in_docker
    done
    print_msg "All checks passed."
fi

if [ "$action" = "build_in_docker" ]; then
    export DEBIAN_FRONTEND=noninteractive

    run_cmd apt-get update
    run_cmd apt-get install -y --no-install-suggests \
            build-essential debhelper devscripts dh-exec \
            $(grep -Po '(?<=Build-Depends:).*' debian/control \
                | egrep -o '[a-zA-Z][a-zA-Z0-9+-]+' | tr '\n' ' ')

    run_cmd apt-get clean

    run_cmd dpkg-buildpackage -b -rfakeroot -us -uc

    run_cmd mkdir -p dist
    run_cmd rm -f dist/*.deb

    run_cmd mv ../*.deb dist
    run_cmd rm -f dist/*-dbgsym_*.deb

    run_cmd chown -R 1000:1000 dist
fi

if [ "$action" = "check_in_docker" ]; then
    export DEBIAN_FRONTEND=noninteractive

    run_cmd apt-get update

    run_cmd apt install -y --no-install-suggests \
        ./dist/libroc-dev_*_amd64.deb \
        ./dist/libroc_*_amd64.deb \
        ./dist/roc_*_amd64.deb

    run_cmd dpkg -L libroc-dev libroc roc

    run_cmd roc-send --version
    run_cmd roc-recv --version
fi
