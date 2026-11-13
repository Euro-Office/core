# ==============================================================================
# MODULE DOCKERFILE
# This file is not meant to be built standalone. It is consumed by the
# docker-bake.hcl files in the parent monorepos.
#
# BUILD BASE
#
# Rocky Linux 8 (glibc 2.28) for both products and both arches. A binary
# runs on any glibc at least as new as the one it was built against, so the
# result runs on RHEL/Rocky/Alma 8 and newer, Debian 11+, Ubuntu 20.04+.
#
# Toolchain: el8's own gcc 8 is too old for C++20, so core builds with
#   gcc-toolset-13. Its libstdc++ links the parts that el8's libstdc++.so.6
#   lacks statically (libstdc++_nonshared.a), so the binaries need nothing
#   beyond a stock el8. clang/LLVM 21 from AppStream builds V8 against the
#   same gcc-toolset-13 libstdc++. GCC 13 also keeps the cached
#   third-party archives linkable on hosts with GCC 13 or newer.
#
# Desktop: Qt is built from source on this base (Common/3dParty/qt), because
#   Qt's prebuilt Linux binaries need glibc 2.34 (amd64) and 2.38 (arm64).
#   The desktop base adds the -devel packages Qt's build needs.
#
# vcpkg binary cache: plain vcpkg "files" provider. The directory keeps the
# historical /nuget-cache path because desktop-apps.bake.Dockerfile mounts
# its cache there and inherits VCPKG_BINARY_SOURCES from core-base.
# ==============================================================================

ARG BUILD_ROOT
ARG PRODUCT=server
ARG TARGETARCH


#### SERVER BASE (Rocky Linux 8, both arches) ####
FROM rockylinux/rockylinux:8 AS core-base-server

    ARG TARGETARCH

    # Skip mirrors slower than 200 kB/s. The mirrorlist can hand out one that
    # serves ~10-80 kB/s, which stretches this stage to one or two hours.
    RUN printf 'minrate=200k\ntimeout=30\nmax_parallel_downloads=10\n' >> /etc/dnf/dnf.conf

    # EPEL + PowerTools provide several -devel packages that are not in the
    # base channels.
    RUN dnf install -y dnf-plugins-core epel-release && \
        dnf config-manager --set-enabled powertools && \
        dnf install -y \
            ca-certificates git zip unzip tar curl \
            sudo wget gnupg2 openssh-clients \
            gcc-toolset-13-gcc gcc-toolset-13-gcc-c++ gcc-toolset-13-binutils \
            gcc-toolset-13-libatomic-devel gcc-toolset-13-libstdc++-devel \
            libatomic libatomic-static make pkgconf-pkg-config \
            glib2-devel \
            python3.12 python3.12-pip python3.12-setuptools \
            autoconf automake libtool findutils \
            perl perl-IPC-Cmd perl-Data-Dumper \
            diffutils which file xz bzip2 patch \
        && dnf clean all

    # gcc-toolset-13 first on PATH, as "scl enable gcc-toolset-13" would set it
    ENV PATH="/opt/rh/gcc-toolset-13/root/usr/bin:${PATH}"

    # The core build scripts need Python >= 3.10 (PEP 604 "X | None" hints),
    # and depot_tools/V8 call a bare "python3". On el8 /usr/bin/python3 is an
    # alternatives link (dnf itself uses /usr/libexec/platform-python), so it
    # can point at 3.12 safely. pip installs CMake (vcpkg needs a newer one
    # than el8's 3.26), Ninja (el8 has 1.8.2) and httplib2 for that python.
    RUN alternatives --set python3 /usr/bin/python3.12 && \
        alternatives --set python /usr/bin/python3
    ENV CORE_PYTHON=/usr/bin/python3.12
    RUN python3.12 -m pip install --no-cache-dir "cmake>=4" ninja httplib2 && \
        cmake --version && ninja --version && python3 --version

    # ccache 4 (EPEL 8 has 3.7, which lacks the compiler type and PCH handling
    # scripts/build.sh relies on). Upstream's static binaries, pinned by hash.
    ARG CCACHE_VERSION=4.14.2
    RUN case "${TARGETARCH}" in \
            amd64) a=x86_64;  h=b20ff6415151226ac5a10dfd6839700c694bd67d1871c8fddb266d366ff25bfb ;; \
            arm64) a=aarch64; h=b1b583cd76980b291c4a25b95d8cb2cf1637b4a58083b9c53014780c7548f8d0 ;; \
            *) echo "unsupported arch ${TARGETARCH}" >&2; exit 1 ;; \
        esac && \
        f="ccache-${CCACHE_VERSION}-linux-${a}-musl-static" && \
        curl -fsSLo /tmp/ccache.tar.xz "https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/${f}.tar.xz" && \
        echo "${h}  /tmp/ccache.tar.xz" | sha256sum -c - && \
        tar -xJf /tmp/ccache.tar.xz -C /tmp && \
        install -m 0755 "/tmp/${f}/ccache" /usr/local/bin/ccache && \
        rm -rf /tmp/ccache.tar.xz "/tmp/${f}" && \
        ccache --version | head -1

    # Clang/LLVM for V8 (core itself builds with gcc). AppStream's clang 21 is
    # recent enough. It pulls in gcc-toolset-15, but V8's build points it at
    # the GCC on PATH (Common/3dParty/v8/nc-build.py), so V8 and core use the
    # same libstdc++.
    RUN dnf install -y clang lld llvm compiler-rt && dnf clean all


