FROM ubuntu:18.04 AS cef-builder
    ENV DEBIAN_FRONTEND=noninteractive

    RUN apt-get -y update && \
        apt-get -y upgrade && \
        apt-get -y install \
        wget \
        curl \
        git \
        python3.10 \
        libcups2-dev \
        libglib2.0-dev \
        libgtk-3-dev \
        libx11-xcb-dev \
        libxi-dev \
        libxrender-dev \
        libxrandr-dev \
        libxinerama-dev \
        libxcursor-dev \
        libgirepository1.0-dev \
        libglib2.0-dev \
        libcairo2-dev \
        libnss3-dev \
        libgnome-keyring-dev \
        libgbm-dev \
        gperf \
        locales \
        uuid-dev \
        libcurl4-openssl-dev \
        libpulse-dev \
        libasound2-dev \
        libkrb5-dev \
        libpci-dev

    COPY ./nc-build.sh /nc-build.sh

    RUN /nc-build.sh

FROM scratch AS artifacts
COPY --from=cef-builder /cef_binary.tar.bz2 /