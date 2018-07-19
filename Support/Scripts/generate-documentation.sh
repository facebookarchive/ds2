#!/usr/bin/env bash
##
## Copyright (c) 2014-present, Facebook, Inc.
## All rights reserved.
##
## This source code is licensed under the University of Illinois/NCSA Open
## Source License found in the LICENSE file in the root directory of this
## source tree. An additional grant of patent rights can be found in the
## PATENTS file in the same directory.
##

# This is used to automatically generate documentation for the project and is
# inspired by https://gist.github.com/domenic/ec8b0fc8ab45f39403dd

source "$(dirname "$0")/common.sh"

SOURCE_BRANCH="master"
TARGET_BRANCH="gh-pages"
DEPLOY_KEY="Support/Testing/Travis/deploy-key.enc"
TARGET_DIRECTORY="html"

do_nothing() {
  echo "exiting: $1"
  exit 0
}

REPO="$(git config remote.origin.url)"
SSH_REPO="${REPO/https:\/\/github.com\//git@github.com:}"
SHA="$(git rev-parse --verify HEAD)"
CURRENT_BRANCH="$(git rev-parse --abbrev-ref HEAD)"

while test $# -gt 0; do
  case "$1" in
    --encryption-label) shift
      ENCRYPTION_LABEL="$1";;
    *) die "Unknown option \`$1'.";;
  esac
  shift
done

git_clone "$REPO" "$TARGET_DIRECTORY" "$TARGET_BRANCH"
cd "$TARGET_DIRECTORY"

rm -rf *
git config user.name "Stephane Sezer (automated commit)"
git config user.email "sas@fb.com"

cd "$OLDPWD"
doxygen Doxyfile
cd "$TARGET_DIRECTORY"

if git diff --name-only --exit-code; then
  do_nothing "No changes to the documentation."
fi

git add --all .
git commit -m "Update documentation." -m "Bump to ${SHA}."


if [ "${TRAVIS_PULL_REQUEST-false}" != "false" ]; then
  do_nothing "Documentation build on a pull request, not pushing."
fi

if [ "${TRAVIS_BRANCH-"$CURRENT_BRANCH"}" != "$SOURCE_BRANCH" ]; then
  do_nothing "Documentation build on branch '$CURRENT_BRANCH', not pushing."
fi

if [ -z "${ENCRYPTION_LABEL}" ]; then
  do_nothing '$ENCRYPTION_LABEL is not set, not pushing.'
fi

ENCRYPTED_KEY_VAR="encrypted_${ENCRYPTION_LABEL}_key"
ENCRYPTED_IV_VAR="encrypted_${ENCRYPTION_LABEL}_iv"
ENCRYPTED_KEY="${!ENCRYPTED_KEY_VAR}"
ENCRYPTED_IV="${!ENCRYPTED_IV_VAR}"
KEY="$(umask 077; mktemp /tmp/deploy-key-XXXXXX)"
openssl aes-256-cbc -K "$ENCRYPTED_KEY" -iv "$ENCRYPTED_IV" -in "../$DEPLOY_KEY" -out "$KEY" -d
eval $(ssh-agent)
ssh-add "$KEY"
git push "$SSH_REPO" "$TARGET_BRANCH"
