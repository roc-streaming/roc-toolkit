#! /bin/bash
set -xe

brew update

brew tap Homebrew/bundle
brew bundle --file=scripts/travis/osx/Brewfile
