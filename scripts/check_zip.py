# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Check the ZIP container of files core writes with a reader that is not core.

Core writes DOCX/XLSX/PPTX/ODT/ODS/ODP through minizip. A writer change can
produce files that our own reader accepts but Word or LibreOffice reject, for
example a STORED entry with a trailing data descriptor. This walks the local
and central headers by hand and also lets Python's zipfile verify every CRC.

    check_zip.py FILE...        exit status 1 if any file has a problem
"""
import struct
import sys
import zipfile
from io import BytesIO
from pathlib import PurePosixPath

LOCAL_SIG = 0x04034B50
CENTRAL_SIG = 0x02014B50
EOCD_SIG = 0x06054B50
DESCRIPTOR_SIG = 0x08074B50

FLAG_ENCRYPTED = 0x1
FLAG_DESCRIPTOR = 0x8
STORED = 0

ODF_MIMETYPES = {
    "odt": b"application/vnd.oasis.opendocument.text",
    "ods": b"application/vnd.oasis.opendocument.spreadsheet",
    "odp": b"application/vnd.oasis.opendocument.presentation",
}
OOXML_EXTENSIONS = {"docx", "xlsx", "pptx"}


def _entries(data):
    """Yield (name, flags, method, crc, csize, usize, local_offset) from the central directory."""
    eocd = data.rfind(struct.pack("<I", EOCD_SIG), max(0, len(data) - 0x10016))
    if eocd < 0:
        raise ValueError("no end of central directory record")
    count, _size, cd_offset = struct.unpack_from("<HII", data, eocd + 10)
    if count == 0xFFFF or cd_offset == 0xFFFFFFFF:
        raise NotImplementedError("zip64")
    pos = cd_offset
    for _ in range(count):
        if struct.unpack_from("<I", data, pos)[0] != CENTRAL_SIG:
            raise ValueError(f"bad central directory signature at {pos}")
        flags, method = struct.unpack_from("<HH", data, pos + 8)
        crc, csize, usize, nlen, elen, clen = struct.unpack_from("<IIIHHH", data, pos + 16)
        offset = struct.unpack_from("<I", data, pos + 42)[0]
        name = data[pos + 46 : pos + 46 + nlen].decode("utf-8", "replace")
        yield name, flags, method, crc, csize, usize, offset
        pos += 46 + nlen + elen + clen


def problems(data, extension):
    """Return a list of problems found in the archive bytes for a file of the given extension."""
    found = []
    try:
        entries = list(_entries(data))
    except NotImplementedError:
        # ponytail: zip64 headers are not walked, only zipfile checks them. Add when an output needs zip64.
        return _with_zipfile(data, extension, None)
    except (ValueError, struct.error) as error:
        return [f"unreadable central directory: {error}"]

    seen = set()
    for index, (name, flags, method, crc, csize, usize, offset) in enumerate(entries):
        where = f"entry {name!r}"
        path = PurePosixPath(name)
        if not name or name.startswith("/") or "\\" in name or ".." in path.parts:
            found.append(f"{where}: unsafe name")
        if name.lower() in seen:
            found.append(f"{where}: duplicate name")
        seen.add(name.lower())
        if flags & FLAG_ENCRYPTED:
            found.append(f"{where}: encrypted")

        try:
            sig, _ver, lflags, lmethod = struct.unpack_from("<IHHH", data, offset)
            lcrc, lcsize, lusize, lnlen, lelen = struct.unpack_from("<IIIHH", data, offset + 14)
        except struct.error:
            found.append(f"{where}: local header outside the file")
            continue
        if sig != LOCAL_SIG:
            found.append(f"{where}: no local header at offset {offset}")
            continue
        if data[offset + 30 : offset + 30 + lnlen].decode("utf-8", "replace") != name:
            found.append(f"{where}: local and central names differ")
        if lmethod != method:
            found.append(f"{where}: local method {lmethod}, central method {method}")

        if lflags & FLAG_DESCRIPTOR:
            if method == STORED:
                # Readers cannot find the end of a STORED entry without sizes. LibreOffice rejects it.
                found.append(f"{where}: STORED with a data descriptor")
            end = offset + 30 + lnlen + lelen + csize
            if data[end : end + 4] == struct.pack("<I", DESCRIPTOR_SIG):
                end += 4
            descriptor = struct.unpack_from("<III", data, end) if end + 12 <= len(data) else None
            if descriptor != (crc, csize, usize):
                found.append(f"{where}: data descriptor {descriptor} differs from central header {(crc, csize, usize)}")
        elif (lcrc, lcsize, lusize) != (crc, csize, usize):
            found.append(
                f"{where}: local header crc/sizes {(lcrc, lcsize, lusize)} differ from central header {(crc, csize, usize)}"
            )

        if extension in ODF_MIMETYPES and index == 0:
            if name != "mimetype":
                found.append(f"{where}: ODF needs mimetype as the first entry")
            if method != STORED:
                found.append(f"{where}: ODF mimetype must be STORED")
            if lelen:
                found.append(f"{where}: ODF mimetype must not have an extra field")

    return found + _with_zipfile(data, extension, {name for name, *_ in entries})


def _with_zipfile(data, extension, names):
    """Required parts, mimetype content and CRCs, read by Python's zipfile. names=None: take them from the archive."""
    found = []
    try:
        with zipfile.ZipFile(BytesIO(data)) as archive:
            names = set(archive.namelist()) if names is None else names
            if extension in OOXML_EXTENSIONS:
                for part in ("[Content_Types].xml", "_rels/.rels"):
                    if part not in names:
                        found.append(f"missing {part}")
            if extension in ODF_MIMETYPES and "mimetype" not in names:
                found.append("missing mimetype")
            bad = archive.testzip()
            if bad:
                found.append(f"entry {bad!r}: zipfile reports a CRC or data error")
            if extension in ODF_MIMETYPES and "mimetype" in names:
                if archive.read("mimetype") != ODF_MIMETYPES[extension]:
                    found.append(f"mimetype content does not match .{extension}")
    except Exception as error:  # a corrupt archive can raise anything (BadZipFile, ValueError, zlib.error), all are findings
        found.append(f"zipfile cannot read the archive: {error}")
    return found


def main(paths):
    failed = False
    for path in paths:
        try:
            with open(path, "rb") as file:
                data = file.read()
        except OSError as error:
            print(f"{path}: {error}", file=sys.stderr)
            failed = True
            continue
        for problem in problems(data, path.rsplit(".", 1)[-1].lower()):
            print(f"{path}: {problem}", file=sys.stderr)
            failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