#### DESKTOP BASE (Rocky Linux 8, both arches) ####
FROM core-base-server AS core-base-desktop

    # What building Qt (Common/3dParty/qt) needs: X11/xcb and Wayland
    # platform plugins, OpenGL/EGL, fonts, the GTK platform theme, DBus,
    # accessibility, and audio for Qt Multimedia. nasm is for FFmpeg's
    # assembly, which Qt Multimedia's FFmpeg backend uses.
    RUN dnf install -y \
            libX11-devel libXext-devel libXrender-devel libXi-devel libXrandr-devel \
            libxcb-devel xcb-util-devel xcb-util-cursor-devel xcb-util-image-devel \
            xcb-util-keysyms-devel xcb-util-renderutil-devel xcb-util-wm-devel \
            libxkbcommon-devel libxkbcommon-x11-devel libxshmfence-devel \
            wayland-devel wayland-protocols-devel \
            mesa-libGL-devel mesa-libEGL-devel mesa-libgbm-devel libdrm-devel \
            vulkan-headers vulkan-loader-devel \
            fontconfig-devel freetype-devel \
            gtk3-devel dbus-devel at-spi2-core-devel cups-devel \
            openssl3-devel libzstd-devel \
            pulseaudio-libs-devel alsa-lib-devel \
            nasm \
        && dnf clean all


#### CORE BASE (shared tail for both products) ####
FROM core-base-${PRODUCT} AS core-base
    ARG BUILD_ROOT=/package

    # Install vcpkg
    WORKDIR /opt
    RUN git clone https://github.com/microsoft/vcpkg.git \
        && cd vcpkg \
        && ./bootstrap-vcpkg.sh

    ENV VCPKG_ROOT=/opt/vcpkg
    ENV PATH="${VCPKG_ROOT}:/root/.cargo/bin:${PATH}"

    # vcpkg binary cache (see header for why the path is /nuget-cache)
    ENV VCPKG_BINARY_SOURCES="clear;files,/nuget-cache,readwrite"
    RUN mkdir -p /nuget-cache

    # Git needs to allow repo paths copied by Docker
    RUN git config --global --add safe.directory '*'

    ENV BUILD_ROOT=${BUILD_ROOT}


#### CORE ####
FROM core-base AS core

    # copy sources right before the final build stage
    COPY core /core

    # Prebuilt third-party libs come from the remote cache (see
    # Common/3dParty/build_3rdparty_common.py). The optional
    # remote_cache_s3_access_key/remote_cache_s3_secret_key secrets also
    # upload the ones missing there.
    # CORE_CCACHE compiles through ccache, for runners whose BuildKit cache
    # doesn't survive the job: they fill and save the mount core-ccache.
    # The build tree holds the third-party installs, so it is per base image:
    # objects from the old Rocky 9 base would leak glibc 2.34 symbols in.
    ARG CORE_CCACHE
    ARG TARGETARCH
    RUN --mount=type=cache,target=/build-cache,id=core-build-el8-${TARGETARCH} \
        --mount=type=cache,target=/nuget-cache,id=vcpkg-binary-cache \
        --mount=type=cache,target=/ccache,id=core-ccache \
        --mount=type=secret,id=remote_cache_s3_access_key \
        --mount=type=secret,id=remote_cache_s3_secret_key \
        export REMOTE_CACHE_S3_ACCESS_KEY="$(cat /run/secrets/remote_cache_s3_access_key 2>/dev/null)" && \
        export REMOTE_CACHE_S3_SECRET_KEY="$(cat /run/secrets/remote_cache_s3_secret_key 2>/dev/null)" && \
        export CCACHE_DIR=/ccache CCACHE_MAXSIZE=2G && \
        mkdir -p ${BUILD_ROOT} && \
        ${CORE_CCACHE:+ccache --zero-stats} && \
        /core/scripts/build.sh --build-dir /build-cache --output /build-cache/package ${CORE_CCACHE:+--ccache} && \
        ${CORE_CCACHE:+ccache --show-stats} && \
        cp -r /build-cache/package/* ${BUILD_ROOT}
