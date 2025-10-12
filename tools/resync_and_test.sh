#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: resync_and_test.sh [options]

Options:
  -u, --repo-url URL      Repository URL to clone or update. If omitted, the
                          script attempts to reuse the current repo's origin remote.
  -b, --branch BRANCH     Branch to check out (default: main).
  -d, --destination DIR   Destination directory for the working tree
                          (default: .cache/bdi_repo).
  --skip-tests            Synchronise without running the test suite.
  -h, --help              Show this help message and exit.

The script maintains a cached working tree that is either cloned or fast-forwarded
on subsequent runs. After syncing, the standard test suite (make -C tests run)
is executed unless --skip-tests is provided.
USAGE
}

REPO_URL=""
BRANCH="main"
DEST_DIR=".cache/bdi_repo"
RUN_TESTS=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    -u|--repo-url)
      REPO_URL="$2"
      shift 2
      ;;
    -b|--branch)
      BRANCH="$2"
      shift 2
      ;;
    -d|--destination)
      DEST_DIR="$2"
      shift 2
      ;;
    --skip-tests)
      RUN_TESTS=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ -z "$REPO_URL" ]]; then
  if git rev-parse --git-dir >/dev/null 2>&1; then
    REPO_URL=$(git remote get-url origin 2>/dev/null || true)
  fi
fi

if [[ -z "$REPO_URL" ]]; then
  echo "Error: repository URL not provided and origin remote not found." >&2
  exit 1
fi

mkdir -p "$(dirname "$DEST_DIR")"

if [[ -d "$DEST_DIR/.git" ]]; then
  echo "Updating existing checkout at $DEST_DIR"
  git -C "$DEST_DIR" fetch --all --prune
  git -C "$DEST_DIR" checkout "$BRANCH"
  git -C "$DEST_DIR" reset --hard "origin/$BRANCH"
else
  echo "Cloning $REPO_URL into $DEST_DIR"
  git clone --branch "$BRANCH" --single-branch "$REPO_URL" "$DEST_DIR"
fi

echo
echo "Repository status after sync:"
GIT_DIR="$DEST_DIR/.git" git --git-dir="$DEST_DIR/.git" --work-tree="$DEST_DIR" log -1 --oneline

echo
if [[ "$RUN_TESTS" -eq 1 ]]; then
  if [[ ! -d "$DEST_DIR/tests" ]]; then
    echo "Warning: tests directory not found in $DEST_DIR; skipping test execution." >&2
    exit 0
  fi
  echo "Running test suite (make -C tests run)..."
  make -C "$DEST_DIR/tests" run
else
  echo "Test execution skipped by request."
fi
