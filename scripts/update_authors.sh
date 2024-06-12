#!/usr/bin/env bash

set -euo pipefail

function find_login() {
    local github_login=""

    if echo "$1" | grep -q users.noreply.github.com
    then
        github_login="$(echo "$1" | sed -re 's,^([0-9]+\+)?(.*)@users.noreply.github.com,\2,')"
    else
        github_login="$(gh api "/search/users?q=$(echo -n "$1" | jq -sRr @uri)" \
            --jq '.items[0].login')"
    fi

    if [[ "${github_login}" != "" ]] && [[ "${github_login}" != "null" ]]
    then
        echo "${github_login}"
    fi
}

function find_name() {
    local github_name="$(gh api "/users/$1" --jq .name \
        | sed -re 's,^\s*,,' -e 's,\s*$,,')"

    if [[ "${github_name}" != "" ]] && [[ "${github_name}" != "null" ]]
    then
        echo "${github_name}"
    fi
}

function find_email() {
    if [[ ! -z "${1:-}" ]]
    then
        local github_email="$(gh api "/users/$1/events/public" \
            --jq '(.[].payload.commits | select(. != null))[].author.email' \
            | grep -v noreply \
            | sort -u \
            | head -1)"

        if [[ "${github_email}" != "" ]] && [[ "${github_email}" != "null" ]]
        then
            echo "${github_email}"
        fi
    fi

    if [[ ! -z "${2:-}" ]]
    then
        local gitlog_email="$(git log --all --pretty=format:"%an <%ae>" | sort -u | \
            grep -vF noreply | grep -F "$2" | sed -re 's,.*<(.*)>,\1,')"

        if [[ "${gitlog_email}" != "" ]]
        then
            echo "${gitlog_email}"
        fi
    fi
}

function update_author() {
    local authors_file="$1"
    local commit_name="$2"
    local commit_email="$3"
    local repo_name="$4"

    if grep -qiF "${commit_name}" "${authors_file}" || grep -qiF "${commit_email}" "${authors_file}"
    then
        return
    fi

    local github_login="$(find_login "${commit_email}")"
    if [[ -z "${github_login}" ]]
    then
        github_login="$(find_login "${commit_name}")"
    fi

    local contact_name="$(find_name "${github_login}")"
    if [ -z "${contact_name}" ] || [[ "${contact_name}" != *" "* && "${commit_name}" = *" "* ]]
    then
        contact_name="${commit_name}"
    fi
    contact_name="$(echo "${contact_name}" | sed -re 's,(\S+)\s+(\S+),\u\1 \u\2,g')"

    local contact_addr=""
    if [[ -z "${commit_email}" ]] || echo "${commit_email}" | grep -q noreply
    then
        contact_addr="$(find_email "${github_login}" "${commit_name}")"
    else
        contact_addr="${commit_email}"
    fi
    if [[ -z "${contact_addr}" ]] && [[ ! -z "${github_login}" ]]
    then
        contact_addr="https://github.com/${github_login}"
    fi

    if grep -qiF "${contact_name}" "${authors_file}"
    then
        return
    fi
    if [ ! -z "${contact_addr}" ] && grep -qiF "${contact_addr}" "${authors_file}"
    then
        return
    fi

    local result="${contact_name}"
    if [ ! -z "${contact_addr}" ]
    then
        result="${result} <${contact_addr}>"
    fi

    echo "${repo_name}: adding ${result}" 1>&2
    echo "* ${result}"
}

function update_repo() {
    local authors_file="$1"
    local org_name="$2"
    local repo_name="$3"
    local repo_dir="${HOME}/.cache/authors/${org_name}/${repo_name}"

    if [ -d "${repo_dir}" ]
    then
        echo "${repo_name}: updating repo" 1>&2
        pushd "${repo_dir}" >/dev/null
        git fetch -q
    else
        echo "${repo_name}: cloning repo" 1>&2
        mkdir -p "$(dirname "${repo_dir}")"
        git clone -q --bare "git@github.com:${org_name}/${repo_name}.git" "${repo_dir}"
        pushd "${repo_dir}" >/dev/null
    fi

    gh pr list -s all --json number --jq '.[].number' | while read pr_num
    do
        local branch="pr${pr_num}"
        if git rev-parse --verify "${branch}" >/dev/null 2>&1
        then
            continue
        fi

        local json="$(gh api "/repos/${org_name}/${repo_name}/pulls/${pr_num}")"

        local state="$(echo "${json}" | jq -r .state)"
        local merged="$(echo "${json}" | jq -r .merged)"
        local remote="$(echo "${json}" | jq -r .head.repo.ssh_url)"
        local commit="$(echo "${json}" | jq -r .head.sha)"

        if [[ "${state}" != "closed" ]]
        then
            echo "${repo_name}: skipping pr ${pr_num}" 1>&2
            continue
        fi

        if [[ "${merged}" == "false" ]]
        then
            echo "${repo_name}: dismissing pr ${pr_num} (not merged)" 1>&2
            git branch "${branch}" HEAD
            continue
        fi

        echo "${repo_name}: fetching pr ${pr_num}" 1>&2

        if [[ "${remote}" == "null" ]] || \
               ! git fetch -q "${remote}" "${commit}" >/dev/null 2>&1
        then
            echo "${repo_name}: dismissing pr ${pr_num} (no remote)" 1>&2
            git branch "${branch}" HEAD
            continue
        fi

        git branch "${branch}" "${commit}"
    done

    git log --all --full-history --reverse \
        --format="format:%at,%an,%ae" \
        --encoding=utf-8 \
        | sort -u -t, -k3,3 \
        | sort -t, -k1n \
        | while read line
    do
        local name="$(echo "${line}" | cut -d, -f2)"
        local email="$(echo "${line}" | cut -d, -f3)"
        update_author "${authors_file}" "${name}" "${email}" "${repo_name}" >> "${authors_file}"
    done

    popd >/dev/null
}

cd "$(dirname "$0")/.."

authors_file="docs/sphinx/about_project/authors.rst"
temp_file="$(mktemp)"
org_name="roc-streaming"

echo "updating ${authors_file}" 1>&2

cat "${authors_file}" > "${temp_file}"

gh repo list "${org_name}" --json name --jq '.[].name' \
    | grep -E "${1:-.*}" | while read repo_name
do
    update_repo "${temp_file}" "${org_name}" "${repo_name}"
done

cat "${temp_file}" > "${authors_file}"
rm "${temp_file}"

echo "all done" 1>&2
