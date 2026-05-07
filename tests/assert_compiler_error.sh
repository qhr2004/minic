#!/usr/bin/env bash
set -euo pipefail

exe_path="$1"
source_path="$2"
shift 2

if output=$("$exe_path" "$source_path" 2>&1); then
    echo "Expected compiler failure for: $source_path" >&2
    echo "--- Compiler output ---" >&2
    echo "$output" >&2
    exit 1
fi

for pattern in "$@"; do
    if [[ "$output" != *"$pattern"* ]]; then
        echo "Missing expected error pattern for: $source_path" >&2
        echo "Pattern: $pattern" >&2
        echo "--- Compiler output ---" >&2
        echo "$output" >&2
        exit 1
    fi
done
