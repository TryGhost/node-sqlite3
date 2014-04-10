#!/bin/bash

set -u

# let's catch the case where we tag but
# forget to increment the package.json version

# check if we are on a tag
if [ `git describe --tags --always HEAD` ]; then
    echo 'looks like we are on a tag'
    if [[ $TRAVIS_BRANCH == `git describe --tags --always HEAD` ]]; then
        echo 'git reports the same tag as travis'
        # now check to make sure package.json `version` matches
        MODULE_VERSION=$(node -e "console.log(require('./package.json').version)")
        if [[ $MODULE_VERSION != $TRAVIS_BRANCH ]] && [[ v$MODULE_VERSION != $TRAVIS_BRANCH ]]; then
            echo "package.json version ($MODULE_VERSION) does not match tag ($TRAVIS_BRANCH)"
            exit 1
        else
            echo "Validation success: package.json ($MODULE_VERSION) matches tag ($TRAVIS_BRANCH)"
        fi
    else
        echo "warning: travis thinks the tag ($TRAVIS_BRANCH) differs from git (`git describe --tags --always HEAD`)"
    fi
fi
