#!/bin/bash
set -e

DOCKER_IMAGE=$(grep -A1 container: .github/workflows/build.yml |grep image: | xargs | cut -d ' ' -f2)

# Increment current version to avoid override
DIGIT=$(echo "$DOCKER_IMAGE" | cut -d '.' -f2)
DOCKER_IMAGE=$(echo "$DOCKER_IMAGE" | cut -d '.' -f1).$((DIGIT + 1))

docker build --no-cache -t "$DOCKER_IMAGE" scripts

# Uncomment below to push the image
# docker push "$DOCKER_IMAGE"
