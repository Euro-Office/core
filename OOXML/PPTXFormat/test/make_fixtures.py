#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Generate the pptx_comments_test fixtures (see OOXML/PPTXFormat/test).

ExampleFiles/modern_comments.pptx is committed as the seed: a minimal
presentation carrying modern comments (the Microsoft `p188` extension) with
three GUID-identified authors (Alice Author / Bob Builder / Carol Carter) and
three comments in deliberately shuffled order (Carol, Alice, Bob). It
originates from the reproducer discussed in Euro-Office/core#143.

This script derives the two variants next to the seed:

- classic_comments.pptx — the same document downgraded to classic comments
  (`p:cmAuthorLst` integer author ids 0/1/2, `p:cmLst` comments with
  `authorId` 2/0/1), the way an older PowerPoint or a lossy editor would
  save it.
- modern_comments_no_author_ids.pptx — the modern authors stripped of their
  GUID id attributes (neither id nor idx), exercising the guarded
  `CommentAuthor::toXmlWriter` path.

Usage:
    python3 make_fixtures.py <seed.pptx>            # write both variants next to it
    python3 make_fixtures.py --check <ExampleFiles> # verify the three fixtures

Only uses the Python standard library.
"""

import os
import re
import sys
import zipfile

AUTHORS_PART = "ppt/authors.xml"
MODERN_COMMENTS_DIR = "ppt/comments/"

GUID2INT = {
    "{11111111-1111-1111-1111-111111111111}": "0",  # Alice Author
    "{22222222-2222-2222-2222-222222222222}": "1",  # Bob Builder
    "{33333333-3333-3333-3333-333333333333}": "2",  # Carol Carter
}

REL_COMMENT_AUTHORS = (
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/commentAuthors"
)
REL_COMMENTS = (
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments"
)

CT_COMMENT_AUTHORS = (
    "application/vnd.openxmlformats-officedocument.presentationml.commentAuthors+xml"
)
CT_COMMENTS = "application/vnd.openxmlformats-officedocument.presentationml.comments+xml"


def read_part(zin, name):
    return zin.read(name).decode("utf-8")


def modern_comment_part(zin):
    names = [
        n
        for n in zin.namelist()
        if n.startswith(MODERN_COMMENTS_DIR) and n.endswith(".xml")
    ]
    if len(names) != 1:
        raise SystemExit("expected exactly one modern comments part, found: %r" % (names,))
    return names[0], read_part(zin, names[0])


def fix_relationship(data, fragment, new_type, new_target):
    # Replace Type and Target of the <Relationship> whose Target contains
    # `fragment`. The element is self-closing (... />) and its attribute
    # order is not guaranteed, so match on the Target only.
    pattern = re.compile(
        r"<Relationship\b[^>]*Target=\"[^\"]*%s[^\"]*\"[^>]*/?>" % re.escape(fragment)
    )
    m = pattern.search(data)
    if not m:
        raise SystemExit("no relationship targets %r" % fragment)
    element = re.sub(r"Type=\"[^\"]*\"", "Type=\"%s\"" % new_type, m.group(0))
    element = re.sub(r"Target=\"[^\"]*\"", "Target=\"%s\"" % new_target, element)
    return data[: m.start()] + element + data[m.end():]


def classic_authors(modern_authors):
    a = modern_authors.replace("p188:authorLst", "p:cmAuthorLst")
    a = a.replace("p188:author", "p:cmAuthor")
    a = re.sub(r"\s+(userId|providerId)=\"[^\"]*\"", "", a)
    a = re.sub(r"\s+xmlns:p188=\"[^\"]*\"", "", a)
    for guid, idx in GUID2INT.items():
        a = a.replace(" id=\"%s\"" % guid, " id=\"%s\"" % idx)
    if "p188" in a:
        raise SystemExit("p188 residue in derived classic authors part:\n%s" % a[:400])
    if "xmlns:p=" not in a:
        a = a.replace(
            "<p:cmAuthorLst",
            "<p:cmAuthorLst"
            " xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"",
            1,
        )
    return a


def classic_comments(modern_comments_xml):
    cms = re.findall(r"<p188:cm\b([^>]*)>(.*?)</p188:cm>", modern_comments_xml, re.S)
    if len(cms) != len(GUID2INT):
        raise SystemExit(
            "expected %d modern comments, found %d" % (len(GUID2INT), len(cms))
        )
    parts = []
    for n, (attrs, body) in enumerate(cms, 1):
        guid = re.search(r"authorId=\"([^\"]+)\"", attrs)
        date = re.search(r"(?:dt|created)=\"([^\"]+)\"", attrs)
        text = re.search(r"<a:t>(.*?)</a:t>", body, re.S)
        if not guid or guid.group(1) not in GUID2INT:
            raise SystemExit(
                "comment %d has an unmapped authorId: %r" % (n, guid and guid.group(1))
            )
        parts.append(
            "<p:cm id=\"%d\" authorId=\"%s\" dt=\"%s\" idx=\"%d\">"
            "<p:pos x=\"100\" y=\"100\"/>"
            "<p:text>%s</p:text></p:cm>"
            % (
                n,
                GUID2INT[guid.group(1)],
                date.group(1) if date else "",
                n,
                text.group(1) if text else "",
            )
        )
    return (
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<p:cmLst xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
        " xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
        + "".join(parts)
        + "</p:cmLst>"
    )


def build_classic(seed_path, out_path):
    zin = zipfile.ZipFile(seed_path)
    authors = read_part(zin, AUTHORS_PART)
    _comments_name, comments_xml = modern_comment_part(zin)

    authors_xml = classic_authors(authors)
    comments_xml = classic_comments(comments_xml)

    with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as zout:
        for item in zin.infolist():
            name = item.filename
            if name == AUTHORS_PART or name.startswith(MODERN_COMMENTS_DIR):
                continue
            data = zin.read(name)
            if name == "[Content_Types].xml":
                s = data.decode("utf-8")
                s, n_authors = re.subn(
                    r"<Override PartName=\"/ppt/authors\.xml\"[^>]*>",
                    "<Override PartName=\"/ppt/commentAuthors.xml\""
                    " ContentType=\"%s\"/>" % CT_COMMENT_AUTHORS,
                    s,
                )
                s, n_comments = re.subn(
                    r"<Override PartName=\"[^\"]*modernComment[^\"]*\"[^>]*>",
                    "<Override PartName=\"/ppt/comments1.xml\""
                    " ContentType=\"%s\"/>" % CT_COMMENTS,
                    s,
                )
                if n_authors != 1 or n_comments != 1:
                    raise SystemExit(
                        "unexpected [Content_Types].xml overrides (%d authors, %d comments)"
                        % (n_authors, n_comments)
                    )
                data = s.encode("utf-8")
            elif name == "ppt/_rels/presentation.xml.rels":
                data = fix_relationship(
                    data.decode("utf-8"),
                    "authors.xml",
                    REL_COMMENT_AUTHORS,
                    "commentAuthors.xml",
                ).encode("utf-8")
            elif re.match(r"ppt/slides/_rels/slide\d+\.xml\.rels$", name):
                s = data.decode("utf-8")
                if "modernComment" in s:
                    data = fix_relationship(
                        s, "modernComment", REL_COMMENTS, "../comments1.xml"
                    ).encode("utf-8")
            zout.writestr(item, data)
        zout.writestr("ppt/commentAuthors.xml", authors_xml.encode("utf-8"))
        zout.writestr("ppt/comments1.xml", comments_xml.encode("utf-8"))
    print("wrote", out_path)


def build_no_ids(seed_path, out_path):
    zin = zipfile.ZipFile(seed_path)
    authors = read_part(zin, AUTHORS_PART)
    stripped = re.sub(r"\s+id=\"\{[0-9a-fA-F\-]+\}\"", "", authors)
    if " id=" in stripped:
        raise SystemExit("author id attributes remain after stripping:\n%s" % stripped[:400])
    with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as zout:
        for item in zin.infolist():
            data = zin.read(item.filename)
            if item.filename == AUTHORS_PART:
                data = stripped.encode("utf-8")
            zout.writestr(item, data)
    print("wrote", out_path)


def check(dir_path):
    def part(z, name):
        return z.read(name).decode("utf-8")

    ok = True

    seed = zipfile.ZipFile(os.path.join(dir_path, "modern_comments.pptx"))
    a = part(seed, AUTHORS_PART)
    ok = ok and a.count("<p188:author ") == 3 and "p188:authorLst" in a
    _name, c = modern_comment_part(seed)
    ok = ok and c.count("<p188:cm ") == 3

    classic = zipfile.ZipFile(os.path.join(dir_path, "classic_comments.pptx"))
    ca = part(classic, "ppt/commentAuthors.xml")
    ok = ok and "p:cmAuthorLst" in ca and "p188" not in ca
    ok = ok and all("id=\"%s\"" % i in ca for i in GUID2INT.values())
    cc = part(classic, "ppt/comments1.xml")
    ok = ok and "p:cmLst" in cc and "p188" not in cc and cc.count("<p:cm ") == 3
    ok = ok and "authorId=\"2\"" in cc and "authorId=\"0\"" in cc and "authorId=\"1\"" in cc

    noids = zipfile.ZipFile(os.path.join(dir_path, "modern_comments_no_author_ids.pptx"))
    na = part(noids, AUTHORS_PART)
    ok = ok and " id=" not in na and na.count("<p188:author ") == 3

    print("FIXTURES OK" if ok else "FIXTURES BROKEN")
    return ok


def main(argv):
    if len(argv) == 3 and argv[1] == "--check":
        return 0 if check(argv[2]) else 1
    if len(argv) != 2:
        print(__doc__)
        return 2
    seed = argv[1]
    base = os.path.dirname(os.path.abspath(seed))
    build_classic(seed, os.path.join(base, "classic_comments.pptx"))
    build_no_ids(seed, os.path.join(base, "modern_comments_no_author_ids.pptx"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
