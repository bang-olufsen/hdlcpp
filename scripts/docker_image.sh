#!/bin/bash
set -e

IMAGE=$(grep -A1 container: .github/workflows/build.yml |grep image: | xargs | cut -d ' ' -f2)

if [ "$1" != "" ]; then
    DIGIT=$(echo "$IMAGE" | cut -d '.' -f2)
    IMAGE=$(echo "$IMAGE" | cut -d '.' -f1).$((DIGIT + 1))
fi

echo "$IMAGE"
