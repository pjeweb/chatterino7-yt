"""Tests for release-artifacts.py. Run: python3 -m unittest discover -s scripts/fork"""

import hashlib
import importlib.util
import io
import json
import shutil
import subprocess
import tarfile
import tempfile
import unittest
import zipfile
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

HERE = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location("release_artifacts", HERE / "release-artifacts.py")
ra = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(ra)

TAG = "v7.5.6-yt.1"
SHA = "0123456789abcdef0123456789abcdef01234567"
REPO = "pjeweb/chatterino7-yt"
WORKFLOW = HERE.parents[1] / ".github/workflows/fork-release.yml"
# The check fork-release.yml runs in out/dist before attesting and releasing.
VERIFY_SH = (
    'sha256sum --check --strict SHA256SUMS && '
    'test "$(find . -mindepth 1 | wc -l)" -eq "$(($(wc -l < SHA256SUMS) + 1))"'
)

# File names as build.yml uploads them (merge-multiple download).
BUILD_FILES = [
    "chatterino7-yt-x86_64.AppImage",
    "Chatterino-ubuntu-22.04-x86_64.deb",
    "Chatterino-ubuntu-24.04-x86_64.deb",
    "Chatterino-ubuntu-26.04-x86_64.deb",
    "chatterino.flatpak",
    "chatterino-windows-x86-64-Qt-6.12.0.zip",
    "chatterino-windows-arm-Qt-6.12.0.zip",
    "chatterino7-yt-x86-64-Qt-6.12.0.pdb.7z",
    "chatterino7-yt-arm-Qt-6.12.0.pdb.7z",
    "chatterino-macos-Qt-6.12.0-universal.dmg",
]


def good_run(**overrides):
    run = {
        "id": 42,
        "path": ".github/workflows/build.yml",
        "status": "completed",
        "conclusion": "success",
        "event": "workflow_dispatch",
        "head_sha": SHA,
        "head_branch": TAG,
        "repository": {"full_name": REPO},
        "head_repository": {"full_name": REPO},
    }
    run.update(overrides)
    return run


class CheckRunTest(unittest.TestCase):
    def test_accepts_dispatch_on_tag(self):
        self.assertEqual(ra.check_run(good_run(), REPO, SHA, TAG), [])

    def test_accepts_tag_push(self):
        self.assertEqual(ra.check_run(good_run(event="push"), REPO, SHA, TAG), [])

    def test_rejects_each_mismatch(self):
        cases = {
            "path": ".github/workflows/fork-release.yml",
            "status": "in_progress",
            "conclusion": "failure",
            "head_sha": "f" * 40,
            "head_branch": "chatterino7",  # nightly-type build of the same commit
            "event": "pull_request",
            "repository": {"full_name": "SevenTV/chatterino7"},
            "head_repository": {"full_name": "someone/fork"},
        }
        for key, value in cases.items():
            with self.subTest(key=key):
                errors = ra.check_run(good_run(**{key: value}), REPO, SHA, TAG)
                self.assertEqual(len(errors), 1, errors)
                self.assertIn(key, errors[0])

    def test_rejects_merge_queue_and_missing_fields(self):
        self.assertTrue(ra.check_run(good_run(event="merge_group"), REPO, SHA, TAG))
        self.assertEqual(len(ra.check_run({}, REPO, SHA, TAG)), 8)


