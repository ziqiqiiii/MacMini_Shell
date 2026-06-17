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

build_prompt_with_header() {
  cat <<PROMPT
You are generating a C unit test file for the MacMini Shell project.

## Project conventions

- Test framework: Unity (single-header, located at tests/unity/unity.h)
- Each test file lives at tests/unit/test_<module>.c
- File header comment format:
    /*
     * tests/unit/test_<module>.c
     *
     * Unit tests for <function>(), declared in includes/libs/<module>.h
     * and implemented in src/...<module>.c.
     *
     * Run with:
     *   make unit
     */
- Includes: "unity.h", then the module header, then standard headers
- setUp() and tearDown() are always defined even if empty
- All test functions are static void, named test_<what>_<expected>()
- main() calls UNITY_BEGIN(), then RUN_TEST() for each, then return UNITY_END()
- No dynamic allocation in tests; use stack buffers
- Test names are snake_case and descriptive
- Cover the happy path, edge cases, and boundary conditions

## Makefile note (add this before the pattern rule for \$(UNIT_BIN_DIR)/test_%)

  \$(UNIT_BIN_DIR)/test_${MODULE}: EXTRA_SRC := ${SOURCE}

## Target module: ${MODULE}

### Header: ${HEADER}
\`\`\`c
$(cat "$HEADER")
\`\`\`

### Implementation: ${SOURCE}
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

## Project conventions

- Test framework : Unity (tests/unity/unity.h)
- Test file      : tests/unit/test_${MODULE}.c
- setUp() and tearDown() must always be defined (even if empty)
- All test functions: static void, named test_<what>_<expected>()
- main(): UNITY_BEGIN() → RUN_TEST(…) for each → return UNITY_END()
- No heap allocation in tests; use stack buffers
- Cover the happy path, edge cases, and boundary conditions

## Makefile note (add this before the pattern rule for \$(UNIT_BIN_DIR)/test_%)

  \$(UNIT_BIN_DIR)/test_${MODULE}: EXTRA_SRC := ${SOURCE}

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
