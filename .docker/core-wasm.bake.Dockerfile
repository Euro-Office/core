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
    # ON adds x2t.wasm for client-only editor sessions (X2tConverter/build/wasm)
    ARG EO_WASM_X2T=OFF

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

    # x2t.wasm is built on its own, exactly like the build-x2t-wasm CI job
    # that smoke-tests it: inside the root graph the sdkjs wasm modules build
    # some of the same sources with other options and the static link mixes
    # them. -UTHIRD_PARTY_PREPARED lets a third-party version bump (e.g.
    # minizip-ng-wasm) reach the cached build dir.
    RUN --mount=type=cache,id=wasm-x2t-build-cache-${CACHE_BUST},target=/build-cache-x2t \
        --mount=type=cache,id=wasm-ccache,target=/ccache \
        --mount=type=cache,id=wasm-em-cache,target=/em-cache <<EOF
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