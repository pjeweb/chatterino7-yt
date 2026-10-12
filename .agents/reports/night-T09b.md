# T09b prerequisite checkpoint

Branch: `night/T09b-oauth-prerequisites`; assigned checkout:
`/home/pje/.t3/projects/c7yt-night/T09b`.
Base inspected: `73d76167e6d7c8930adaee1892daf664d65a7766`.
Completed 2026-10-12 before 08:00 Europe/Berlin.

## Delivered

- `docs/fork/youtube-auth.md`: 157-line feasibility/implementation prerequisite,
  source findings, narrow fork-local secret-store and OAuth transport boundaries,
  platform packaging inputs, failure/deletion/migration policy and bounded tests.
- Three-line link/status addition to `docs/fork/youtube.md`; original implementation
  slice remains deferred. No C++, dependencies, YAML, workflows or gitlinks changed.
- Read `.agents/NIGHT.md`, `.agents/REVIEW.md`, `.agents/TASKS.md`, relevant fork
  YouTube/network/CI/index/roadmap docs, actual packaging and account/settings/logging
  source. Did not edit shared board/coordinator files or other checkouts.

Key findings: no reusable secure-store abstraction; settings persist Twitch/Kick
credentials and enable backups; generic HTTP logging includes URLs/POST payloads,
with a Debug Qt >= 6.10 body-redaction override. Recommend isolated OAuth transport
with unconditional redaction rather than relying on that flag. Flatpak retains
QtKeychain 0.15.0 and unused historical app/build switches; presence is not linkage.

## Validation commands and results

All commands ran in the assigned checkout. The following source-claim check passed
22 assertions (including the scope of changes). It checks source text, not runtime.

```bash
python3 - <<'PY'
from pathlib import Path
import subprocess
checks = 0
def has(path, *terms):
    global checks
    text = Path(path).read_text()
    for term in terms:
        assert term in text, (path, term)
        checks += 1
has('src/providers/kick/KickAccount.cpp', 'QStringSetting::set', '/clientSecret', '/authToken', '/refreshToken')
has('src/providers/twitch/TwitchAccountManager.cpp', '/oauthToken')
has('src/controllers/accounts/AccountController.cpp', 'this->twitch.load()', 'this->kick.load()')
has('src/singletons/Settings.cpp', '/settings.json', 'setBackupEnabled(true)', 'setBackupSlots(9)')
has('src/singletons/Paths.cpp', 'modes.isPortable', 'QStandardPaths::AppDataLocation')
has('src/common/network/NetworkTask.cpp', 'CHATTERINO_HTTP_ALWAYS_SHOW_REQUEST_BODY', 'QT_VERSION_CHECK(6, 10, 0)', '<< status << payload')
has('src/common/ForkIdentity.hpp', 'com.chatterino7yt.chatterino7yt')
has('.CI/com.chatterino7yt.chatterino7yt.yml', 'libsecret-0.21.4', '0.15.0.tar.gz', 'USE_SYSTEM_QTKEYCHAIN=ON')
for path in ('CMakeLists.txt', 'src/CMakeLists.txt', 'conanfile.py', 'vcpkg.json'):
    assert 'keychain' not in Path(path).read_text().lower()
assert subprocess.run(['rg', '-i', 'QKeychain|SecretStore|qtkeychain', 'src'], capture_output=True).returncode == 1
checks += 1
paths = set(subprocess.check_output(['git', 'diff', 'HEAD', '--name-only'], text=True).splitlines())
assert paths <= {'docs/fork/youtube.md', 'docs/fork/youtube-auth.md', '.agents/reports/night-T09b.md'}
checks += 1
assert not subprocess.check_output(['git', 'diff', 'HEAD', '--raw'], text=True).count('160000')
checks += 1
print(checks, 'source/scope assertions passed; manifest negatives also passed')
PY
```

Link/length check passed: 12 local links, 15 unique remote links HTTP 200,
both remote fragment IDs present, new YouTube index link present, 157 lines.
Exact verification command (public unauthenticated documentation only):

