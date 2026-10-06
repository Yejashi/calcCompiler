#!/usr/bin/env bash
# End-to-end tests for compiler_driver (quiet mode).
# Usage: e2e.sh /path/to/compiler_driver
set -u

DRIVER="${1:?usage: e2e.sh /path/to/compiler_driver}"

pass=0
fail=0

# check_ok NAME INPUT EXPECTED_STDOUT
check_ok() {
  local name="$1" input="$2" expected="$3"
  local actual
  actual="$(printf '%b' "$input" | "$DRIVER" -q 2>/dev/null)"
  if [ "$actual" = "$expected" ]; then
    echo "PASS: $name"
    pass=$((pass + 1))
  else
    echo "FAIL: $name"
    echo "  input:    $input"
    echo "  expected: $expected"
    echo "  actual:   $actual"
    fail=$((fail + 1))
  fi
}

# check_err NAME INPUT PATTERN
# stderr must contain PATTERN and stdout must be empty.
check_err() {
  local name="$1" input="$2" pattern="$3"
  local out err
  out="$(printf '%b' "$input" | "$DRIVER" -q 2>/dev/null)"
  err="$(printf '%b' "$input" | "$DRIVER" -q 2>&1 >/dev/null)"
  if [ -z "$out" ] && [[ "$err" == *"$pattern"* ]]; then
    echo "PASS: $name"
    pass=$((pass + 1))
  else
    echo "FAIL: $name"
    echo "  input:    $input"
    echo "  pattern:  $pattern"
    echo "  stdout:   $out"
    echo "  stderr:   $err"
    fail=$((fail + 1))
  fi
}

# check_warn NAME INPUT EXPECTED_STDOUT STDERR_PATTERN
# (driver warns on stderr but still prints a result)
check_warn() {
  local name="$1" input="$2" expected="$3" pattern="$4"
  local out err
  out="$(printf '%b' "$input" | "$DRIVER" -q 2>/dev/null)"
  err="$(printf '%b' "$input" | "$DRIVER" -q 2>&1 >/dev/null)"
  if [ "$out" = "$expected" ] && [[ "$err" == *"$pattern"* ]]; then
    echo "PASS: $name"
    pass=$((pass + 1))
  else
    echo "FAIL: $name"
    echo "  input:    $input"
    echo "  expected stdout: $expected"
    echo "  actual stdout:   $out"
    echo "  stderr:   $err"
    fail=$((fail + 1))
  fi
}

# --- Correct evaluation -----------------------------------------------------
check_ok "README example 1"         'with a, b: a * (4 + b)\n3\n2\n'  'Result = 18'
check_ok "README example 2 (KE)"    'with m, v: 0.5 * m * v * v\n80\n5\n' 'Result = 1000'
check_ok "precedence"               '1 + 2 * 3\n'                     'Result = 7'
check_ok "unary minus"              '-5 + 3\n'                        'Result = -2'
check_ok "parenthesized unary"      '-(2 * 3)\n'                      'Result = -6'
check_ok "left assoc minus"         '10 - 3 - 2\n'                    'Result = 5'
check_ok "left assoc divide"        '100 / 10 / 5\n'                  'Result = 2'
check_ok "float literal"            '3.14 * 2\n'                      'Result = 6.28'
check_ok "negative variable"        'with a: a\n-2.5\n'               'Result = -2.5'
check_ok "div by zero"              'with a: 1 / a\n0\n'              'Result = Infinity'
check_ok "0/0"                      '0 / 0\n'                         'Result = NaN'
check_ok "multi-expression session" '1 + 1\n2 + 2\n'                  $'Result = 2\nResult = 4'
check_ok "blank lines skipped"      '\n   \n1 + 1\n'                  'Result = 2'

# --- Error handling ----------------------------------------------------------
# Note: error inputs carry no trailing value lines — the error fires before
# any prompt, so leftover lines would be evaluated as new expressions.
check_err "undeclared variable"     'with a: b + 1\n'                 "undeclared variable 'b'"
check_err "incomplete expression"   '1 +\n'                           'expected a number, identifier'
check_err "trailing tokens"         'with a: 1 2\n'                   'expected end-of-input'
check_err "duplicate declaration"   'with a, a: a * a\n'              "duplicate variable declaration 'a'"
check_err "empty with list"         'with : 1\n'                      'expected identifier'
check_err "trailing comma"          'with a, b, : a\n'                'expected identifier'
check_err "missing comma"           'with a b: a\n'                   "expected ':'"
check_err "malformed number"        '1.2.3\n'                         "unexpected character '.'"
check_err "leading dot number"      '.5\n'                            "unexpected character '.'"
check_err "dangling dot number"     '3.\n'                            'malformed number literal'
# 320 nines (~1e319) exceeds DBL_MAX (~1.8e308) -> std::stod throws
# out_of_range, which the parser reports as a CalcError.
overflow_input=$(printf '9%.0s' {1..320})
check_err "number overflow" "${overflow_input}\n" 'number literal out of range'
check_warn "invalid variable input" 'with a: a + 1\nabc\n'            'Result = 1' 'Invalid input for a'

echo
echo "e2e: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
