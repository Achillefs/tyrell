#!/usr/bin/env bash
# Installs the project pre-commit hook into .git/hooks.
# Run once after cloning. Re-run to refresh after edits.
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
hook_path="$repo_root/.git/hooks/pre-commit"

cat > "$hook_path" <<'HOOK'
#!/usr/bin/env bash
# VP330 pre-commit hook:
#  1. clang-format on staged C++ files (auto-applied; re-stages).
#  2. domain-isolation grep on staged files under domain/.
set -euo pipefail

# 1. Format staged C++ files.
mapfile -t cpp_files < <(git diff --cached --name-only --diff-filter=ACMR \
                          | grep -E '\.(cpp|h)$' || true)
if [ "${#cpp_files[@]}" -gt 0 ]; then
    if ! command -v clang-format >/dev/null; then
        echo "pre-commit: clang-format not found; install it (brew install clang-format)." >&2
        exit 1
    fi
    clang-format -i "${cpp_files[@]}"
    git add "${cpp_files[@]}"
fi

# 2. Domain-isolation grep on staged files under domain/.
mapfile -t domain_files < <(git diff --cached --name-only --diff-filter=ACMR -- 'domain/' || true)
if [ "${#domain_files[@]}" -gt 0 ]; then
    if printf '%s\n' "${domain_files[@]}" \
       | xargs grep -l -E '#include[[:space:]]*<(juce_|sndfile|alsa)' 2>/dev/null; then
        echo "pre-commit: forbidden third-party include in domain/ (spec §3.1)." >&2
        exit 1
    fi
fi
HOOK

chmod +x "$hook_path"
echo "Installed pre-commit hook at $hook_path"
