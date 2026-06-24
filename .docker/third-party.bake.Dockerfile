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

    COPY /core/Common/3dParty /3dParty


    RUN --mount=type=secret,id=nextcloud_user \
        --mount=type=secret,id=nextcloud_pass \
        export NEXTCLOUD_USER="$(cat /run/secrets/nextcloud_user)" && \
        export NEXTCLOUD_PASS="$(cat /run/secrets/nextcloud_pass)" && \
        python3.10 /3dParty/build_3rdparty.py \
            --only=qt \
            /third_party/work \
            /third_party/install

    