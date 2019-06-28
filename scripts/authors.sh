#! /bin/sh
temp="$(mktemp)"

(
    cat AUTHORS
    git shortlog --summary --numbered --email \
        | sed -r -e 's,^\s*,,' -e 's,\s+, ,g' \
        | cut -d' ' -f2- \
        | while read line
        do
            if ! grep -qiF "$(echo "$line" | sed -re 's,^(.+) <.+,\1,')" AUTHORS
            then
               echo "$line"
            fi
        done
) > "$temp"

cat "$temp" > AUTHORS
rm "$temp"
