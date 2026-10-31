"""Consumer acceptance harness regressions; only tiny synthetic indexes are built."""
import contextlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/repoindex/src"))
from repoindex import workspace


class FreshCloneTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.base = Path(cls.temporary.name)
        original = cls.base / "original"
        original.mkdir()
        cls.trees = ["LA.UM.8.12.r1-11900-sm8250.0", "android-t-qcom"]
        config = {"trees": cls.trees, "ignore": [], "scopes": {
            "camera": [cls.trees[0] + "/camx"], "frameworks": [cls.trees[1] + "/frameworks"]}}
        (original / "repoindex.json").write_text(json.dumps(config))
        cases = [(cls.trees[0], "camx/src/hwl/bps", "camxbpsnode", "BPSNode", "ExecuteProcessRequest"),
                 (cls.trees[1], "frameworks/av", "AudioFlinger", "AudioFlinger", "checkAttributionSourcePackage")]
        for tree, folder, stem, symbol, method in cases:
            directory = original / tree / folder
            directory.mkdir(parents=True)
            (directory / (stem + ".h")).write_text("class %s {};\n" % symbol)
            (directory / (stem + ".cpp")).write_text("int %s::%s() { return 1; }\n" % (symbol, method))
            for index in range(120):
                (directory / ("padding_%03d_" % index + "x" * 90 + ".cpp")).write_text("")
            with contextlib.redirect_stdout(io.StringIO()):
                workspace.build(original, tree, jobs=1)
        cls.template = cls.base / "template"
        cls.template.mkdir()
        shutil.copy2(original / "repoindex.json", cls.template / "repoindex.json")
        shutil.copytree(original / "index", cls.template / "index")
        shutil.copytree(ROOT / "tools/repoindex/src", cls.template / "tools/repoindex/src",
                        ignore=shutil.ignore_patterns("__pycache__"))

    def fixture(self, name):
        target = self.base / name
        shutil.copytree(self.template, target)
        return target

    def test_guarded_probe_passes_without_site_packages_or_source_trees(self):
        root = self.fixture("passing")
        result = subprocess.run([sys.executable, "-I", "-S", str(ROOT / "scripts/_fresh_clone_probe.py"), str(root)],
                                cwd=self.base, text=True, capture_output=True)
        self.assertEqual(0, result.returncode, result.stderr)
        report = json.loads(result.stdout)
        self.assertTrue(report["completed"])
        self.assertEqual(set(self.trees), set(report["trees"]))
        self.assertFalse(report["site_packages_enabled"])
        self.assertTrue(report["probe_network_process_ingestion_guards"])
        self.assertTrue(all(not (root / name).exists() for name in self.trees))

    def test_missing_chunk_fails_under_optimization_without_rebuild(self):
        root = self.fixture("broken")
        saved = json.loads((root / "index" / self.trees[0] / "manifest.json").read_text())
        (root / "index" / self.trees[0] / saved["parts"][0]["file"]).unlink()
        result = subprocess.run([sys.executable, "-O", "-I", "-S", str(ROOT / "scripts/_fresh_clone_probe.py"), str(root)],
                                cwd=self.base, text=True, capture_output=True)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("Query probe failed", result.stderr)
        self.assertTrue(all(not (root / name).exists() for name in self.trees))
        self.assertEqual([], list((root / ".cache").rglob("*.db")))

    def test_clone_failure_cleans_temporary_checkout_and_reports_failure(self):
        parent = self.base / "clones"
        report = self.base / "failed-report.json"
        result = subprocess.run([sys.executable, str(ROOT / "scripts/check_fresh_clone.py"),
                                 "--repository", str(self.base / "missing-repository"),
                                 "--temp-dir", str(parent), "--report", str(report)],
                                text=True, capture_output=True)
        self.assertEqual(1, result.returncode)
        self.assertFalse(json.loads(report.read_text())["completed"])
        self.assertEqual([], list(parent.iterdir()))


if __name__ == "__main__":
    unittest.main()