class PlanTest(unittest.TestCase):
    def test_maps_every_build_file(self):
        names = [spec.name.format(tag=TAG) for _, spec in ra.plan(BUILD_FILES, TAG)]
        self.assertEqual(len(set(names)), len(BUILD_FILES))
        self.assertIn("chatterino7-yt-v7.5.6-yt.1-windows-x86_64.zip", names)
        self.assertTrue(all(n.startswith(f"chatterino7-yt-{TAG}-") for n in names))

    def test_tolerates_qt_version_bump(self):
        files = [f.replace("6.12.0", "6.13.1") for f in BUILD_FILES]
        self.assertEqual(len(ra.plan(files, TAG)), len(BUILD_FILES))

    def test_fails_closed(self):
        cases = {
            "missing": BUILD_FILES[1:],
            "unknown": BUILD_FILES + ["chatterino-macos-Qt-6.12.0-universal.dmg.sig"],
            "duplicate": BUILD_FILES + ["chatterino-windows-x86-64-Qt-6.13.0.zip"],
            "nightly name": BUILD_FILES + ["Chatterino7TV.Nightly.Installer.exe"],
        }
        for label, files in cases.items():
            with self.subTest(label), self.assertRaises(ra.ReleaseError):
                ra.plan(files, TAG)

    def test_rejects_bad_tags(self):
        for tag in ["nightly-build", "7.5.6", "v7.5", "v7.5.6/../x", "v7.5.6 x", "v1.2.3-"]:
            with self.subTest(tag), self.assertRaises(ra.ReleaseError):
                ra.plan(BUILD_FILES, tag)


class FilesTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)
        self.src = self.tmp / "src"
        self.src.mkdir()
        for i, name in enumerate(BUILD_FILES):
            (self.src / name).write_bytes(f"artifact {i}\n".encode())
        self.dist = self.tmp / "dist"

    def test_select_writes_checksums_matching_sha256sum(self):
        sboms = ra.select(self.src, self.dist, TAG)
        lines = (self.dist / "SHA256SUMS").read_text().splitlines()
        sums = {name: digest for digest, name in (l.split("  ", 1) for l in lines)}
        self.assertEqual(len(sums), len(BUILD_FILES))
        for name, digest in sums.items():
            self.assertEqual(hashlib.sha256((self.dist / name).read_bytes()).hexdigest(), digest)
        content = (self.src / "chatterino.flatpak").read_bytes()
        self.assertEqual(
            sums[f"chatterino7-yt-{TAG}-x86_64.flatpak"], hashlib.sha256(content).hexdigest()
        )
        # sha256sum -c format: digest, two spaces, name; sorted by name.
        self.assertEqual([l.split("  ")[1] for l in lines], sorted(sums))
        # Flatpak and symbols are released, attested and checksummed, not SBOM'd.
        self.assertEqual(len(sboms), 7)
        self.assertEqual({s["unpack"] for s in sboms}, {"zip", "deb", "7z"})
        self.assertFalse(any("flatpak" in s["name"] or "pdb" in s["name"] for s in sboms))

    def test_select_rejects_symlinks_and_nesting(self):
        (self.src / "link").symlink_to(self.src / "chatterino.flatpak")
        with self.assertRaises(ra.ReleaseError):
            ra.select(self.src, self.dist, TAG)
        (self.src / "link").unlink()
        (self.src / "sub").mkdir()
        (self.src / "sub" / "x").write_text("x")
        with self.assertRaises(ra.ReleaseError):
            ra.select(self.src, self.dist, TAG)

    def check(self):
        return subprocess.run(
            ["bash", "-eo", "pipefail", "-c", VERIFY_SH], cwd=self.dist, capture_output=True
        ).returncode

    def test_workflow_runs_this_check(self):
        self.assertEqual(WORKFLOW.read_text().count(VERIFY_SH), 2)

    @unittest.skipUnless(shutil.which("sha256sum"), "sha256sum not installed")
    def test_workflow_check_detects_tampering_extra_and_missing_files(self):
        ra.select(self.src, self.dist, TAG)
        self.assertEqual(self.check(), 0)
        dmg = self.dist / f"chatterino7-yt-{TAG}-macos-universal.dmg"
        original = dmg.read_bytes()
        dmg.write_bytes(b"swapped")
        self.assertNotEqual(self.check(), 0)
        dmg.write_bytes(original)
        (self.dist / "extra.exe").write_bytes(b"x")
        self.assertNotEqual(self.check(), 0)
        (self.dist / "extra.exe").unlink()
        dmg.unlink()
        self.assertNotEqual(self.check(), 0)

    def test_main_reports_errors_without_traceback(self):
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            self.assertEqual(ra.main(["select", str(self.src), str(self.dist), "--tag", "nightly"]), 1)
        self.assertIn("error:", err.getvalue())
        with redirect_stdout(out):
            self.assertEqual(ra.main(["select", str(self.src), str(self.tmp / "d2"), "--tag", TAG]), 0)
        self.assertEqual(len(json.loads(out.getvalue())), 7)

    def test_check_run_cli(self):
        run = self.tmp / "run.json"
        run.write_text(json.dumps(good_run()))
        args = ["check-run", str(run), "--repo", REPO, "--sha", SHA, "--tag", TAG]
        with redirect_stdout(io.StringIO()):
            self.assertEqual(ra.main(args), 0)
        with redirect_stderr(io.StringIO()):
            self.assertEqual(ra.main(args[:-1] + ["v9.9.9"]), 1)


