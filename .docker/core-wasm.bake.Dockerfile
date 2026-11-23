# ==============================================================================
# MODULE DOCKERFILE
# This file is not meant to be built standalone. It is consumed by the 
# docker-bake.hcl files in the parent monorepos.
#
# REQUIRED CONTEXTS:
# - core: the core checkout
# ==============================================================================

#### CORE WASM ####
FROM ghcr.io/euro-office/emsdk:5.0.4 AS core-wasm
    ARG BUILD_ROOT
    ARG CACHE_BUST

    ENV BUILD_ROOT=${BUILD_ROOT}
    RUN apt-get update && apt-get install -y --no-install-recommends ccache ninja-build && rm -rf /var/lib/apt/lists/*
    COPY core /core

    # The same build as core's CI (scripts/build.sh --wasm), whose packages
    # downstream repos can use instead of this stage. Prebuilt third-party libs
    # come from the remote cache; the optional remote_cache_s3_* secrets also
    # upload the ones missing there. Emscripten's own cache (the system
    # libraries) stays in the image: the emsdk image ships it prebuilt.
    RUN --mount=type=cache,id=wasm-build-cache-${CACHE_BUST},target=/build-cache-wasm \
        --mount=type=cache,id=wasm-ccache,target=/ccache \
        --mount=type=secret,id=remote_cache_s3_access_key \
        --mount=type=secret,id=remote_cache_s3_secret_key <<EOF
        set -e
        export REMOTE_CACHE_S3_ACCESS_KEY="$(cat /run/secrets/remote_cache_s3_access_key 2>/dev/null)"
        export REMOTE_CACHE_S3_SECRET_KEY="$(cat /run/secrets/remote_cache_s3_secret_key 2>/dev/null)"
        export CCACHE_DIR=/ccache CCACHE_MAXSIZE=1G
        ccache --zero-stats
        /core/scripts/build.sh --wasm --ccache --build-dir /build-cache-wasm --output /build-cache-wasm/dist
        ccache --show-stats
        mkdir -p ${BUILD_ROOT}
        cp -a /build-cache-wasm/dist/. ${BUILD_ROOT}/
EOF
