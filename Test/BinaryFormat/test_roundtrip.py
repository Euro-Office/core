# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Self-tests for roundtrip/roundtrip.py, with a stand-in docbuilder (roundtrip/fake_docbuilder.py).

The stand-in runs facts.js in node against a mock Api, so these tests are
skipped where node is not installed. Run:
    python3 -m unittest discover -s Test/BinaryFormat -p 'test_*.py'
"""

import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile
from contextlib import redirect_stdout
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "roundtrip"))

import roundtrip  # noqa: E402

FAKE = HERE / "roundtrip" / "fake_docbuilder.py"
HAVE_NODE = shutil.which("node") is not None


class Script(unittest.TestCase):
    def test_three_passes_and_facts_per_pass(self):
        case = next(c for c in roundtrip.all_cases() if c.name == "inputs/medium.docx")
        text = roundtrip.builder_script(case, Path("/w/medium.docx"), Path("/w"))
        commands = [l for l in text.splitlines() if l.startswith("builder.")]
        self.assertEqual([c.split("(")[0] for c in commands], [
            "builder.SetTmpFolder",
            "builder.OpenFile", "builder.WriteData", "builder.SaveFile", "builder.CloseFile",
            "builder.OpenFile", "builder.WriteData", "builder.SaveFile", "builder.CloseFile",
            "builder.OpenFile", "builder.WriteData", "builder.CloseFile"])
        # WriteData's append flag is only read when quoted (ParceParameters reads quoted args).
        self.assertIn('"jsValue(sFacts)", "false")', text)
        self.assertIn('builder.OpenFile("/w/pass1.docx", "")', text)
        self.assertIn('rtFacts("word")', text)

    def test_cases_cover_all_editors_and_templates(self):
        names = {c.name: c for c in roundtrip.all_cases()}
        for name, kind in [("inputs/medium.odt", "word"), ("inputs/spread_basic.ods", "cell"),
                           ("inputs/simple.pptx", "slide"), ("templates/xlsx.bin", "cell"),
                           ("templates/pptx.bin", "slide"), ("templates/docx.bin", "word")]:
            self.assertEqual(names[name].kind, kind, name)

    def test_template_is_wrapped_as_editor_package(self):
        case = next(c for c in roundtrip.all_cases() if c.name == "templates/docx.bin")
        with tempfile.TemporaryDirectory() as d:
            target = case.prepare(Path(d))
            self.assertEqual(target.suffix, ".doct")
            with zipfile.ZipFile(target) as z:
                self.assertEqual(z.read("Editor.bin"), case.source.read_bytes())

    def test_fact_diff_names_the_fact(self):
        lines = roundtrip.fact_diff({"tableRows": [3, 2]}, {"tableRows": [3]})
        self.assertIn('    tableRows.length: expected 2, got 1', lines)
        self.assertIn('    tableRows[1]: expected 2, got "<missing>"', lines)


@unittest.skipUnless(HAVE_NODE, "node is needed to run facts.js against the mock Api")
class Run(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="roundtrip-test-"))
        self.expected = self.tmp / "expected"
        self.patch = mock.patch.object(roundtrip, "EXPECTED", self.expected)
        self.patch.start()

    def tearDown(self):
        self.patch.stop()
        shutil.rmtree(self.tmp, ignore_errors=True)

    def run_tool(self, *extra, env=None, case="inputs/simple.*"):
        argv = ["roundtrip.py", "--out", str(self.tmp / "out"), "--docbuilder", str(FAKE), "--case", case, *extra]
        out = io.StringIO()
        with mock.patch.object(sys, "argv", argv), mock.patch.dict(os.environ, env or {}), redirect_stdout(out):
            code = roundtrip.main()
        return code, out.getvalue()

    def test_clean_round_trip_passes_and_facts_have_no_errors(self):
        code, out = self.run_tool("--update")
        self.assertEqual(code, 0, out)
        code, out = self.run_tool()
        self.assertEqual(code, 0, out)
        self.assertIn("ok   inputs/simple.pptx", out)
        facts = json.loads((self.expected / "inputs/simple.pptx.json").read_text())
        self.assertEqual(facts["editor"], "slide")
        self.assertEqual(len(facts["facts"]["slides"]), 2)
        self.assertNotIn("error:", json.dumps(facts))

    def test_every_editor_collector_runs(self):
        code, out = self.run_tool("--missing", "warn", case="templates/*")
        self.assertEqual(code, 0, out)
        for name, key in [("docx.bin", "elements"), ("xlsx.bin", "sheets"), ("pptx.bin", "slides")]:
            facts = json.loads((self.tmp / "out/expected/templates" / f"{name}.json").read_text())
            self.assertIn(key, facts["facts"], name)
            self.assertNotIn("error:", json.dumps(facts), name)

    def test_lost_content_fails_with_the_pass_and_fact(self):
        self.run_tool("--update")
        code, out = self.run_tool(env={"FAKE_DROP_ON_SAVE": "1"})
        self.assertEqual(code, 1)
        self.assertIn("FAIL inputs/simple.pptx: facts changed between pass 1 and pass 2", out)
        self.assertIn("facts.slides.length: expected 2, got 1", out)

    def test_changed_first_load_fails_against_expectation(self):
        self.run_tool("--update")
        f = self.expected / "inputs/simple.pptx.json"
        data = json.loads(f.read_text())
        data["facts"]["slides"].append(data["facts"]["slides"][0])
        f.write_text(json.dumps(data))
        code, out = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn("first load differs from expected/inputs/simple.pptx.json", out)

    def test_docbuilder_failure_reports_the_missing_pass(self):
        code, out = self.run_tool("--missing", "warn", env={"FAKE_CRASH_ON_OPEN": "2"})
        self.assertEqual(code, 1)
        self.assertIn("docbuilder exited with 1; no facts from pass 2", out)
        self.assertIn("| error: cannot open file", out)

    def test_missing_expectation(self):
        code, out = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn("no expected/inputs/simple.odp.json", out)
        code, out = self.run_tool("--missing", "warn")
        self.assertEqual(code, 0, out)
        self.assertIn("WARN inputs/simple.odp", out)

    def test_unstable_second_save_warns_or_fails(self):
        self.run_tool("--update")
        code, out = self.run_tool(env={"FAKE_UNSTABLE": "1"})
        self.assertEqual(code, 0, out)
        self.assertIn("WARN inputs/simple.pptx: second save is not stable", out)
        self.assertIn("word/document.xml differs", out)
        self.assertNotIn("docProps", out)
        code, out = self.run_tool("--stability", "fail", env={"FAKE_UNSTABLE": "1"})
        self.assertEqual(code, 1)


@unittest.skipUnless(HAVE_NODE, "node is needed to syntax-check facts.js")
class FactsJs(unittest.TestCase):
    def test_parses_as_es5_script(self):
        r = subprocess.run(["node", "--check", str(HERE / "roundtrip/facts.js")], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr)


if __name__ == "__main__":
    unittest.main()
