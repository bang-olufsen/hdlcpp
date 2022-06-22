#!/bin/bash
set -e

help() {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  -b               Build x86"
  echo "  -m               Build hdlcpp main"
  echo "  -t               Run tests"
  echo "  -o               Run coverage"
  echo "  -e               Build build environment"
  echo "  -c               Clean build"
  echo "  -s               Create a Docker shell"
  echo "  -h               Print this message and exit"
  exit 1
}

if [ $# -eq 0 ]; then
  help
fi

while getopts "bmtoecsh" option; do
  case $option in
    b) scripts/docker_shell.sh scripts/build_x86.sh ;;
    m) scripts/docker_shell.sh scripts/build_main.sh ;;
    t) scripts/docker_shell.sh scripts/run_tests.sh ;;
    o) scripts/docker_shell.sh scripts/coverage.sh ;;
    e) scripts/build_buildenv.sh ;;
    s) scripts/docker_shell.sh ;;
    c) scripts/clean.sh ;;
    *) help ;;
  esac
done
