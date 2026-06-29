# ==============================================================================
# MODULE DOCKERFILE
# This file is not meant to be built standalone. It is consumed by the 
# docker-bake.hcl files in the parent monorepos.
#
# REQUIRED CONTEXTS:
# - sdkjs: files from sdkjs
# ==============================================================================

#### CORE WASM ####
FROM ghcr.io/euro-office/emsdk:5.0.4 AS core-wasm
    ARG BUILD_ROOT
    ARG CACHE_BUST
    ARG NUGET_SOURCE_PATH

    ENV BUILD_ROOT=${BUILD_ROOT}
    ENV EM_CACHE=/em-cache
    RUN apt-get update && apt-get install -y --no-install-recommends ccache && rm -rf /var/lib/apt/lists/*
    COPY core /core

    # hash wasm has one dependency in sdkjs which we must copy here (TODO: fix this)
    COPY sdkjs/common/stringserialize.js /sdkjs/common/stringserialize.js

    RUN --mount=type=cache,id=wasm-build-cache-${CACHE_BUST},target=/build-cache-wasm \
        --mount=type=cache,id=wasm-ccache,target=/ccache \
        --mount=type=cache,id=wasm-em-cache,target=/em-cache \
        --mount=type=bind,source=${NUGET_SOURCE_PATH},target=/nuget-cache,rw <<EOF
        set -e
        export CCACHE_DIR=/ccache
        cd /build-cache-wasm
        emcmake cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache \
                      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
                      -DEO_CORE_OUTPUT_DIR=/build-cache-wasm/dist /core
        cmake --build . -- -j$(nproc)
        ccache --show-stats
        echo "=== CCACHE ON DISK ==="
        du -sh /ccache || true
        find /ccache -type f | wc -l || true
        echo "=== EM CACHE ON DISK ==="
        du -sh /em-cache || true
        cp -a /build-cache-wasm/dist/. ${BUILD_ROOT}/
EOF