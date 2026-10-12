# Releases

> **Not run yet.** `fork-release.yml` has never run, and no release exists. It
> needs a successful Build run for the tag, and Build is paused on this fork.

## Verify a download

Install the [GitHub CLI](https://cli.github.com/). Then, for a tag such as
`v7.5.6-yt.1`:

```sh
TAG=v7.5.6-yt.1
REPO=pjeweb/chatterino7-yt
WF=$REPO/.github/workflows/fork-release.yml

# Provenance: these bytes were published by fork-release.yml running on the tag.
gh attestation verify chatterino7-yt-$TAG-x86_64.AppImage --repo $REPO \
  --signer-workflow $WF --source-ref refs/tags/$TAG --deny-self-hosted-runners

# Checksums: SHA256SUMS is attested too; then check the files you downloaded.
gh attestation verify SHA256SUMS --repo $REPO \
  --signer-workflow $WF --source-ref refs/tags/$TAG
sha256sum --check --ignore-missing SHA256SUMS

# SBOM: the attested SPDX document for one artifact.
gh attestation verify chatterino7-yt-$TAG-x86_64.AppImage --repo $REPO \
  --signer-workflow $WF --predicate-type https://spdx.dev/Document/v2.3 \
  --format json --jq '.[0].verificationResult.statement.predicate.name'
```

On macOS use `shasum -a 256 -c`; on Windows compare
`Get-FileHash -Algorithm SHA256 <file>` with the line in `SHA256SUMS`.

## What the release contains

| Asset                               | Provenance + SHA256SUMS | SBOM (`<asset>.spdx.json`)                    |
| ----------------------------------- | ----------------------- | --------------------------------------------- |
| Windows x86_64 / ARM64 `.zip`       | yes                     | files + PE version info (Qt, OpenSSL, CRT)    |
| macOS universal `.dmg`              | yes                     | files + app bundle name/version               |
| Ubuntu 22.04 / 24.04 / 26.04 `.deb` | yes                     | files + Debian package metadata               |
| Linux `.AppImage`                   | yes                     | files only (bundled `.so` names, no versions) |
| Linux `.flatpak`                    | yes                     | none: the OSTree bundle isn't unpacked        |
| Windows symbols `.pdb.7z`           | yes                     | none                                          |
| `SHA256SUMS`                        | provenance              | n/a                                           |

## How it works

1. Push a `v*` tag, then `gh workflow run build.yml --ref $TAG`.
2. When it succeeds: `gh workflow run fork-release.yml --ref $TAG -f build-run-id=<id>`.
3. `fork-release.yml` refuses the run unless it is this repository's
   `build.yml`, succeeded, was a push or dispatch on `$TAG`, and built the
   tag's commit. It requires exactly the expected files (unknown, missing or
   duplicate files fail), renames them, writes `SHA256SUMS`, unpacks each and
   scans it with Syft, attests provenance for every asset and an SBOM per
   scanned asset, then creates a **draft** release. A maintainer publishes it.

Jobs are split so only the attest jobs get `id-token: write` and only the
release job gets `contents: write`.

## Limitations

- **Two runs, one signature.** Build compiles; fork-release attests. The
  provenance names fork-release's run and the tag's commit, not the Build run.
  The Build run ID is in the release notes and the fork-release log.
  Attesting inside `build.yml` jobs would remove this gap (T13).
- **SBOMs list what's packaged, not how it was built.** Syft can't see
  statically linked code. Linux builds link Qt statically, and Conan
  dependencies and `lib/` submodules aren't listed. A source/dependency SBOM
  (for example Conan's) would be separate work and must be labelled that way.
- Binaries are unsigned (macOS is ad-hoc signed, not notarized).
- Build artifacts expire (90 days by default); release before then.
- The release workflow must exist at the tagged commit. A tag made before it
  landed can't use it.
