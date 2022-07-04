#!/bin/bash
set -e

DOCKER_IMAGE=$(scripts/docker_image.sh 1)
docker build --no-cache -t "$DOCKER_IMAGE" scripts

# Uncomment below to push the image
#docker push "$DOCKER_IMAGE"
echo "push skipped - uncomment above to push image to the hub"
