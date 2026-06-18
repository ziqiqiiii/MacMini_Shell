#!/usr/bin/env bash
# Generates an AI prompt for unit-testing a shell builtin/command source file.
#
# Usage:
#   bash scripts/gen_unit_tests.sh <module>
#   make ai-unit-tests MODULE=<module>
#
# <module> is the source file stem, e.g. "09a_echo" for src/09a_echo.c.
#
# If includes/libs/<module>.h exists the prompt tells the AI to include it.
# Otherwise the prompt instructs the AI to use extern declarations so no new
# header is needed and readline/curses are never pulled in.
#
# Set MACMINI_AGENT_CMD to pipe the prompt to your AI tool, otherwise the
# prompt is printed to stdout for manual use:
#
#   export MACMINI_AGENT_CMD="claude --print"
#   export MACMINI_AGENT_CMD="tee /tmp/prompt.txt"

set -euo pipefail

MODULE="${1:-}"
if [[ -z "$MODULE" ]]; then
  echo "Usage: $0 <module>" >&2
  echo "  Example: $0 09a_echo" >&2
  exit 1
fi

SOURCE="$(find ./src -name "*${MODULE}.c" | head -1)"
if [[ -z "$SOURCE" || ! -f "$SOURCE" ]]; then
  echo "Error: no src file matching *${MODULE}.c found under ./src." >&2
  exit 1
fi

HEADER="./includes/libs/${MODULE}.h"
HAS_HEADER=0
[[ -f "$HEADER" ]] && HAS_HEADER=1

