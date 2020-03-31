#! /bin/bash

function find_name() {
    github_name="$(curl -s "https://api.github.com/search/users?q=$1" \
        | jq -r .items[0].login 2>/dev/null \
        | xargs -i curl -s "https://api.github.com/users/{}" \
        | jq -r .name 2>/dev/null \
        | sed -r -e 's,^\s*,,' -e 's,\s*$,,')"

    if [[ "${github_name}" != "" ]] && [[ "${github_name}" != "null" ]]
    then
        echo "${github_name}"
    fi
}

function add_if_new() {
    line="$1"
    file="$2"

    name="$(echo "${line}" | sed -re 's,^(.*?)\s+<[^>]+>$,\1,')"
    email="$(echo "${line}" | sed -re 's,^.*<([^>]+)>$,\1,')"

    if grep -qiF "${name}" "${file}" || grep -qiF "${email}" "${file}"
    then
        return
    fi

    echo "adding ${email}" 1>&2

    print_name="$(find_name "${email}")"

    if [ -z "${print_name}" ]
    then
        print_name="$(find_name "${name}")"
    fi

    if [ -z "${print_name}" ]
    then
        print_name="${name}"
    fi

    echo "* ${print_name} <${email}>"
}

function add_contributors() {
    out_file="$1"
    repo_dir="../$2"

    if [ ! -d "${repo_dir}" ]
    then
        return
    fi

    GIT_DIR="${repo_dir}"/.git git shortlog --summary --numbered --email \
        | sed -r -e 's,^\s*,,' -e 's,\s+, ,g' \
        | cut -d' ' -f2- \
        | while read line
        do
            add_if_new "${line}" "${out_file}" >> "${out_file}"
        done
}

cd "$(dirname "$0")/.."

file="docs/sphinx/about_project/authors.rst"
temp="$(mktemp)"

cat "$file" > "$temp"

add_contributors "${temp}" "roc"
add_contributors "${temp}" "roc-tests"
add_contributors "${temp}" "roc-go"
add_contributors "${temp}" "roc-java"
add_contributors "${temp}" "roc/vendor"
add_contributors "${temp}" "openfec"
add_contributors "${temp}" "dockerfiles"

cat "$temp" > "$file"
rm "$temp"
