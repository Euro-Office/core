# ==============================================================================
# MODULE DOCKERFILE — Rocky Linux 9 build base
# Not built standalone; consumed by the docker-bake.hcl in the parent monorepo.
#
# WHY ROCKY 9:
#   The toolchain emits a reference to __libc_single_threaded (added in glibc
#   2.32), so the build glibc must be >= 2.32.  deploy target is Rocky 9
#   (glibc 2.34), so the build glibc must also be <= 2.34.  That leaves the
#   window [2.32, 2.34].  Ubuntu 20.04 (2.31) is below it; 22.04 (2.35) is above
#   it.
# ==============================================================================

ARG NUGET_CACHE=local
ARG BUILD_ROOT
ARG NUGET_SOURCE_PATH

#### VCPKG BASE ####
FROM rockylinux:9 AS vcpkg-base

    # EPEL + CRB provide ninja-build, python3-httplib2, qemu-user-static and
    # several -devel packages not in the base channels.
    RUN dnf install -y dnf-plugins-core epel-release && \
        dnf config-manager --set-enabled crb && \
        dnf install -y \
            ca-certificates \
            git \
            zip \
            unzip \
            tar \
            gcc gcc-c++ make \
            pkgconf-pkg-config \
            ninja-build \
            python3 python3-pip \
        && dnf clean all

    # rockylinux:9 ships curl-minimal; --allowerasing swaps in the full curl
    # CLI (needed by some bootstrap scripts).
    RUN dnf install -y --allowerasing curl && dnf clean all

    # vcpkg now requires CMake >= 4.x; el9's system cmake is 3.20–3.26 (too old).
    # The PyPI wheel ships a prebuilt cmake binary, so this avoids needing a
    # Kitware RPM repo or GitHub access.
    RUN pip3 install --no-cache-dir "cmake>=4" && cmake --version

    # Install vcpkg
    WORKDIR /opt
    RUN git clone https://github.com/microsoft/vcpkg.git \
        && cd vcpkg \
        && ./bootstrap-vcpkg.sh

    ENV VCPKG_ROOT=/opt/vcpkg
    ENV PATH="${VCPKG_ROOT}:${PATH}"


    ENV VCPKG_BINARY_SOURCES="clear;files,/nuget-cache,readwrite"

    RUN mkdir -p /nuget-cache

#### CORE BASE ####
FROM vcpkg-base AS core-base
    ARG BUILD_ROOT=/package
    ARG TARGETARCH

    RUN dnf install -y \
            git sudo wget gnupg2 openssh-clients \
            gcc gcc-c++ libstdc++-static make ninja-build pkgconf-pkg-config \
            glib2-devel \
            python3 python3-setuptools python3-httplib2 \
            python-unversioned-command \
            autoconf automake libtool findutils \
            perl perl-FindBin perl-IPC-Cmd perl-Data-Dumper \
            diffutils which file \
        && dnf clean all

    # ---- Clang / LLVM 13.0.1 (matches V8 9.x) pinned from the Rocky 9.0 vault ----
    # Current Rocky 9.x ships clang 18; we pin 13.0.1 from the 9.0 snapshot.
    # On el9 the binaries are unversioned (/usr/bin/clang IS 13.0.1 once
    # installed), so the Ubuntu update-alternatives block is unnecessary.
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

    ENV PATH="/root/.cargo/bin:${PATH}"
    RUN git config --global --add safe.directory '*'

    COPY core /core
    ENV BUILD_ROOT=${BUILD_ROOT}


#### CORE ####
FROM core-base AS core
    ARG NUGET_SOURCE_PATH
    ARG TARGETARCH
    RUN --mount=type=cache,target=/build-cache \
        --mount=type=bind,source=${NUGET_SOURCE_PATH},target=/nuget-cache,rw \
        mkdir -p ${BUILD_ROOT} && \
        cd /build-cache && \
        cmake -GNinja \
        -DVCPKG_MANIFEST_MODE=ON \
        -DVCPKG_MANIFEST_DIR=/core \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS_RELEASE="-O3 -w" \
        -DCMAKE_C_FLAGS_RELEASE="-O3 -w" \
        -DEO_CORE_OUTPUT_DIR=/build-cache/package/bin \
        -DEO_CORE_TOOLS_DIR=/build-cache/package/tools \
        /core && \
        cmake --build . && \
        cp -r package/* ${BUILD_ROOT}
