#!/usr/bin/env bash
set -euo pipefail

exe_path="$1"
source_dir="$2"

tmpdir="$(mktemp -d)"
cleanup() {
    rm -rf "$tmpdir"
}
trap cleanup EXIT

expect_success() {
    local name="$1"
    local source_path="$2"
    local output

    if ! output=$("$exe_path" "$source_path" 2>&1); then
        echo "Expected success for: $name" >&2
        echo "--- Compiler output ---" >&2
        echo "$output" >&2
        exit 1
    fi
}

expect_error() {
    local name="$1"
    local source_path="$2"
    local pattern="$3"
    local output

    if output=$("$exe_path" "$source_path" 2>&1); then
        echo "Expected semantic error for: $name" >&2
        echo "--- Compiler output ---" >&2
        echo "$output" >&2
        exit 1
    fi

    if [[ "$output" != *"$pattern"* ]]; then
        echo "Unexpected error output for: $name" >&2
        echo "Expected pattern: $pattern" >&2
        echo "--- Compiler output ---" >&2
        echo "$output" >&2
        exit 1
    fi
}

cat >"$tmpdir/exact_types_ok.mc" <<'EOF'
char echo_char(char c) {
    return c;
}

float echo_float(float f) {
    return f;
}

int main() {
    char c = 'a';
    float f = 1.0;
    int flag = !1;
    c = echo_char(c);
    f = echo_float(f);
    if (flag) {
        flag = 0;
    }
    while (flag) {
        flag = 0;
    }
    for (; flag; flag = 0) {
        flag = 0;
    }
    return flag;
}
EOF

cat >"$tmpdir/assignment_type_mismatch.mc" <<'EOF'
int main() {
    int a = 0;
    a = 'c';
    return 0;
}
EOF

cat >"$tmpdir/function_arg_count.mc" <<'EOF'
int add(int a, int b) {
    return a + b;
}

int main() {
    return add(1);
}
EOF

cat >"$tmpdir/function_arg_type.mc" <<'EOF'
int takes_char(char c) {
    return 0;
}

int main() {
    return takes_char(1);
}
EOF

cat >"$tmpdir/if_condition_float.mc" <<'EOF'
int main() {
    if (1.0) {
        return 1;
    }
    return 0;
}
EOF

cat >"$tmpdir/while_condition_char.mc" <<'EOF'
int main() {
    while ('a') {
        return 1;
    }
    return 0;
}
EOF

cat >"$tmpdir/for_condition_float.mc" <<'EOF'
int main() {
    for (; 1.0; ) {
        return 1;
    }
    return 0;
}
EOF

cat >"$tmpdir/logical_not_char.mc" <<'EOF'
int main() {
    char c = 'a';
    char x = !c;
    return 0;
}
EOF

expect_success "exact int/float/char paths" "$tmpdir/exact_types_ok.mc"
expect_error "declaration initializer type mismatch" "$source_dir/tests/semantic_type_mismatch.mc" "initializer for variable 'y'"
expect_error "assignment type mismatch" "$tmpdir/assignment_type_mismatch.mc" "assignment to variable 'a'"
expect_error "return type mismatch" "$source_dir/tests/type_error.mc" "return statement"
expect_error "function argument count mismatch" "$tmpdir/function_arg_count.mc" "expects 2 argument(s), got 1"
expect_error "function argument type mismatch" "$tmpdir/function_arg_type.mc" "argument 1 of function 'takes_char'"
expect_error "if condition type mismatch" "$tmpdir/if_condition_float.mc" "if condition"
expect_error "while condition type mismatch" "$tmpdir/while_condition_char.mc" "while condition"
expect_error "for condition type mismatch" "$tmpdir/for_condition_float.mc" "for condition"
expect_error "duplicate variable definition" "$source_dir/tests/redeclare.mc" "already declared in this scope"
expect_error "duplicate parameter definition" "$source_dir/tests/semantic_duplicate_param.mc" "parameter 'a' is already declared in this scope"
expect_error "undeclared variable use" "$source_dir/tests/semantic_undeclared.mc" "undeclared variable 'missing'"
expect_error "logical not requires int" "$tmpdir/logical_not_char.mc" "logical operator '!'"