class UnpackTest(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp)

    def test_zip_extracts_and_rejects_traversal(self):
        good = self.tmp / "good.zip"
        with zipfile.ZipFile(good, "w") as z:
            z.writestr("Chatterino2/chatterino7-yt.exe", b"MZ")
        ra.unpack("zip", good, self.tmp / "out")
        self.assertTrue((self.tmp / "out/Chatterino2/chatterino7-yt.exe").is_file())
        for member in ["../evil", "/abs", "C:/x", "a\\..\\..\\b"]:
            bad = self.tmp / "bad.zip"
            with zipfile.ZipFile(bad, "w") as z:
                z.writestr(member, b"x")
            with self.subTest(member), self.assertRaises(ra.ReleaseError):
                ra.unpack("zip", bad, self.tmp / f"bad-{abs(hash(member))}")

    def test_zip_rejects_symlink_member(self):
        bad = self.tmp / "link.zip"
        info = zipfile.ZipInfo("link")
        info.external_attr = 0o120777 << 16
        with zipfile.ZipFile(bad, "w") as z:
            z.writestr(info, "/etc/passwd")
        with self.assertRaises(ra.ReleaseError):
            ra.unpack("zip", bad, self.tmp / "out")

    def test_empty_archive_fails(self):
        empty = self.tmp / "empty.zip"
        zipfile.ZipFile(empty, "w").close()
        with self.assertRaisesRegex(ra.ReleaseError, "nothing"):
            ra.unpack("zip", empty, self.tmp / "out")

    @unittest.skipUnless(shutil.which("dpkg-deb"), "dpkg-deb not installed")
    def test_deb_extracts_and_tar_filter_blocks_escape(self):
        root = self.tmp / "pkg"
        (root / "DEBIAN").mkdir(parents=True)
        (root / "DEBIAN/control").write_text(
            "Package: chatterino7-yt\nVersion: 1.0\nArchitecture: amd64\n"
            "Maintainer: test <t@example.invalid>\nDescription: test\n"
        )
        (root / "usr/bin").mkdir(parents=True)
        (root / "usr/bin/chatterino7-yt").write_bytes(b"\x7fELF")
        for d in [root, *root.rglob("*")]:
            d.chmod(0o755)  # dpkg-deb rejects mkdtemp's 0700
        deb = self.tmp / "t.deb"
        subprocess.run(["dpkg-deb", "--root-owner-group", "--build", str(root), str(deb)],
                       check=True, capture_output=True)
        ra.unpack("deb", deb, self.tmp / "out")
        self.assertTrue((self.tmp / "out/usr/bin/chatterino7-yt").is_file())

        # The data filter is what stands between a hostile tar and the runner.
        buf = io.BytesIO()
        with tarfile.open(fileobj=buf, mode="w") as t:
            t.addfile(tarfile.TarInfo("../escape"), io.BytesIO(b""))
        buf.seek(0)
        with tarfile.open(fileobj=buf) as t, self.assertRaises(tarfile.FilterError):
            t.extractall(self.tmp / "tar-out", filter="data")


if __name__ == "__main__":
    unittest.main()
