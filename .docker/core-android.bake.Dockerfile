# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# ==============================================================================
# MODULE DOCKERFILE
# This file is not meant to be built standalone. It is consumed by the
# docker-bake.hcl files in the parent monorepos.
#
# Core for Android: x2t and the libraries it links, cross-compiled with the NDK
# for one ABI (arm64-v8a or x86_64). The result (stage core-android) is a
# directory of stripped shared libraries for the libx2t JNI wrapper
# (X2tConverter/build/Android/libx2t) and the apps' editor engine.
#
# REQUIRED CONTEXTS:
# - android-ndk: an image with the NDK in ANDROID_NDK_ROOT and the host tools
#   Common/3dParty/build_3rdparty.py needs (CMake, Ninja, clang, Python, perl,
#   autotools, git, curl). The parent defines it, so that core and the apps'
#   own native code are built with the same NDK and C++ runtime.
#
# The build tree and the third-party builds (V8 alone takes hours) stay in a
# BuildKit cache mount per ABI: pruning the build cache rebuilds them.
#
# OPTIONAL SECRETS (as core.bake.Dockerfile): remote_cache_s3_access_key,
# remote_cache_s3_secret_key to upload third-party builds missing from the
# remote cache (Common/3dParty/build_3rdparty_common.py). Downloads need none.
# ==============================================================================

ARG ANDROID_ABI=arm64-v8a
ARG ANDROID_API_LEVEL=26

#### CORE FOR ANDROID ####
FROM android-ndk AS core-android-build
    ARG ANDROID_ABI
    ARG ANDROID_API_LEVEL

    COPY core /core

    RUN --mount=type=cache,id=core-android-${ANDROID_ABI},target=/build-cache \
        --mount=type=secret,id=remote_cache_s3_access_key \
        --mount=type=secret,id=remote_cache_s3_secret_key <<EOF
        set -e
        export REMOTE_CACHE_S3_ACCESS_KEY="$(cat /run/secrets/remote_cache_s3_access_key 2>/dev/null || true)"
        export REMOTE_CACHE_S3_SECRET_KEY="$(cat /run/secrets/remote_cache_s3_secret_key 2>/dev/null || true)"
        # -U: check the third-party deps on every build (done ones are skipped),
        # so new deps in common.cmake are built into the cached tree
        cmake -S /core -B /build-cache/core -G Ninja \
            -UTHIRD_PARTY_PREPARED \
            -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
            -DANDROID_ABI=${ANDROID_ABI} \
            -DANDROID_PLATFORM=android-${ANDROID_API_LEVEL} \
            -DANDROID_STL=c++_shared \
            -DCMAKE_BUILD_TYPE=Release \
            -DEO_CORE_3RD_PARTY_DIR=/build-cache/3rd \
            -DEO_CORE_OUTPUT_DIR=/build-cache/package
        cmake --build /build-cache/core --target x2tlib
        mkdir -p /out
        cp /build-cache/package/*.so /out/
        find /build-cache/core -name libx2t.so -exec cp {} /out/ \;
        # The NDK's toolchain always adds -g: stripped copies (the unstripped ones stay in the cache)
        "${ANDROID_NDK_ROOT}"/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip --strip-unneeded /out/*.so
        ls -1 /out
EOF

FROM scratch AS core-android
    COPY --from=core-android-build /out/ /
