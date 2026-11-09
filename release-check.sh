#!/usr/bin/env bash
# release-check.sh — run before `git tag`
set -euo pipefail

TARGET_SHA="${1:-$(git rev-parse HEAD)}"
TARGET_SHA_SHORT="${TARGET_SHA:0:7}"
REQUIRED=("Tests" "CI/CD" "Cross-Platform Tests" "Static Analysis" "Cross-Platform Sanitization Pipeline")
FAILED=0

echo "Verifying workflows green on $TARGET_SHA_SHORT"
echo ""

for wf in "${REQUIRED[@]}"; do
    # Get the most recent completed run for this workflow on this exact SHA
    RESULT=$(gh run list --workflow="$wf" --branch=main --limit=20 \
             --json headSha,conclusion,status \
             --jq ".[] | select(.headSha == \"$TARGET_SHA\" and .status == \"completed\") | .conclusion" \
             2>/dev/null | head -1)

    if [[ -z "$RESULT" ]]; then
        echo "❌ $wf: no completed run found for $TARGET_SHA_SHORT"
        FAILED=1
    elif [[ "$RESULT" != "success" ]]; then
        echo "❌ $wf on $TARGET_SHA_SHORT: $RESULT"
        FAILED=1
    else
        echo "✅ $wf on $TARGET_SHA_SHORT"
    fi
done

if [[ $FAILED -eq 1 ]]; then
    echo ""
    echo "Refusing to tag $TARGET_SHA_SHORT: not all required workflows are green on this exact commit."
    exit 1
fi

echo ""
echo "All required workflows green on $TARGET_SHA_SHORT — safe to tag."