```bash
python3 - <<'PY'
from pathlib import Path
import re, urllib.request, concurrent.futures
p = Path('docs/fork/youtube-auth.md')
links = re.findall(r'\]\(([^)]+)\)', p.read_text())
local = [u for u in links if not u.startswith('https://')]
for u in local:
    assert (p.parent / u).is_file(), u
assert (p.parent / 'youtube-auth.md').is_file()
assert '[secure storage and Desktop OAuth plan](youtube-auth.md)' in Path('docs/fork/youtube.md').read_text()
def check(u):
    with urllib.request.urlopen(u, timeout=15) as r:
        body = r.read().decode('utf-8')
        assert r.status == 200, u
    if '#' in u:
        fragment = u.split('#', 1)[1]
        assert re.search(r'(?:id|name)=[\"\']' + re.escape(fragment) + r'[\"\']', body), u
    return u
remote = list(dict.fromkeys(u for u in links if u.startswith('https://')))
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    assert len(list(pool.map(check, remote))) == len(remote)
assert len(p.read_text().splitlines()) < 160
print(len(local), 'local;', len(remote), 'remote;', len(p.read_text().splitlines()), 'lines: PASS')
PY
```

Formatting/whitespace commands:

```bash
npx --yes prettier@3.8.1 --version
npx --yes prettier@3.8.1 --write docs/fork/youtube-auth.md docs/fork/youtube.md .agents/reports/night-T09b.md
npx --yes prettier@3.8.1 --check docs/fork/youtube-auth.md docs/fork/youtube.md .agents/reports/night-T09b.md
git diff --check
```

Results: Prettier version `3.8.1`; all three Markdown files formatted and passed
check; `git diff --check` passed. No build required or run. No global dependency
installation/upgrade; npx used the exact formatter package, not project dependencies.

## Source references and research limits

Official citations are inline in the delivered plan. QtKeychain source is pinned
at `987522a32ecdf71f48678e4b454d9461899101a9`: ReadMe, Job API, both CMake files
and BSD-3-Clause COPYING. This is inspected development source, not a selected
release/recipe. Apple Keychain, Microsoft CredWrite and freedesktop Secret Service
locking/attribute documentation verify backend interfaces, not fork operation.
Google Desktop OAuth and lifecycle pages verify loopback/PKCE, installed-client
limits, revocation impact and External/Testing expiry. Public unauthenticated
YouTube discovery revision `20261006` was fetched with Python urllib and verified
REST read/write scopes; send/ban method docs corroborate write requirements.
No authenticated API or OAuth request was made. Initial guessed QtKeychain README/
backend paths and Secret Service locking paths failed; final canonical URLs were
resolved and checked. Web tool could not fetch discovery; urllib fetched it
successfully. Apple page content is sparse; backend mapping also uses QtKeychain's
official ReadMe and Apple framework linkage source.

## Deferred behavior and blockers

No OAuth, imported client JSON, key generation, keychain mutation, secret
persistence, signing/release work or runtime configuration changes occurred.
No subagents, launchers, builds, workflow dispatch/enabling, push, PR or messages.

Subsequent implementation requires reviewed pinned QtKeychain acquisition/linkage,
license notices and ABI/deployment proof for Linux packages, macOS universal and
Windows x64/ARM64. Flatpak sandbox routing/browser/loopback and FreeBSD support
remain unproven. Linux container QtKeychain contents were not inspected or assumed.
Fake-store/transport/listener and race/redaction tests are specified, not executed.
Real store smoke tests need separately authorized disposable stores/platforms;
live login needs explicit authorization and user-owned Desktop client/project,
consent/test-user setup and a working browser/loopback. Heavy CI remains paused.
Read-only streaming, Google identity/channel grant sharing, live refresh behavior
and platform prompts remain untested. No live-login/platform success is claimed.

Supervisor owns review, publication and integration. This local checkpoint is
prerequisite documentation only; authentication implementation is not complete.