# ---------------------------------------------------------------------------
# Extract non-static public function signatures from the source file.
# In 42 / norminette style each public function starts at column 0 with its
# return type and name on the same line, followed by '{' on the next line.
# ---------------------------------------------------------------------------
extract_public_signatures() {
  awk '
    /^static[[:space:]]/ { next }
    /^[a-z][a-zA-Z0-9_*[:space:]	]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(/ {
      sub(/[[:space:]]*\{[[:space:]]*$/, "")
      sub(/[[:space:]]+$/, "")
      if (/\)/) print $0 ";"
    }
  ' "$1"
}

SIGNATURES="$(extract_public_signatures "$SOURCE")"

if [[ -z "$SIGNATURES" ]]; then
  echo "Warning: no public function signatures found in $SOURCE." >&2
  echo "The prompt will include the full source for the AI to analyse." >&2
fi

USED_TYPES="$(echo "$SIGNATURES" \
  | grep -oE 't_[a-z_]+|t_list|t_root|t_tree|t_history' \
  | sort -u | tr '\n' ' ' | sed 's/[[:space:]]*$//' \
  || true)"

# ---------------------------------------------------------------------------
# Identify cross-file function calls so the AI knows what to stub.
# Finds every function-call-like token in the source, removes libft calls,
# static functions, standard C functions, and functions defined in this file.
# ---------------------------------------------------------------------------
extract_cross_file_deps() {
  local src="$1"

  local all_calls
  all_calls="$(grep -oE '[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(' "$src" \
    | sed 's/[[:space:]]*($//' | sort -u || true)"

  local static_fns
  static_fns="$(awk '/^static[[:space:]]/ && /\(/ {
    match($0, /[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(/)
    s = substr($0, RSTART, RLENGTH)
    gsub(/[[:space:]]*\(/, "", s)
    print s
  }' "$src" | sort -u)"

  local public_fns
  public_fns="$(echo "$SIGNATURES" \
    | grep -oE '[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(' \
    | sed 's/[[:space:]]*($//' | sort -u || true)"

  echo "$all_calls" | while IFS= read -r fn; do
    [[ -z "$fn" ]] && continue
    # skip libft
    [[ "$fn" == ft_* ]] && continue
    # skip standard C / POSIX
    case "$fn" in
      printf|puts|perror|free|malloc|calloc|exit|write|read|open|close|dup|dup2|\
      pipe|fork|chdir|getcwd|access|unlink|execve|waitpid|wait|kill|\
      signal|sigaction|sigemptyset|sigaddset|isatty|tcgetattr|tcsetattr|\
      snprintf|getenv|setenv|fflush|readlink|dirname|strlen|strcmp|strncmp|\
      strcpy|strcat|memset|memcpy|sprintf|stat|lstat|fstat|\
      if|while|return|sizeof|NULL) continue ;;
    esac
    # skip functions defined in this file
    echo "$static_fns" | grep -qxF "$fn" && continue
    echo "$public_fns" | grep -qxF "$fn" && continue
    echo "$fn"
  done | sort -u
}

CROSS_DEPS="$(extract_cross_file_deps "$SOURCE")"

build_prompt_with_header() {
  cat <<PROMPT
You are generating a C unit test file for the MacMini Shell project.

## Goal

Write tests/unit/test_${MODULE}.c that exercises the public functions in
${SOURCE}.

## Hard constraints

1. Include the module header "${HEADER#./includes/libs/}" — it provides the
   type definitions and function declarations needed by this module.
2. libft.a IS linked into the unit test binary. Call ft_* functions directly
   (ft_strlen, ft_strncmp, ft_calloc, ft_strdup, ft_substr, ft_strchr,
   ft_lstadd_back, ft_lstnew, ft_lstclear, ft_lstdelone, ft_putstr_fd,
   ft_isalpha, ft_isalnum, ft_atoi, ft_itoa, ft_split, ft_strjoin, etc.).
   Do NOT write libft stubs.
3. Define \`int g_exit_status = 0;\` at file scope — minishell.h declares it
   as extern, so each test binary must supply the definition.
4. The Makefile only compiles one shell source file per test (the one matching
   the module name). If the module calls functions defined in OTHER shell source
   files (not libft), provide minimal stubs for those functions in the test file.
5. Only include standard C headers, "unity.h", and the module header.
6. Do NOT create any new header file.

## Project conventions

- Test framework : Unity (tests/unity/unity.h)
- Test file      : tests/unit/test_${MODULE}.c
- setUp() and tearDown() must always be defined (even if empty)
- All test functions: static void, named test_<what>_<expected>()
- main(): UNITY_BEGIN() → RUN_TEST(…) for each → return UNITY_END()
- No heap allocation in tests; use stack buffers where possible
- Cover the happy path, edge cases, and boundary conditions
- Capture stdout via pipe() + dup2() when testing functions that write to fd 1
- Capture stderr via pipe() + dup2() when testing functions that write to fd 2

## Public functions to test

\`\`\`c
${SIGNATURES:-/* (no signatures extracted — see full source below) */}
\`\`\`

${USED_TYPES:+Project types referenced: ${USED_TYPES}
These types are already defined in the module header — no forward declarations needed.
}${CROSS_DEPS:+## Cross-file dependencies to stub

The following functions are called by this module but defined in other source
files (not libft). Provide minimal stubs in the test file:

${CROSS_DEPS}
}
## Header: ${HEADER}

\`\`\`c
$(cat "$HEADER")
\`\`\`

## Full source: ${SOURCE}

\`\`\`c
$(cat "$SOURCE")
\`\`\`

## Output

Respond with the complete, compilable C source for tests/unit/test_${MODULE}.c.
Output only the raw C code — no markdown fences, no tool calls, no explanation.
PROMPT
}

build_prompt_extern() {
  cat <<PROMPT
You are generating a C unit test file for the MacMini Shell project.

## Goal

Write tests/unit/test_${MODULE}.c that exercises the public functions in
${SOURCE}.

## Hard constraints

1. Do NOT create any new header file.
2. Do NOT include "minishell.h", "system_program.h", or any project header
   that transitively pulls in readline, curses, or libft, because those
   libraries are not linked in the unit test build.
3. Instead, forward-declare every function under test with "extern" and supply
   the minimum necessary typedefs / struct stubs for the types it uses.
4. Only include standard C headers (<unistd.h>, <string.h>, <stdlib.h>, etc.)
   plus "unity.h".
5. If a function depends on a project type (t_list, t_root, …) that is opaque
   to the test, forward-declare it as "typedef struct s_X t_X;" and cast or
   pass NULL where appropriate, testing only the paths that do not dereference
   it.
6. Define \`int g_exit_status = 0;\` at file scope — minishell.h declares it
   as extern, so each test binary must supply the definition.

## Project conventions

- Test framework : Unity (tests/unity/unity.h)
- Test file      : tests/unit/test_${MODULE}.c
- setUp() and tearDown() must always be defined (even if empty)
- All test functions: static void, named test_<what>_<expected>()
- main(): UNITY_BEGIN() → RUN_TEST(…) for each → return UNITY_END()
- No heap allocation in tests; use stack buffers where possible
- Cover the happy path, edge cases, and boundary conditions
- Capture stdout via pipe() + dup2() when testing functions that write to fd 1
- Capture stderr via pipe() + dup2() when testing functions that write to fd 2

If the source file uses libft helpers (ft_strlen, ft_strncmp, …) that cannot
be resolved without libft, add stubs at the top of the test file, e.g.:

  #include <string.h>
  size_t ft_strlen(const char *s) { return strlen(s); }
  int    ft_strncmp(const char *a, const char *b, size_t n) { return strncmp(a,b,n); }

## Public functions to test

\`\`\`c
${SIGNATURES:-/* (no signatures extracted — see full source below) */}
\`\`\`

${USED_TYPES:+Project types referenced: ${USED_TYPES}
Provide minimal forward declarations / typedef stubs so the file compiles.
}${CROSS_DEPS:+## Cross-file dependencies to stub

The following functions are called by this module but defined in other source
files (not libft). Provide minimal stubs in the test file:

${CROSS_DEPS}
}
## Full source: ${SOURCE}

\`\`\`c
$(cat "$SOURCE")
\`\`\`

## Output

Respond with the complete, compilable C source for tests/unit/test_${MODULE}.c.
Output only the raw C code — no markdown fences, no tool calls, no explanation.
PROMPT
}

build_prompt() {
  if [[ "$HAS_HEADER" -eq 1 ]]; then
    build_prompt_with_header
  else
    build_prompt_extern
  fi
}

AGENT_CMD="${MACMINI_AGENT_CMD:-}"

if [[ -z "$AGENT_CMD" ]]; then
  {
    echo "================================================================"
    if [[ "$HAS_HEADER" -eq 1 ]]; then
      echo "Header found: ${HEADER}"
    else
      echo "No header found — using extern declarations (no new files needed)."
    fi
    echo "MACMINI_AGENT_CMD not set — printing prompt to stdout."
    echo "Paste everything below into your AI tool, then save the output"
    echo "to tests/unit/test_${MODULE}.c"
    echo "================================================================"
    echo ""
  } >&2
  build_prompt
  exit 0
fi

build_prompt | eval "$AGENT_CMD"
