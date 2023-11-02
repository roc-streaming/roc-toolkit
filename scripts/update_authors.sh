#!/usr/bin/env bash

function find_login() {
    local github_login="$(curl -s "https://api.github.com/search/users?q=$1" \
        | jq -r '.items[0].login' 2>/dev/null)"

    if [[ "${github_login}" != "" ]] && [[ "${github_login}" != "null" ]]
    then
        echo "${github_login}"
    fi
}

function find_name() {
    local github_name="$(curl -s "https://api.github.com/users/$1" \
        | jq -r .name 2>/dev/null \
        | sed -r -e 's,^\s*,,' -e 's,\s*$,,')"

    if [[ "${github_name}" != "" ]] && [[ "${github_name}" != "null" ]]
    then
        echo "${github_name}"
    fi
}

function find_email() {
    local github_email="$(curl -s "https://api.github.com/users/$1/events/public" \
        | jq -r \
    '((.[].payload.commits | select(. != null))[].author | select(.name == "'$1'")).email' \
    2>/dev/null \
        | sort -u \
        | grep -v users.noreply.github.com \
        | head -1)"

    if [[ "${github_email}" != "" ]] && [[ "${github_email}" != "null" ]]
    then
        echo "${github_email}"
    fi

    local reflog_email="$(git reflog --pretty=format:"%an <%ae>" | sort -u | \
        grep -vF users.noreply.github.com | grep -F "$1" | sed -re 's,.*<(.*)>,\1,')"

    if [[ "${reflog_email}" != "" ]]
    then
        echo "${reflog_email}"
    fi
}

function add_if_new() {
    local file="$1"

    local commit_name="$2"
    local commit_email="$3"

    local repo_name="$4"

    if grep -qiF "${commit_name}" "${file}" || grep -qiF "${commit_email}" "${file}"
    then
        return
    fi

    local github_login="$(find_login "${commit_email}")"
    if [ -z "${github_login}" ]
    then
        github_login="$(find_login "${commit_name}")"
    fi
    if [[ -z "${github_login}" ]]
    then
        if echo "${commit_email}" | grep -q users.noreply.github.com
        then
            github_login="$(echo "${commit_email}" | sed -re 's,^([0-9]+\+)?([^@]+).*$,\2,')"
        fi
    fi

    local print_name="$(find_name "${github_login}")"
    if [ -z "${print_name}" ]
    then
        print_name="${commit_name}"
    fi
    print_name="$(echo "${print_name}" | sed -re 's/\S+/\u&/g')"

    local print_addr=""
    if echo "${commit_email}" | grep -q users.noreply.github.com
    then
        if [[ ! -z "${github_login}" ]]
        then
            print_addr="$(find_email "${github_login}")"
        fi
    else
        print_addr="${commit_email}"
    fi
    if [[ -z "${print_addr}" && ! -z "${github_login}" ]]
    then
        print_addr="https://github.com/${github_login}"
    fi

    if grep -qiF "${print_addr}" "${file}"
    then
        return
    fi

    if [ -z "${print_addr}" ]
    then
        echo "[${repo_name}] adding ${print_name}" 1>&2
        echo "* ${print_name}"
    else
        echo "[${repo_name}] adding ${print_name} <${print_addr}>" 1>&2
        echo "* ${print_name} <${print_addr}>"
    fi
}

function add_contributors() {
    out_file="$1"
    repo_dir="../$2"
    repo_name="$(basename "$2")"

    if [ ! -d "${repo_dir}" ]
    then
        return
    fi

    pushd "${repo_dir}" >/dev/null

    git log --encoding=utf-8 --full-history --reverse "--format=format:%at,%an,%ae" \
        | sort -u -t, -k3,3 \
        | sort -t, -k1n \
        | while read line
    do
        name="$(echo "${line}" | cut -d, -f2)"
        email="$(echo "${line}" | cut -d, -f3)"

        add_if_new "${out_file}" "${name}" "${email}" "${repo_name}" >> "${out_file}"
    done

    popd >/dev/null
}

function update_sphinx() {
    file="$1"
    temp="$(mktemp)"

    cat "${file}" > "${temp}"

    add_contributors "${temp}" "$(basename "$(pwd)")"
    add_contributors "${temp}" "$(basename "$(pwd)")/3rdparty/_distfiles"
    add_contributors "${temp}" "rt-tests"
    add_contributors "${temp}" "roc-pulse"
    add_contributors "${temp}" "roc-vad"
    add_contributors "${temp}" "roc-go"
    add_contributors "${temp}" "roc-java"
    add_contributors "${temp}" "roc-droid"
    add_contributors "${temp}" "openfec"
    add_contributors "${temp}" "dockerfiles"
    add_contributors "${temp}" "roc-streaming.github.io"

    cat "$temp" > "${file}"
    rm "${temp}"
}

function update_debian() {
    from="$1"
    to="$2"
    temp="$(mktemp)"

    cat "${to}" | sed '/^Copyright:/q' > "${temp}"
    cat "${from}" | grep -F '* ' | sed -re 's,\*\s*,  ,' >> "${temp}"
    cat "${to}" | sed -n '/^License:/,$p' >> "${temp}"

    cat "${temp}" > "${to}"
    rm "${temp}"
}

cd "$(dirname "$0")/.."

sphinx="docs/sphinx/about_project/authors.rst"
debian="debian/copyright"

echo "Updating ${sphinx}..."
update_sphinx "${sphinx}"

echo "Updating ${debian}..."
update_debian "${sphinx}" "${debian}"

echo "Done."
