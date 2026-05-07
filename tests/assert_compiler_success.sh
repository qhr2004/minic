#!/usr/bin/env bash
set -euo pipefail

exe_path="$1"
source_path="$2"

output="$("$exe_path" "$source_path" 2>&1)" || {
    echo "Expected compiler success for: $source_path" >&2
    echo "--- Compiler output ---" >&2
    echo "$output" >&2
    exit 1
}
