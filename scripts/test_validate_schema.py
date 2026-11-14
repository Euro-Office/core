# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
import contextlib
import io
import tempfile
import unittest
import zipfile
from pathlib import Path

try:
    import validate_schema
except ImportError:  # lxml is not installed everywhere scripts/ tests run, the schema job installs it
    validate_schema = None

W = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
ODF_OFFICE = "urn:oasis:names:tc:opendocument:xmlns:office:1.0"

# One tiny schema per format: <w:document><w:p/>+</w:document> and <office:document-meta/> with a version.
WML = f"""<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:xml="http://www.w3.org/XML/1998/namespace"
 targetNamespace="{W}" xmlns:w="{W}" elementFormDefault="qualified" attributeFormDefault="qualified">
 <xs:import namespace="http://www.w3.org/XML/1998/namespace"/>
 <xs:element name="document"><xs:complexType><xs:sequence>
  <xs:element name="p" maxOccurs="unbounded"><xs:complexType><xs:attribute ref="xml:space"/></xs:complexType></xs:element>
 </xs:sequence></xs:complexType></xs:element></xs:schema>"""
RNG = """<grammar xmlns="http://relaxng.org/ns/structure/1.0"><start>
 <element name="office:document-meta" xmlns:office="%s"><attribute name="office:version"><value>%s</value></attribute></element>
</start></grammar>"""


def package(parts):
    path = Path(tempfile.mkdtemp()) / "out.docx"
    with zipfile.ZipFile(path, "w") as archive:
        for name, content in parts.items():
            archive.writestr(name, content)
    return path


@unittest.skipIf(validate_schema is None, "lxml is not installed")
class ValidateSchemaTest(unittest.TestCase):
    def setUp(self):
        self.schemas = Path(tempfile.mkdtemp())
        (self.schemas / "xsd").mkdir()
        (self.schemas / "xsd" / "wml.xsd").write_text(WML)
        for version in ("1.2", "1.3"):
            name = "OpenDocument-v1.2-os-schema.rng" if version == "1.2" else "OpenDocument-v1.3-schema.rng"
            (self.schemas / name).write_text(RNG % (ODF_OFFICE, version))

    def check(self, path, allowlist=()):
        return validate_schema.Validator(self.schemas, allowlist).file_problems(path)

    def doc(self, body):
        return f'<w:document xmlns:w="{W}">{body}</w:document>'

    def test_valid_part_passes(self):
        self.assertEqual(self.check(package({"word/document.xml": self.doc("<w:p/>")})), [])

    def test_schema_violation_is_reported(self):
        problems = self.check(package({"word/document.xml": self.doc("<w:q/>")}))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("word/document.xml", problems[0])

    def test_foreign_namespace_that_is_not_ignorable_is_reported(self):
        xml = f'<w:document xmlns:w="{W}" xmlns:foo="urn:example"><w:p foo:a="1"/><foo:x/></w:document>'
        self.assertEqual(len(self.check(package({"word/document.xml": xml}))), 2)

    def test_extension_namespaces_and_alternate_content_are_what_a_consumer_sees(self):
        xml = (
            f'<w:document xmlns:w="{W}" xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml" '
            'xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006" mc:Ignorable="w14">'
            '<w:p w14:paraId="1"/>'
            '<mc:AlternateContent><mc:Choice Requires="w14"><w14:x/></mc:Choice><mc:Fallback><w:p/></mc:Fallback></mc:AlternateContent>'
            "</w:document>"
        )
        self.assertEqual(self.check(package({"word/document.xml": xml})), [])

    def test_xml_space_is_known(self):
        self.assertEqual(self.check(package({"word/document.xml": self.doc('<w:p xml:space="preserve"/>')})), [])

    def test_unknown_formats_are_skipped_and_counted(self):
        validator = validate_schema.Validator(self.schemas)
        self.assertEqual(validator.file_problems(package({"a/b.xml": "<x/>"})), [])
        self.assertEqual((validator.validated, validator.skipped), (0, 1))

    def test_malformed_part_is_reported(self):
        self.assertIn("not well-formed", self.check(package({"word/document.xml": "<w:document"}))[0])

    def test_odf_schema_follows_the_declared_version(self):
        meta = f'<office:document-meta xmlns:office="{ODF_OFFICE}" office:version="%s"/>'
        self.assertEqual(self.check(package({"meta.xml": meta % "1.2"})), [])
        self.assertEqual(self.check(package({"meta.xml": meta % "1.3"})), [])
        self.assertIn("no schema", self.check(package({"meta.xml": meta % "1.4"}))[0])
        manifest = '<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0"/>'
        (self.schemas / "OpenDocument-v1.2-os-manifest-schema.rng").write_text(
            '<grammar xmlns="http://relaxng.org/ns/structure/1.0"><start><element name="manifest:manifest" '
            'xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0"><empty/></element></start></grammar>'
        )
        self.assertEqual(self.check(package({"META-INF/manifest.xml": manifest})), [])  # no version: ODF 1.2

    def test_allowlist_silences_only_the_matching_failure(self):
        allow = tempfile.mkdtemp() + "/allow.txt"
        Path(allow).write_text("# comment\nout.* | word/*.xml | Element 'q'|Element 'z'  # https://github.com/Euro-Office/core/issues/1\n")
        entries = validate_schema.load_allowlist(allow)
        path = package({"word/document.xml": self.doc("<w:q/>"), "word/styles.xml": self.doc("<w:q/>")})
        validator = validate_schema.Validator(self.schemas, entries)
        self.assertEqual(validator.file_problems(path), [])
        self.assertEqual(validator.used, {2})
        other = validate_schema.Validator(self.schemas, [("other.docx", "*", entries[0][2], 2)])
        self.assertTrue(other.file_problems(path))

    def test_main_fails_on_a_new_failure_and_reports_unused_allowlist_entries(self):
        allow = tempfile.mkdtemp() + "/allow.conf"
        Path(allow).write_text("* | * | never matches  # https://github.com/Euro-Office/core/issues/1\n")
        bad = package({"word/document.xml": self.doc("<w:q/>")})
        with contextlib.redirect_stdout(io.StringIO()) as out, contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(validate_schema.main(self.schemas, allow, [bad]), 1)
        self.assertIn("allowlist line 1 matched nothing", out.getvalue())

    def test_allowlist_entry_needs_an_issue_link(self):
        allow = tempfile.mkdtemp() + "/allow.txt"
        Path(allow).write_text("* | * | anything\n")
        with self.assertRaises(ValueError):
            validate_schema.load_allowlist(allow)


if __name__ == "__main__":
    unittest.main()
