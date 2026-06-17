#!/usr/bin/env bash
# Generates an AI prompt for unit-testing a shell builtin/command source file.
#
# Usage:
#   bash scripts/gen_builtin_tests.sh <module>
#   make ai-builtin-tests MODULE=<module>
#
# <module> is the source file stem, e.g. "09a_echo" for src/09a_echo.c.
#
# The generated prompt tells the AI to:
#   - Include "minishell.h" (UNIT_TEST is defined in TEST_CFLAGS, which
#     excludes readline, curses, and get_next_line automatically)
#   - Use libft.a directly (linked by the Makefile unit test rule)
#   - Stub only cross-file minishell functions that the module calls
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

SOURCE="$(find ./src -name "${MODULE}.c" | head -1)"
if [[ -z "$SOURCE" || ! -f "$SOURCE" ]]; then
  echo "Error: ./src/${MODULE}.c not found under ./src/." >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# Extract non-static public function signatures from the source file.
# In 42 / norminette style, each public function starts at column 0 with its
# return type and name on the same line, followed by '{' on the next line.
# Static functions are prefixed with "static" on the same line.
# ---------------------------------------------------------------------------
extract_public_signatures() {
  awk '
    /^static[[:space:]]/ { next }
    /^[a-z][a-zA-Z0-9_*[:space:]	]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(/ {
      # strip trailing brace or whitespace, append semicolon
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

# ---------------------------------------------------------------------------
# Identify any project-defined types used in the signatures so the AI knows
# they are available via minishell.h.
# ---------------------------------------------------------------------------
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

  # All function calls in the source
  local all_calls
  all_calls="$(grep -oE '[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(' "$src" \
    | sed 's/[[:space:]]*($//' | sort -u || true)"

  # Static functions defined in this file
  local static_fns
  static_fns="$(awk '/^static[[:space:]]/ && /\(/ {
    match($0, /[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(/)
    s = substr($0, RSTART, RLENGTH)
    gsub(/[[:space:]]*\(/, "", s)
    print s
  }' "$src" | sort -u)"

  # Public functions defined in this file
  local public_fns
  public_fns="$(echo "$SIGNATURES" \
    | grep -oE '[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(' \
    | sed 's/[[:space:]]*($//' | sort -u || true)"

  # Filter out libft (ft_*), standard C, control-flow keywords, and local functions
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
      if|while|return|sizeof|NULL) continue ;;
    esac
    # skip functions defined in this file
    echo "$static_fns" | grep -qxF "$fn" && continue
    echo "$public_fns" | grep -qxF "$fn" && continue
    echo "$fn"
  done | sort -u
}

CROSS_DEPS="$(extract_cross_file_deps "$SOURCE")"

build_prompt() {
  cat <<PROMPT
You are generating a C unit test file for the MacMini Shell project.

## Goal

Write tests/unit/test_${MODULE}.c that exercises the public functions in
${SOURCE}.

## Hard constraints

1. Include "minishell.h" — the build flags define UNIT_TEST, which excludes
   readline, curses, and get_next_line headers automatically.
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
5. Only include standard C headers, "unity.h", and "minishell.h".
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
These types are already defined in minishell.h — no forward declarations needed.
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

AGENT_CMD="${MACMINI_AGENT_CMD:-}"

if [[ -z "$AGENT_CMD" ]]; then
  {
    echo "================================================================"
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
