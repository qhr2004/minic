#!/usr/bin/env bash
set -euo pipefail

exe_path="$1"
source_path="$2"
mode="$3"

tmpdir="$(mktemp -d)"
cleanup() {
    rm -rf "$tmpdir"
}
trap cleanup EXIT

output_path="$tmpdir/output.abc"
mpl_path="$tmpdir/output.mpl"

run_expect_failure() {
    local output

    if output=$("$@" 2>&1); then
        echo "Expected --emit-abc failure for mode: $mode" >&2
        echo "--- Compiler output ---" >&2
        echo "$output" >&2
        exit 1
    fi

    if [[ ! -s "$mpl_path" ]]; then
        echo "Expected Maple IR file to be kept: $mpl_path" >&2
        exit 1
    fi

    case "$mode" in
        unset-env)
            [[ "$output" == *"MINIC_ARK_BACKEND_CMD is not set"* ]] || {
                echo "Missing unset-env message" >&2
                echo "$output" >&2
                exit 1
            }
            [[ "$output" == *"Maple IR kept at: $mpl_path"* ]] || {
                echo "Missing Maple IR path in unset-env message" >&2
                echo "$output" >&2
                exit 1
            }
            ;;
        missing-input)
            [[ "$output" == *"MINIC_ARK_BACKEND_CMD must contain {input}"* ]] || {
                echo "Missing {input} placeholder message" >&2
                echo "$output" >&2
                exit 1
            }
            [[ "$output" == *"Maple IR kept at: $mpl_path"* ]] || {
                echo "Missing Maple IR path in missing-input message" >&2
                echo "$output" >&2
                exit 1
            }
            ;;
        missing-output)
            [[ "$output" == *"MINIC_ARK_BACKEND_CMD must contain {output}"* ]] || {
                echo "Missing {output} placeholder message" >&2
                echo "$output" >&2
                exit 1
            }
            [[ "$output" == *"Maple IR kept at: $mpl_path"* ]] || {
                echo "Missing Maple IR path in missing-output message" >&2
                echo "$output" >&2
                exit 1
            }
            ;;
        command-fails)
            local expected_command="false '$mpl_path' '$output_path'"
            [[ "$output" == *"Ark backend command failed"* ]] || {
                echo "Missing backend failure header" >&2
                echo "$output" >&2
                exit 1
            }
            [[ "$output" == *"command: $expected_command"* ]] || {
                echo "Missing executed command in backend failure output" >&2
                echo "$output" >&2
                exit 1
            }
            [[ "$output" == *"exit code: 1"* ]] || {
                echo "Missing exit code in backend failure output" >&2
                echo "$output" >&2
                exit 1
            }
            [[ "$output" == *"Maple IR kept at: $mpl_path"* ]] || {
                echo "Missing Maple IR path in backend failure output" >&2
                echo "$output" >&2
                exit 1
            }
            ;;
        *)
            echo "Unknown mode: $mode" >&2
            exit 1
            ;;
    esac
}

case "$mode" in
    unset-env)
        run_expect_failure env -u MINIC_ARK_BACKEND_CMD \
            "$exe_path" --emit-abc "$source_path" "$output_path"
        ;;
    missing-input)
        run_expect_failure env MINIC_ARK_BACKEND_CMD='cat {output}' \
            "$exe_path" --emit-abc "$source_path" "$output_path"
        ;;
    missing-output)
        run_expect_failure env MINIC_ARK_BACKEND_CMD='cat {input}' \
            "$exe_path" --emit-abc "$source_path" "$output_path"
        ;;
    command-fails)
        run_expect_failure env MINIC_ARK_BACKEND_CMD='false {input} {output}' \
            "$exe_path" --emit-abc "$source_path" "$output_path"
        ;;
    *)
        echo "Unknown mode: $mode" >&2
        exit 1
        ;;
esac
