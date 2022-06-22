#!/bin/bash
set -e

DOCKER_ARGS="--privileged \
--rm=true \
-it \
-u user \
-v $HOME:/home/user \
-v empty:/home/user/.local/bin \
-v $(pwd):/workspaces/$(basename "$(pwd)") \
-w /workspaces/$(basename "$(pwd)")"

DOCKER_IMAGE=$(scripts/docker_image.sh)

# Just run the command if we are inside a docker container
if [ -f /.dockerenv ]; then
    $*
else
    DOCKER_COMMAND="docker run $DOCKER_ARGS $DOCKER_IMAGE $*"
    $DOCKER_COMMAND
fi
