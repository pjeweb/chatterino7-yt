#!/usr/bin/env python3
"""Release helpers for .github/workflows/fork-release.yml.

    release-artifacts.py check-run RUN_JSON --repo R --sha SHA --tag TAG
    release-artifacts.py select SRC DEST --tag TAG
    release-artifacts.py unpack {zip,deb,7z} ARTIFACT DEST

check-run  Fail unless RUN_JSON (GET /repos/R/actions/runs/ID) is a successful
           Build run of this repository's build.yml for TAG at SHA.
select     Map every file downloaded from that run to its release name in DEST
           and write DEST/SHA256SUMS. Fails on missing, duplicate or unknown
           files. Prints [{"name", "unpack"}] for the assets that get an SBOM.
unpack     Extract an artifact so Syft can inventory its contents. Uses 7z
           for AppImage and DMG.
"""

import argparse
import hashlib
import io
import json
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile
from pathlib import Path, PurePosixPath
from typing import NamedTuple

SUMS = "SHA256SUMS"
BUILD_WORKFLOW = ".github/workflows/build.yml"
# Events that can build a tag ref in build.yml (dispatch today; push once
# T13 adds a tag trigger). PR and merge-queue runs are never release inputs.
BUILD_EVENTS = {"workflow_dispatch", "push"}
TAG_RE = re.compile(r"v[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.]+)?")


class Spec(NamedTuple):
    pattern: str  # full match against the file name uploaded by build.yml
    name: str  # release asset name; {tag} is substituted
    unpack: str | None  # how unpack() opens it for the SBOM; None: no SBOM


# One entry per file build.yml uploads. Keep in sync with its upload steps.
SPECS = [
    Spec(r"chatterino7-yt-x86_64\.AppImage", "chatterino7-yt-{tag}-x86_64.AppImage", "7z"),
    Spec(r"Chatterino-ubuntu-22\.04-x86_64\.deb", "chatterino7-yt-{tag}-ubuntu-22.04-x86_64.deb", "deb"),
    Spec(r"Chatterino-ubuntu-24\.04-x86_64\.deb", "chatterino7-yt-{tag}-ubuntu-24.04-x86_64.deb", "deb"),
    Spec(r"Chatterino-ubuntu-26\.04-x86_64\.deb", "chatterino7-yt-{tag}-ubuntu-26.04-x86_64.deb", "deb"),
    # Flatpak bundles are OSTree deltas; nothing here can unpack them.
    Spec(r"chatterino\.flatpak", "chatterino7-yt-{tag}-x86_64.flatpak", None),
    Spec(r"chatterino-windows-x86-64-Qt-[0-9.]+\.zip", "chatterino7-yt-{tag}-windows-x86_64.zip", "zip"),
    Spec(r"chatterino-windows-arm-Qt-[0-9.]+\.zip", "chatterino7-yt-{tag}-windows-arm64-experimental.zip", "zip"),
    Spec(r"chatterino7-yt-x86-64-Qt-[0-9.]+\.pdb\.7z", "chatterino7-yt-{tag}-windows-x86_64-symbols.pdb.7z", None),
    Spec(r"chatterino7-yt-arm-Qt-[0-9.]+\.pdb\.7z", "chatterino7-yt-{tag}-windows-arm64-symbols.pdb.7z", None),
    Spec(r"chatterino-macos-Qt-[0-9.]+-universal\.dmg", "chatterino7-yt-{tag}-macos-universal.dmg", "7z"),
]


class ReleaseError(Exception):
    pass


def check_run(run: dict, repo: str, sha: str, tag: str) -> list[str]:
    """Reasons the run can't supply release artifacts; empty if it can."""
    expected = {
        "path": BUILD_WORKFLOW,
        "status": "completed",
        "conclusion": "success",
        "head_sha": sha,
        "head_branch": tag,
    }
    errors = [
        f"{key} is {run.get(key)!r}, expected {want!r}"
        for key, want in expected.items()
        if run.get(key) != want
    ]
    if run.get("event") not in BUILD_EVENTS:
        errors.append(f"event is {run.get('event')!r}, expected one of {sorted(BUILD_EVENTS)}")
    for key in ("repository", "head_repository"):
        got = (run.get(key) or {}).get("full_name")
        if got != repo:
            errors.append(f"{key} is {got!r}, expected {repo!r}")
    return errors


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def plan(files: list[str], tag: str) -> list[tuple[str, Spec]]:
    """Pair each downloaded file name with its spec, failing closed."""
    if not TAG_RE.fullmatch(tag):
        raise ReleaseError(f"tag {tag!r} is not vMAJOR.MINOR.PATCH[-suffix]")
    matches = {spec: [f for f in files if re.fullmatch(spec.pattern, f)] for spec in SPECS}
    errors = []
    for spec, found in matches.items():
        if len(found) != 1:
            errors.append(f"{spec.pattern}: expected one file, found {found or 'none'}")
    claimed = {f for found in matches.values() for f in found}
    errors += [f"unexpected file {f!r}" for f in sorted(set(files) - claimed)]
    if errors:
        raise ReleaseError("\n".join(errors))
    return [(matches[spec][0], spec) for spec in SPECS]


