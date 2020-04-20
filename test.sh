#! /bin/bash

set -e -o pipefail
OK="0"
FAILURE="1"

cmd_lookup() {
  type "$@" 2>/dev/null \
    | awk '/^\w/ {
        if ($0 ~ /is a (function|shell builtin)/) {
          print($1);
        } else if ($0 ~ /is \//) {
          print($3);
        }
      }' || true
}

cmd_select() {
  local cmd="$(cmd_lookup "$@" | head -n 1)"
  if test -n "$cmd"; then
    echo "$cmd"
    return "$OK"
  else
    echo >&2 "error: No command found for $@"
    return "$FAILURE"
  fi
}

my_uuidgen() {
  "$cmd_uuidgen"
}

hashfile() {
  if [[ "$cmd_hash" = rhash ]]; then
    rhash --sha512 "$@" | awk '{ print($1) }'
  elif [[ "$cmd_hash" = openssl ]]; then
    openssl sha512 "$@" | awk '{ print($2) }'
  else
    "$cmd_hash" "$@" | awk '{ print($1) }'
  fi
}

test_cleanup() {
  if [[ "$test_ok" = "$OK" ]]; then
    rm -rf "${tmpdir}"
  fi
}

test_fail() {
  test_ok="$FAILURE"
}

test_forbidden_characters() {
  # Grep excluding C89 allowed characters. `]` for some reason won't work in the regex
  if tr ']' ' ' | grep '[^a-zA-Z0-9!"#%&'\''()*+,./:;<=>?^_{|}[\\~` -]' 2>/dev/null >/dev/null; then
    return "$FAILURE"
  else
    return "$OK"
  fi
}

test_compare_files() {
  local h1="$(hashfile "$1")"
  local h2="$(hashfile "$2")"
  if [[ "$h1" != "$h2" ]]; then
    echo >&2 -e "fail\n  $1 and $2 differ"
    test_fail
  else
    echo >&2 "ok"
  fi
}

test_bin2c() {
  local name="$1"
  local inf="$2";

  local basename="${tmpdir}/${name}"
  local cfile="${basename}.c"
  local warnings="${basename}.warnings"
  local outp="${basename}.outp"
  local exec="${basename}.elf"
  test -n "$inf" || inf="${basename}"

  echo -n >&2 "Testing bin2c for ${name}..."

  build/bin2c myfile < "$inf" > "$cfile" || true
  cc -Wall -Wextra -Wpedantic -std=c89 \
    -Wno-overlength-strings \
    build/print_myfile.o "$cfile" \
    -o "$exec" 2>&1 | cat > "$warnings" || true
  "$exec" 2>&1 | cat > "$outp" || true

  test_compare_files "$inf" "$outp"

  test_forbidden_characters < "$cfile" || {
    echo >&2 "  fail: $cfile contains forbidden characters!"
    test_fail
  }

  if test -n "$(cat "$warnings")"; then
    echo >&2 "  fail: compiler warnings"
    cat "$warnings" | sed 's@^@    @' >&2
    test_fail
  fi
}

test_main() {
  test_ok="$OK"

  test_bin2c help src/help.txt
  
  echo -n >&2 "Checking help.c formatting..."
  test_compare_files "${tmpdir}/help.c" "test/help.txt.c"

  build/genbytes > "${tmpdir}/all_bytes"
  test_bin2c all_bytes

  dd if=/dev/urandom of="${tmpdir}/random" bs=10000000 count=1 2>/dev/null
  test_bin2c random

  test "$test_ok" = "$OK"
}

init() {
  export LC_ALL="C"

  cd "$(dirname "$0")"

  # Select uuidgen implementation
  cmd_uuidgen="$(cmd_select uuidgen genrandom)"
  cmd_hash="$(cmd_select sha512sum sha384sum sha224sum sha1sum xxh128sum xxh64sum xxhsum openssl rhash)"
  cc="$(cmd_select cc clang gcc)"

  # Dealing with temp files
  tmpdir="/tmp/bin2c-test-"$(date +"%Y-%m-%d-%H:%M:%S")"-$(my_uuidgen)"
  mkdir -p "$tmpdir"
  trap test_cleanup EXIT

  test_main
}

init "$@"
