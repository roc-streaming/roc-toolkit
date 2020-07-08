#! /bin/bash

function find_login() {
    curl -s "https://api.github.com/search/users?q=$1" \
        | jq -r '.items[0].login' 2>/dev/null
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
        | sort -u \
        | grep -v users.noreply.github.com \
        | head -1)"

    if [[ "${github_email}" != "" ]] && [[ "${github_email}" != "null" ]]
    then
        echo "${github_email}"
    fi
}

function add_if_new() {
    local file="$1"

    local commit_name="$2"
    local commit_email="$3"

    if grep -qiF "${commit_name}" "${file}" || grep -qiF "${commit_email}" "${file}"
    then
        return
    fi

    local github_login="$(find_login "${commit_email}")"
    if [ -z "${login}" ]
    then
        github_login="$(find_login "${commit_name}")"
    fi

    local print_name="$(find_name "${github_login}")"
    if [ -z "${print_name}" ]
    then
        print_name="${commit_name}"
    fi

    local print_email=""
    if echo "${commit_email}" | grep -q users.noreply.github.com
    then
        print_email="$(find_email "${github_login}")"
    fi
    if [ -z "${print_email}" ]
    then
        print_email="${commit_email}"
    fi

    echo "adding ${print_name} <${print_email}>" 1>&2
    echo "* ${print_name} <${print_email}>"
}

function add_contributors() {
    out_file="$1"
    repo_dir="../$2"

    if [ ! -d "${repo_dir}" ]
    then
        return
    fi

    GIT_DIR="${repo_dir}"/.git \
           git log --encoding=utf-8 --full-history --reverse "--format=format:%at,%an,%ae" \
        | sort -u -t, -k3,3 \
        | sort -t, -k1n \
        | while read line
    do
        name="$(echo "${line}" | cut -d, -f2)"
        email="$(echo "${line}" | cut -d, -f3)"

        add_if_new "${out_file}" "${name}" "${email}" >> "${out_file}"
    done
}

cd "$(dirname "$0")/.."

file="docs/sphinx/about_project/authors.rst"
temp="$(mktemp)"

cat "$file" > "$temp"

add_contributors "${temp}" "roc-toolkit"
add_contributors "${temp}" "roc-go"
add_contributors "${temp}" "roc-java"
add_contributors "${temp}" "rt-tests"
add_contributors "${temp}" "openfec"
add_contributors "${temp}" "dockerfiles"
add_contributors "${temp}" "roc-toolkit/vendor"

cat "$temp" > "$file"
rm "$temp"
