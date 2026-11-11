#!/bin/sh
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Usage: .github/scripts/core-base-image.sh PRODUCT ARCH TAG
#
# Tags the core-base stage of .docker/core.bake.Dockerfile for PRODUCT
# (server, desktop) as TAG. Euro-Office/docker-ci publishes it as
# core-base:<product>-<arch>-<hash>, <hash> being the first 12 characters of
# the Dockerfile's sha256. Without that tag (the Dockerfile changed and
# docker-ci has not rebuilt yet) the stage is built here: it has no COPY, so
# the Dockerfile is fed via stdin with an empty build context. Layers are
# cached by the runner's Docker daemon.

set -eu

product="$1" arch="$2" tag="$3"
hash=$(sha256sum .docker/core.bake.Dockerfile | cut -c1-12)
published="ghcr.io/euro-office/core-base:$product-$arch-$hash"
if docker pull "$published"; then
    docker tag "$published" "$tag"
else
    echo "::notice::$published is not published, building core-base here"
    docker build --target core-base --build-arg PRODUCT="$product" -t "$tag" - < .docker/core.bake.Dockerfile
fi
