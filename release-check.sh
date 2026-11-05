#!/usr/bin/env bash
# release-check.sh — run before `git tag`
set -euo pipefail

REQUIRED=("Tests" "CI/CD" "Cross-Platform Tests" "Static Analysis")
FAILED=0

for wf in "${REQUIRED[@]}"; do
    RESULT=$(gh run list --workflow="$wf" --branch=main --limit=1 \
             --json conclusion,headSha --jq '.[0] | "\(.conclusion) \(.headSha[0:7])"' 2>/dev/null || echo "unknown ------")
    CONCLUSION=$(echo "$RESULT" | cut -d' ' -f1)
    SHA=$(echo "$RESULT" | cut -d' ' -f2)

    if [[ "$CONCLUSION" != "success" ]]; then
        echo "❌ $wf on $SHA: $CONCLUSION"
        FAILED=1
    else
        echo "✅ $wf on $SHA"
    fi
done

if [[ $FAILED -eq 1 ]]; then
    echo ""
    echo "Refusing to tag: not all required workflows are green."
    exit 1
fi

echo ""
echo "All required workflows green — safe to tag."
