#!/usr/bin/env bash
set -euo pipefail

mode="maple"
if [[ "${1:-}" == "--abc" ]]; then
    mode="abc"
    shift
fi

exe_path="$1"
source_path="$2"
output_path="$3"
shift 3

search_path="$output_path"

if [[ "$mode" == "abc" ]]; then
    backend_cmd="${MINIC_ARK_BACKEND_CMD:-}"
    if [[ "$backend_cmd" != *"{input}"* || "$backend_cmd" != *"{output}"* ]]; then
        backend_cmd='cat {input} > {output}'
    fi

    MINIC_ARK_BACKEND_CMD="$backend_cmd" "$exe_path" --emit-abc "$source_path" "$output_path" >/dev/null

    if [[ ! -s "$output_path" ]]; then
        echo "ABC output was not generated: $output_path" >&2
        exit 1
    fi

    search_path="${output_path%.*}.mpl"
    if [[ ! -s "$search_path" ]]; then
        echo "Expected Maple IR companion file was not generated: $search_path" >&2
        exit 1
    fi
else
    "$exe_path" --emit-maple "$source_path" "$output_path" >/dev/null
fi

for pattern in "$@"; do
    if ! grep -Fq "$pattern" "$search_path"; then
        echo "Missing Maple IR pattern: $pattern" >&2
        echo "--- Maple IR ---" >&2
        sed -n '1,320p' "$search_path" >&2
        exit 1
    fi
done
