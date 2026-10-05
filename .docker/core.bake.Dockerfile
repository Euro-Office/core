# ==============================================================================
# MODULE DOCKERFILE
# This file is not meant to be built standalone. It is consumed by the
# docker-bake.hcl files in the parent monorepos.
#
# BUILD BASE PER PRODUCT / ARCH
#
#               amd64           arm64
#   server      Rocky Linux 9   Rocky Linux 9
#   desktop     Ubuntu 22.04    Ubuntu 24.04
#
# Server: the deploy target is Rocky 9 (glibc 2.34). The toolchain emits a
#   reference to __libc_single_threaded (glibc 2.32), so the build glibc must
#   be in [2.32, 2.34]. Ubuntu 20.04 (2.31) is below that window, Ubuntu 22.04
#   (2.35) is above it; building on Rocky 9 itself matches the target exactly.
#
# Desktop: the prebuilt Qt host tools fetched via aqt need glibc >= 2.38 on
#   arm64 (Qt builds its arm64 Linux binaries on Ubuntu 24.04), so arm64
#   desktop must build on 24.04. amd64 Qt binaries only need glibc 2.34, so
#   amd64 desktop stays on 22.04. Rocky 9 (2.34) cannot run the arm64 Qt tools.
#
# vcpkg binary cache: plain vcpkg "files" provider. The directory keeps the
# historical /nuget-cache path because desktop-apps.bake.Dockerfile mounts
# its cache there and inherits VCPKG_BINARY_SOURCES from core-base.
# ==============================================================================

ARG BUILD_ROOT
ARG PRODUCT=server
ARG TARGETARCH


#### SERVER BASE (Rocky Linux 9, both arches) ####
FROM rockylinux/rockylinux:9 AS core-base-server

    # EPEL + CRB provide ninja-build, ccache, python3-httplib2 and several
    # -devel packages not in the base channels.
    RUN dnf install -y dnf-plugins-core epel-release && \
        dnf config-manager --set-enabled crb && \
        dnf install -y \
            ca-certificates git zip unzip tar \
            sudo wget gnupg2 openssh-clients ccache \
            gcc gcc-c++ libstdc++-static libatomic make ninja-build pkgconf-pkg-config \
            glib2-devel \
            python3 python3-pip python3-setuptools python3-httplib2 \
            python3.12 python3.12-pip python3.12-setuptools \
            python-unversioned-command \
            autoconf automake libtool findutils \
            perl perl-FindBin perl-IPC-Cmd perl-Data-Dumper \
            diffutils which file xz bzip2 patch \
        && dnf clean all

    # rockylinux:9 ships curl-minimal; --allowerasing swaps in the full curl
    # CLI (needed by some bootstrap scripts).
    RUN dnf install -y --allowerasing curl && dnf clean all

    # The core build scripts need Python >= 3.10 (PEP 604 "X | None" hints).
    # el9's /usr/bin/python3 is 3.9 and must stay that way (dnf depends on it),
    # so the build is pointed at the AppStream python3.12 instead via
    # -DPYTHON_BIN (see core stage).
    ENV CORE_PYTHON=/usr/bin/python3.12

    # vcpkg requires a newer CMake than el9 ships (3.20-3.26). The PyPI wheel
    # ships a prebuilt cmake binary, so no Kitware repo or GitHub access is
    # needed.
    RUN pip3 install --no-cache-dir "cmake>=4" && cmake --version

    # Clang/LLVM 13.0.1 (required for V8 9.x), pinned from the Rocky 9.0 vault.
    # Current Rocky 9.x ships a much newer clang. On el9 the binaries are
    # unversioned (/usr/bin/clang IS 13.0.1), so no update-alternatives needed.
    RUN printf '%s\n' \
        '[rocky90-appstream]' \
        'name=Rocky 9.0 AppStream (vault)' \
        'baseurl=https://dl.rockylinux.org/vault/rocky/9.0/AppStream/$basearch/os/' \
        'enabled=0' \
        'gpgcheck=1' \
        'gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-Rocky-9' \
        > /etc/yum.repos.d/rocky90-vault.repo && \
        dnf install -y \
            --enablerepo=rocky90-appstream \
            --setopt=rocky90-appstream.module_hotfixes=1 \
            clang-13.0.1 clang-libs-13.0.1 clang-devel-13.0.1 \
            llvm-13.0.1 llvm-libs-13.0.1 llvm-devel-13.0.1 \
            lld-13.0.1 \
            compiler-rt-13.0.1 \
        && dnf clean all


#### DESKTOP BASE (Ubuntu: 22.04 on amd64, 24.04 on arm64) ####
FROM ubuntu:22.04 AS ubuntu-desktop-amd64
FROM ubuntu:24.04 AS ubuntu-desktop-arm64
FROM ubuntu-desktop-${TARGETARCH} AS core-base-desktop

    # 22.04 ships Python 3.10, 24.04 ships 3.12; both are new enough.
    ENV CORE_PYTHON=/usr/bin/python3
    ENV TZ=Etc/UTC
    ENV DEBIAN_FRONTEND=noninteractive

    RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
        apt-get update && apt-get install -yq --no-install-recommends \
            ca-certificates git curl zip unzip tar \
            sudo wget ssh gpg ccache \
            build-essential make cmake ninja-build pkg-config \
            libglib2.0-dev \
            python3 python-is-python3 python3-venv python3-setuptools \
            python3-httplib2 \
            lsb-release autoconf automake libtool findutils \
            gn \
        && rm -rf /var/lib/apt/lists/*

    # clang-13 required for V8 9.x. apt.llvm.org's jammy-13 repo also installs
    # cleanly on the noble (24.04) arm64 desktop base.
    RUN mkdir -p /etc/apt/keyrings && \
        wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | \
        gpg --dearmor -o /etc/apt/keyrings/llvm-snapshot.gpg && \
        echo "deb [signed-by=/etc/apt/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-13 main" \
        > /etc/apt/sources.list.d/llvm-13.list && \
        apt-get update && apt-get install -yq --no-install-recommends \
            clang-13 lld-13 llvm-13-dev llvm-13 \
            libc++-13-dev libc++abi-13-dev \
            qemu-user-static binfmt-support && \
        rm -rf /var/lib/apt/lists/*

    # set clang 13 as standard
    RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-13 100 && \
        update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-13 100 && \
        update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-13 100 && \
        update-alternatives --install /usr/bin/llvm-nm llvm-nm /usr/bin/llvm-nm-13 100 && \
        update-alternatives --install /usr/bin/llvm-ranlib llvm-ranlib /usr/bin/llvm-ranlib-13 100 && \
        update-alternatives --install /usr/bin/lld lld /usr/bin/lld-13 100


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

    RUN --mount=type=cache,target=/build-cache \
        --mount=type=cache,target=/nuget-cache,id=vcpkg-binary-cache \
        mkdir -p ${BUILD_ROOT} && \
        cd /build-cache && \
        cmake -GNinja \
        -DVCPKG_MANIFEST_MODE=ON \
        -DVCPKG_MANIFEST_DIR=/core \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DPYTHON_BIN=${CORE_PYTHON} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS_RELEASE="-O3 -w" \
        -DCMAKE_C_FLAGS_RELEASE="-O3 -w" \
        -DEO_CORE_OUTPUT_DIR=/build-cache/package/bin \
        -DEO_CORE_TOOLS_DIR=/build-cache/package/tools \
        /core && \
        cmake --build . && \
        cp -r package/* ${BUILD_ROOT}
