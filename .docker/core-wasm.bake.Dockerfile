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
    # ON adds x2t.wasm for client-only editor sessions (X2tConverter/build/wasm)
    ARG EO_WASM_X2T=OFF

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

    # x2t.wasm is built on its own, exactly like the build-x2t-wasm CI job
    # that smoke-tests it: inside the root graph the sdkjs wasm modules build
    # some of the same sources with other options and the static link mixes
    # them. -UTHIRD_PARTY_PREPARED lets a third-party version bump (e.g.
    # minizip-ng-wasm) reach the cached build dir.
    RUN --mount=type=cache,id=wasm-x2t-build-cache-${CACHE_BUST},target=/build-cache-x2t \
        --mount=type=cache,id=wasm-ccache,target=/ccache <<EOF
        set -e
        [ "${EO_WASM_X2T}" = "ON" ] || exit 0
        export CCACHE_DIR=/ccache
        cd /build-cache-x2t
        emcmake cmake -S /core/X2tConverter/build/wasm -B . \
                      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
                      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
                      -UTHIRD_PARTY_PREPARED \
                      -DEO_CORE_OUTPUT_DIR=/build-cache-x2t/dist
        cmake --build . --target x2t -- -j$(nproc)
        mkdir -p ${BUILD_ROOT}/x2t
        cp -a /build-cache-x2t/dist/x2t/. ${BUILD_ROOT}/x2t/
EOF
