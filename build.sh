#!/bin/bash
set -e

help() {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  -b               Build X86"
  echo "  -t               Run tests"
  echo "  -o               Run coverage"
  echo "  -e               Build build environment"
  echo "  -c               Clean build"
  echo "  -h               Print this message and exit"
  exit 1
}

if [ $# -eq 0 ]; then
  help
fi

while getopts "btoech" option; do
  case $option in
    b) scripts/build_x86.sh ;;
    t) scripts/run_tests.sh ;;
    o) scripts/coverage.sh ;;
    e) scripts/build_buildenv.sh ;;
    c) scripts/clean.sh ;;
    *) help ;;
  esac
done
