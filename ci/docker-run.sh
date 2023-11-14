#!/bin/sh -e
#
# create and run docker build env
#
CMD_PATH="$(realpath "$(dirname "$0")")"
BASE_DIR="$(realpath "$CMD_PATH/..")"
. "$BASE_DIR/ci/common_names.sh"
IMAGE_NAME="${PROJECT}-build"

IT="-it"
if [ "${CI}" = true ]; then
    echo "Running in CI"
    . "$BASE_DIR/ci/ignored_sources.sh"
    IT=""
fi

if [ -f "${HOME}/.ssh/asmcov_token" ]; then
    echo "Loading asmcov token from ${HOME}/.ssh/asmcov_token."
    source "${HOME}/.ssh/asmcov_token"
fi

echo "==> create docker image"
cd $BASE_DIR/ci
DOCKER_BUILDKIT=0 \
docker build \
    --build-arg UID=$(id -u) --build-arg GID=$(id -g) \
    --build-arg ASMCOV_URI="${ASMCOV_URI-not available}" \
    --tag ${IMAGE_NAME} -f Dockerfile .

echo "==> run $PROJECT build container"

if ! [ -e "$BASE_DIR"/ci/sshconfig ]; then
    { echo "Host *"
      echo "  User $(id -u -n)"
    } > "$BASE_DIR"/ci/sshconfig
fi

if [ "$SSH_AUTH_SOCK" ]; then
    SSH_AGENT_SOCK=$(readlink -f $SSH_AUTH_SOCK)
    SSH_AGENT_OPTS="-v $SSH_AGENT_SOCK:/run/ssh-agent -e SSH_AUTH_SOCK=/run/ssh-agent"
fi

echo ""
echo "======== Docker run SH Info ==============="
echo "docker run --rm ${IT} $SSH_AGENT_OPTS"
echo "    -v $BASE_DIR:/base/elos"
echo "    -w /base/elos"
echo "    -e GIT_USER_TOKEN=\"${GIT_USER_TOKEN}\""
echo "    -e IGNORE_SOURCES=\"${IGNORE_SOURCES}\""
echo "    -e UNUSED_SOURCES=\"${UNUSED_SOURCES}\""
echo "    --privileged"
echo "    --device=/dev/kmsg"
echo "    $IMAGE_NAME $*"
echo "ls -la ${BASE_DIR} (\$BASE_DIR)"
echo "$(ls -la $BASE_DIR)"
echo "============================================"

docker run --rm ${IT} $SSH_AGENT_OPTS \
    -v $BASE_DIR:/base \
    -w /base \
    -e GIT_USER_TOKEN="${GIT_USER_TOKEN}" \
    -e IGNORE_SOURCES="${IGNORE_SOURCES}" \
    -e UNUSED_SOURCES="${UNUSED_SOURCES}" \
    --privileged \
    --device=/dev/kmsg \
    $IMAGE_NAME $@
