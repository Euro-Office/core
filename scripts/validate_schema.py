# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Validate the XML parts of documents core writes against the ECMA-376 and ODF schemas.

    validate_schema.py SCHEMA_DIR ALLOWLIST FILE...

SCHEMA_DIR comes from scripts/fetch_schemas.sh (the schemas are fetched, not vendored).
The schema for a part is chosen by the namespace of its root element, parts of other
formats are skipped. OOXML parts are validated as a consumer that only knows the ECMA
namespaces sees them: namespaces listed in mc:Ignorable (w14, x14, ...) are dropped and
mc:AlternateContent is replaced by its fallback, as the transitional schemas do not
allow them. Exit status 1 if a part fails and no ALLOWLIST entry covers the failure.

ALLOWLIST lines: `file glob | part glob | message regex  # issue link` (fields split on ` | `, the
regex may contain `|`). The link is required.
"""
import fnmatch
import re
import sys
import zipfile
from pathlib import Path

from lxml import etree

NS = "http://schemas.openxmlformats.org/"
OOXML_SCHEMAS = {
    NS + "wordprocessingml/2006/main": "xsd/wml.xsd",
    NS + "spreadsheetml/2006/main": "xsd/sml.xsd",
    NS + "presentationml/2006/main": "xsd/pml.xsd",
    NS + "drawingml/2006/main": "xsd/dml-main.xsd",
    NS + "drawingml/2006/chart": "xsd/dml-chart.xsd",
    NS + "drawingml/2006/spreadsheetDrawing": "xsd/dml-spreadsheetDrawing.xsd",
    NS + "officeDocument/2006/extended-properties": "xsd/shared-documentPropertiesExtended.xsd",
}
NAMESPACE_OF = {schema: namespace for namespace, schema in OOXML_SCHEMAS.items()}
XML_SCHEMA = Path(__file__).with_name("xml-namespace.xsd").resolve()
ODF_OFFICE = "urn:oasis:names:tc:opendocument:xmlns:office:1.0"
ODF_MANIFEST = "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0"
# (root namespace, declared version) -> schema. A manifest root without a version is ODF 1.2.
ODF_SCHEMAS = {
    (ODF_OFFICE, "1.2"): "OpenDocument-v1.2-os-schema.rng",
    (ODF_OFFICE, "1.3"): "OpenDocument-v1.3-schema.rng",
    (ODF_MANIFEST, "1.2"): "OpenDocument-v1.2-os-manifest-schema.rng",
    (ODF_MANIFEST, "1.3"): "OpenDocument-v1.3-manifest-schema.rng",
}
MC = "http://schemas.openxmlformats.org/markup-compatibility/2006"
XML = "http://www.w3.org/XML/1998/namespace"


def to_ecma(root):
    """What a consumer that only knows the ECMA namespaces reads (ECMA-376 Part 3, markup compatibility).

    Namespaces listed in mc:Ignorable are dropped, mc:AlternateContent is replaced by its fallback. Anything else
    foreign stays and fails validation: Word reports a part with an unknown, not ignorable namespace as unreadable.
    """
    ignorable = set()
    for element in root.iter():
        for prefix in (element.get("{%s}Ignorable" % MC) or "").split():
            ignorable.add(element.nsmap.get(prefix))
    for alternate in list(root.iter("{%s}AlternateContent" % MC)):
        fallback = alternate.find("{%s}Fallback" % MC)
        parent, index = alternate.getparent(), alternate.getparent().index(alternate)
        for offset, child in enumerate(list(fallback) if fallback is not None else []):
            parent.insert(index + offset, child)
        parent.remove(alternate)
    for element in list(root.iter()):
        if not isinstance(element.tag, str):
            continue
        if element is not root and etree.QName(element).namespace in ignorable:
            element.getparent().remove(element)
            continue
        for name in list(element.attrib):
            namespace = etree.QName(name).namespace
            if namespace in ignorable or namespace == MC:
                del element.attrib[name]
    return root


def _short(message):
    """Namespace URIs and the list of expected elements make a message unstable and unreadable."""
    return re.sub(r"\{[^}]*\}", "", message).split(" Expected is")[0]


def load_allowlist(path):
    entries = []
    for number, line in enumerate(Path(path).read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        rule, _, link = line.partition("#")
        fields = [field.strip() for field in rule.split(" | ", 2)]
        if len(fields) != 3 or "https://" not in link:
            raise ValueError(f"{path}:{number}: expected `file glob | part glob | message regex  # https://issue`")
        entries.append((fields[0], fields[1], re.compile(fields[2]), number))
    return entries


class Validator:
    def __init__(self, schema_dir, allowlist=()):
        self.schema_dir = Path(schema_dir)
        self.allowlist = allowlist
        self.used = set()
        self._schemas = {}
        self.validated = 0
        self.skipped = 0

    def _schema(self, relative):
        if relative not in self._schemas:
            path = self.schema_dir / relative
            if relative.endswith(".rng"):
                self._schemas[relative] = etree.RelaxNG(etree.parse(str(path)))
            else:
                # the ECMA schemas import the xml: namespace without a location, so import it first
                wrapper = (
                    '<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">'
                    f'<xs:import namespace="{XML}" schemaLocation="{XML_SCHEMA.as_uri()}"/>'
                    f'<xs:import namespace="{NAMESPACE_OF[relative]}" schemaLocation="{path.resolve().as_uri()}"/>'
                    "</xs:schema>"
                )
                self._schemas[relative] = etree.XMLSchema(etree.fromstring(wrapper))
        return self._schemas[relative]

    def _allowed(self, source, part, message):
        for glob, part_glob, pattern, number in self.allowlist:
            if fnmatch.fnmatch(source, glob) and fnmatch.fnmatch(part, part_glob) and pattern.search(message):
                self.used.add(number)
                return True
        return False

    def part_problems(self, source, part, data):
        """Problems of one XML part, after the allowlist."""
        try:
            root = etree.fromstring(data, etree.XMLParser(resolve_entities=False, no_network=True)).getroottree().getroot()
        except etree.XMLSyntaxError as error:
            return [f"not well-formed: {error}"]
        namespace = etree.QName(root).namespace
        if namespace in OOXML_SCHEMAS:
            schema, root = self._schema(OOXML_SCHEMAS[namespace]), to_ecma(root)
        elif namespace in (ODF_OFFICE, ODF_MANIFEST):
            version = root.get("{%s}version" % namespace, "1.2")
            if (namespace, version) not in ODF_SCHEMAS:
                return [f"ODF {version} has no schema here, add it to fetch_schemas.sh and ODF_SCHEMAS"]
            schema = self._schema(ODF_SCHEMAS[namespace, version])
        else:
            self.skipped += 1
            return []
        self.validated += 1
        if schema.validate(root):
            return []
        messages = (_short(e.message) for e in schema.error_log)
        return [m for m in messages if not self._allowed(source, part, m)]

    def file_problems(self, path):
        found = []
        source = Path(path).name
        try:
            with zipfile.ZipFile(path) as archive:
                for part in archive.namelist():
                    if part.endswith((".xml", ".rels")):
                        found += [f"{part}: {problem}" for problem in self.part_problems(source, part, archive.read(part))]
        except (OSError, zipfile.BadZipFile) as error:
            found.append(f"cannot read: {error}")
        return found


def main(schema_dir, allowlist, paths):
    entries = load_allowlist(allowlist)
    validator = Validator(schema_dir, entries)
    failed = False
    for path in paths:
        for problem in validator.file_problems(path):
            print(f"{path}: {problem}", file=sys.stderr)
            failed = True
    print(f"validated {validator.validated} parts, skipped {validator.skipped} of other formats")
    for glob, part_glob, pattern, number in entries:
        if number not in validator.used:
            print(f"allowlist line {number} matched nothing: {glob} | {part_glob} | {pattern.pattern}")
    return 1 if failed else 0


if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3:]))
