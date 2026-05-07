#!/usr/bin/env bash
set -euo pipefail

exe_path="$1"
source_path="$2"
output_path="$3"
shift 3

"$exe_path" --emit-ast-dot "$source_path" "$output_path" >/dev/null

if [[ ! -s "$output_path" ]]; then
    echo "AST DOT output was not generated: $output_path" >&2
    exit 1
fi

for pattern in "$@"; do
    if ! grep -Fq "$pattern" "$output_path"; then
        echo "Missing AST DOT pattern: $pattern" >&2
        echo "--- AST DOT ---" >&2
        sed -n '1,320p' "$output_path" >&2
        exit 1
    fi
done
