#!/bin/bash
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
#
# Fetches the schemas scripts/validate_schema.py needs into DIR. They are downloaded, not
# vendored, and pinned by sha256 so a changed upstream file fails instead of moving the baseline.
#   ECMA-376 5th edition Part 4 (Transitional XSD): ecma-international.org
#   ODF 1.2 and 1.3 RELAX NG: docs.oasis-open.org
# Usage: fetch_schemas.sh DIR
set -euo pipefail

dir="${1:?usage: fetch_schemas.sh DIR}"
mkdir -p "$dir"
cd "$dir"

fetch() { # url sha256 file
	[ -f "$3" ] || curl -fsSL --retry 3 -o "$3" "$1"
	echo "$2  $3" | sha256sum -c --quiet -
}

fetch https://ecma-international.org/wp-content/uploads/ECMA-376-4_5th_edition_december_2016.zip \
	bd25da1109f73762356596918bf5ff8b74a1331642dba5f1c1d1dfc6bed34ecd ecma-376-part4.zip
unzip -qoj ecma-376-part4.zip OfficeOpenXML-XMLSchema-Transitional.zip
unzip -qo OfficeOpenXML-XMLSchema-Transitional.zip -d xsd

odf=https://docs.oasis-open.org/office
fetch $odf/v1.2/os/OpenDocument-v1.2-os-schema.rng \
	ca5d2510858c6f922b20cd27dc76d34959c1a976b9dfc6b5caeadbebfc83dbbb OpenDocument-v1.2-os-schema.rng
fetch $odf/v1.2/os/OpenDocument-v1.2-os-manifest-schema.rng \
	75f972902bc785c15e51fb4e112e2f1b24e2327829eb54786cf8687ee39c7631 OpenDocument-v1.2-os-manifest-schema.rng
fetch $odf/OpenDocument/v1.3/os/schemas/OpenDocument-v1.3-schema.rng \
	40bad03efdbb02825230d357da0aa6ac679934c5bf56c6281752c0c24d58e4e6 OpenDocument-v1.3-schema.rng
fetch $odf/OpenDocument/v1.3/os/schemas/OpenDocument-v1.3-manifest-schema.rng \
	8aee71f03484be112af972d622cc9031280c007b0a454ae8815e8e333c9bdd17 OpenDocument-v1.3-manifest-schema.rng
