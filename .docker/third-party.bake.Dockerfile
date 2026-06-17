FROM ubuntu:18.04 AS third-party-builder

    # Prevent interactive prompts
    ENV DEBIAN_FRONTEND=noninteractive

    # Install build dependencies for 18.04
    # Note: python-is-python3 doesn't exist here, so we link it manually
    RUN apt-get update && apt-get install -y \
        build-essential \
        wget \
        tar \
        curl \
        file \
        lsb-release \
        procps \
        git \
        python3 \
        python3-pip \
        pkg-config \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libgl1-mesa-dev \
        libxcb1-dev \
        libdbus-1-dev \
        libicu-dev \
        libfreetype6-dev \
        libfontconfig1-dev \
        libharfbuzz-dev \
        #libxkbcommon-dev \
        #libxkbcommon-x11-dev \
        libxcb-xinput-dev \
        libsm-dev \
        libatspi2.0-dev \
        libglib2.0-dev \
        libxi-dev \
        libasound2-dev \
        libpulse-dev \
        #libssl-dev \
        #libssl1.0-dev \
        libcups2-dev \
        libgtk-3-dev \
        libxcomposite-dev \
        libx11-xcb-dev \
        libxrender-dev \
        libxrandr-dev \
        libxinerama-dev \
        libxcursor-dev \
        libgirepository1.0-dev \
        libcairo2-dev \
        libnss3-dev \
        libgnome-keyring-dev \
        libgbm-dev \
        gperf \
        locales \
        uuid-dev \
        libcurl4-openssl-dev \
        libkrb5-dev \
        libpci-dev \
        && ln -s /usr/bin/python3 /usr/bin/python \
        && rm -rf /var/lib/apt/lists/*

    RUN apt-get update && apt-get install -y \
            wget build-essential libssl-dev zlib1g-dev libbz2-dev \
            libreadline-dev libsqlite3-dev libffi-dev liblzma-dev \
            libncursesw5-dev xz-utils && \
        wget -q https://www.python.org/ftp/python/3.10.13/Python-3.10.13.tgz && \
        tar xzf Python-3.10.13.tgz && \
        cd Python-3.10.13 && \
        ./configure --prefix=/usr/local && \
        make -j"$(nproc)" && \
        make altinstall && \
        cd .. && rm -rf Python-3.10.13* && \
        python3.10 --version

    RUN apt-get update && apt-get install -y sudo && \
        curl 'https://chromium.googlesource.com/chromium/src/+/refs/tags/109.0.5414.120/build/install-build-deps.sh?format=TEXT' \
            | base64 -d > install-build-deps.sh && \
        chmod +x install-build-deps.sh && \
        ./install-build-deps.sh --no-arm --no-chromeos-fonts --no-nacl --no-prompt && \
        python3 -m pip install dataclasses importlib_metadata && \
        locale-gen en_US.UTF-8 && \
        update-locale LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 && \
        rm -rf /var/lib/apt/lists/*


    COPY /core/Common/3dParty /3dParty


    RUN --mount=type=secret,id=nextcloud_user \
        --mount=type=secret,id=nextcloud_pass \
        export NEXTCLOUD_USER="$(cat /run/secrets/nextcloud_user)" && \
        export NEXTCLOUD_PASS="$(cat /run/secrets/nextcloud_pass)" && \
        python3.10 /3dParty/build_3rdparty.py \
            --only=qt,cef \
            /third_party/work \
            /third_party/install

    