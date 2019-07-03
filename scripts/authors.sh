#! /bin/sh

file="docs/sphinx/about_project/authors.rst"
temp="$(mktemp)"

(
    cat "$file"
    git shortlog --summary --numbered --email \
        | sed -r -e 's,^\s*,,' -e 's,\s+, ,g' \
        | cut -d' ' -f2- \
        | while read line
        do
            if ! grep -qiF "$(echo "$line" | sed -re 's,^(.+) <.+,\1,')" "$file"
            then
               echo "* $line"
            fi
        done
) > "$temp"

cat "$temp" > "$file"
rm "$temp"