def select(src: Path, dest: Path, tag: str) -> list[dict]:
    entries = sorted(src.rglob("*"))
    if bad := [p for p in entries if p.is_symlink() or not (p.is_file() or p.is_dir())]:
        raise ReleaseError(f"not a regular file: {bad}")
    files = [p for p in entries if p.is_file()]
    if nested := [p for p in files if p.parent != src]:
        raise ReleaseError(f"unexpected nested file: {nested}")
    pairs = plan([p.name for p in files], tag)
    dest.mkdir(parents=True, exist_ok=False)
    lines, sboms = [], []
    for original, spec in pairs:
        name = spec.name.format(tag=tag)
        shutil.copyfile(src / original, dest / name)
        lines.append(f"{sha256(dest / name)}  {name}\n")
        if spec.unpack:
            sboms.append({"name": name, "unpack": spec.unpack})
    (dest / SUMS).write_text("".join(sorted(lines, key=lambda l: l.split("  ", 1)[1])))
    return sboms


def safe_member(name: str) -> bool:
    path = PurePosixPath(name.replace("\\", "/"))
    return not path.is_absolute() and ".." not in path.parts and ":" not in name


def extract_zip(archive: Path, dest: Path) -> None:
    with zipfile.ZipFile(archive) as z:
        for info in z.infolist():
            # Unix symlinks have S_IFLNK in the high 16 bits.
            if not safe_member(info.filename) or (info.external_attr >> 28) == 0xA:
                raise ReleaseError(f"unsafe zip member {info.filename!r}")
        z.extractall(dest)


def extract_deb(archive: Path, dest: Path) -> None:
    tar = subprocess.run(
        ["dpkg-deb", "--fsys-tarfile", str(archive)], check=True, capture_output=True
    ).stdout
    with tarfile.open(fileobj=io.BytesIO(tar)) as t:
        t.extractall(dest, filter="data")


def extract_7z(archive: Path, dest: Path) -> None:
    # 7z already refuses absolute symlinks; skipping the DMG's /Applications
    # alias keeps that refusal from turning into a failed extraction.
    subprocess.run(
        ["7z", "x", "-y", "-bd", "-bso0", f"-o{dest}", "-xr!Applications", str(archive)],
        check=True,
    )


EXTRACTORS = {"zip": extract_zip, "deb": extract_deb, "7z": extract_7z}


def unpack(kind: str, artifact: Path, dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=False)
    EXTRACTORS[kind](artifact, dest)
    if not any(p.is_file() for p in dest.rglob("*")):
        raise ReleaseError(f"{artifact.name} unpacked to nothing")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("check-run")
    p.add_argument("run_json", type=Path)
    p.add_argument("--repo", required=True)
    p.add_argument("--sha", required=True)
    p.add_argument("--tag", required=True)
    p = sub.add_parser("select")
    p.add_argument("src", type=Path)
    p.add_argument("dest", type=Path)
    p.add_argument("--tag", required=True)
    p = sub.add_parser("unpack")
    p.add_argument("kind", choices=EXTRACTORS)
    p.add_argument("artifact", type=Path)
    p.add_argument("dest", type=Path)
    args = parser.parse_args(argv)

    try:
        if args.cmd == "check-run":
            run = json.loads(args.run_json.read_text())
            if errors := check_run(run, args.repo, args.sha, args.tag):
                raise ReleaseError("\n".join(errors))
            print(f"Build run {run.get('id')} matches {args.tag} at {args.sha}")
        elif args.cmd == "select":
            print(json.dumps(select(args.src, args.dest, args.tag)))
        else:
            unpack(args.kind, args.artifact, args.dest)
    except (ReleaseError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile, tarfile.TarError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
