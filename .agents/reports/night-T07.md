# T07 release provenance — night worker (Claude)

Branch `night/T07-release-provenance`. Scope: `fork-release.yml`, its helper
script and tests, `docs/fork/RELEASES.md`, one-line CI/ROADMAP/README updates.
Upstream `build.yml` is **unchanged**. Nothing was dispatched, tagged, released,
pushed, signed or configured. No secrets or keys were added.

**Status: implemented and statically/locally verified. Never run on GitHub.**
It is blocked on a successful Build run for a tag, and `build.yml` is
`disabled_manually` on pjeweb/chatterino7-yt (checked via the API 2026-10-12).

## Design

- `workflow_dispatch` only, on a tag ref, with input `build-run-id`. It
  consumes an existing Build run, so it does not copy or run the matrix.
  Jobs skip unless `github.repository == 'pjeweb/chatterino7-yt'` and
  `github.ref_type == 'tag'`.
- `collect` (`actions: read`, `contents: read`): checks the run via
  `GET /actions/runs/{id}` (path `build.yml`, completed/success, `head_sha ==
github.sha`, `head_branch == tag`, event push/dispatch, same repository and
  head repository). Then it downloads that run's artifacts and maps them
  through a fixed allowlist. Missing, duplicate, unknown, nested and symlinked
  files fail. It renames the files, writes `SHA256SUMS`, unpacks each scannable
  artifact and runs Syft 1.54.1 on it. Syft comes from
  `anchore/sbom-action/download-syft`, whose installer checks the release
  checksum.
- `attest-provenance` and the `attest-sbom` matrix (`id-token`,
  `attestations` write only) re-check `SHA256SUMS` and run
  `actions/attest@v4.2.2`. Provenance covers every asset, including
  `SHA256SUMS`. Each scanned asset gets an SPDX 2.3 SBOM attestation.
  `artifact-metadata: write` is omitted: the source only uses it for
  registry storage records (`push-to-registry`).
- `draft-release` (`contents: write` only): re-checks the files and confirms
  the tag still resolves to `github.sha`. It refuses if a release already
  exists, then runs `gh release create --draft --verify-tag`.
- All actions are pinned to SHAs resolved from the release tags:
  attest v4.2.2 `1e69f48…`, sbom-action v0.24.3 `66cbf4b…`,
  download-artifact v8.0.2 `9000827…`, upload-artifact v7.0.2 `cf430e0…`,
  checkout v7.0.1 `3d3c42e…`. The top level is `permissions: {}`, and checkout
  uses `persist-credentials: false`.

## Docs checked (2026-10-12)

- READMEs and `action.yml` files at the pinned tags: `actions/attest`;
  `attest-build-provenance` (v4 is now a wrapper around `actions/attest`);
  `anchore/sbom-action` (pinned Syft install with checksum check;
  `--source-name`/`--source-version`); `download-artifact` (`run-id` plus
  `github-token`; digest mismatch errors by default); and the
  `flatpak-github-actions` artifact naming (file `chatterino.flatpak`).
- `actions/attest` `src/sbom.ts`: SPDX predicate type is
  `https://spdx.dev/Document/v<spdxVersion>`, giving `v2.3` for Syft output.
- Ubuntu 24.04 runner image README: Python 3.12.3, dpkg, p7zip-full. On noble
  that package depends on `7zip` 23.01, which provides `/usr/bin/7z`, per
  packages.ubuntu.com. `ubuntu-latest` moves to 26.04 in Nov 2026, so the jobs
  pin `ubuntu-24.04`.
- `gh attestation verify --help` (gh 2.102.0) for `--signer-workflow`,
  `--source-ref`, `--predicate-type` and the JSON output shape.

## SBOMs measured on real binaries

I ran Syft 1.54.1 locally on SevenTV `nightly-build` assets. They come from the
same upstream build steps and were renamed to the fork's `build.yml` file
names. All SBOMs are SPDX-2.3. Each names its asset and records the asset's
SHA-256, which matched `SHA256SUMS` in every case:

| Asset        | Unpack                     | Packages found                                          | Files |
| ------------ | -------------------------- | ------------------------------------------------------- | ----- |
| Windows zip  | Python `zipfile`, checked  | 35 (Qt6 6.12.0, OpenSSL 3.6.1, MSVC CRT, DXC, crashpad) | 46    |
| macOS dmg    | 7-Zip 23.01                | app bundle "Chatterino 7TV" 2.5.6                       | 134   |
| .deb         | dpkg-deb + tar data filter | Debian package (name/version/purl)                      | 13    |
| AppImage     | 7-Zip 23.01 (squashfs)     | none; file inventory only                               | 72    |
| flatpak, pdb | not unpacked               | no SBOM produced; documented                            | —     |

Scanning the files directly with `syft file:` found nothing for the AppImage,
DMG and Flatpak, which is why the artifacts are unpacked first. These SBOMs
describe **packaged contents only**. They are not source or dependency SBOMs.
Statically linked Qt on Linux, Conan dependencies and `lib/` submodules are
invisible, and `RELEASES.md` says so.

## Verification run

- `python3 -m unittest discover -s scripts/fork -p 'test_*.py'`: 35 OK. That is
  17 network-gate tests plus 18 new ones covering run checks, allowlist
  fail-closed cases, tag validation, the Qt-version tolerance, checksums and
  their format, zip/tar traversal and symlink rejection, and a real `.deb`
  round trip. They also run the workflow's own shell check against tampered,
  extra and missing files, and assert the workflow contains that check twice.
  The existing `fork-network-hosts.yml` job will run them. Local Python is
  3.14.4; the runner's 3.12 was not tested.
- `zizmor 1.30.1` (online, default and `--persona=pedantic`) on
  `fork-release.yml`: no findings. On all `fork-*.yml`: no findings (1 suppressed,
  pre-existing). `actionlint 1.7.12` with shellcheck: clean.
- Local rehearsal ran the workflow's actual `run:` scripts, extracted from the
  YAML, through `bash -eo pipefail`. Steps covered: select, SBOM generation
  (7 SBOMs), and the attest and release checksum checks, including a tamper
  case and an extra-file case. `check-run` rejected a real API run JSON
  (fork-codeql push), naming `path` and `head_branch`.
- I checked that the documented `gh attestation verify` commands parse; they
  return 404 since no attestations exist. The same command form verified a
  real attested `cli/cli` v2.102.0 asset.
- Prettier 3.8.1 `--check` on changed Markdown and YAML, plus
  `git diff --check`: clean.

## Not tested / limitations

- **No live run.** Sigstore signing, the attestation upload, cross-run
  artifact download with `GITHUB_TOKEN` + `actions: read`, `gh release
create`, and the runner's distro 7z on fork-built binaries are all
  untested. The fork binaries themselves (renamed `chatterino7-yt` bundles,
  real ARM64 zip, 22.04 and 26.04 debs) were never available; stand-ins were
  used.
- `head_branch == tag` for a `workflow_dispatch` on a tag ref is inferred
  from tag-push runs (goreleaser shows `v2.18.3`). If GitHub reports it
  differently, the check fails closed and needs adjusting.
- **Provenance gap.** The attestation signer is `fork-release.yml` on the
  tag, not the Build job that compiled the bytes. The Build run ID appears
  only in the release notes and log. Attesting inside `build.yml` would need
  an upstream-file change plus `id-token` in the build jobs. I left that for
  T13 and did not implement it.
- To unblock: enable Build (T13), merge this, tag, dispatch Build on the tag,
  then dispatch fork-release with the run ID. A tag created before this
  workflow landed can't use it.
- Windows and macOS binaries are unsigned (macOS ad-hoc). No SBOM for the
  Flatpak or the symbols.
