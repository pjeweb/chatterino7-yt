# YouTube secure storage and Desktop OAuth prerequisite

T09b is a documentation-only plan, verified against source and official docs on
2026-10-12. T09a offline events/builders are merged; authentication is absent.
No dependency, backend runtime, listener or live login was tested here.

## Source findings and smallest boundary

- [`AccountController.cpp`](../../src/controllers/accounts/AccountController.cpp)
  aggregates Twitch/Kick insert/remove signals and loads both managers. Add a
  YouTube manager and provider dispatch only in a subsequent implementation.
- [`KickAccount.cpp`](../../src/providers/kick/KickAccount.cpp) saves client secret,
  access and refresh tokens with `QStringSetting`; Twitch's
  [`TwitchAccountManager.cpp`](../../src/providers/twitch/TwitchAccountManager.cpp)
  reads `/accounts/*/oauthToken`. Do not copy either persistence path.
- [`Settings.cpp`](../../src/singletons/Settings.cpp) uses `settings.json`, enables
  backups and nine backup slots. [`Paths.cpp`](../../src/singletons/Paths.cpp)
  supports portable and standard app-data roots; neither is secret storage.
- [`NetworkPrivate.cpp`](../../src/common/network/NetworkPrivate.cpp) logs URLs;
  [`NetworkTask.cpp`](../../src/common/network/NetworkTask.cpp) logs POST bodies.
  Its `hideRequestBody` can be overridden in Debug with Qt >= 6.10 by
  `CHATTERINO_HTTP_ALWAYS_SHOW_REQUEST_BODY`. It is insufficient for OAuth.

Recommend new fork-local `YouTubeSecretStore` interface under
`src/providers/youtube/`: asynchronous `read/write/remove(key)` and typed results
(success, missing, denied/locked, unavailable, failure), injected into the manager.
One QtKeychain implementation and an in-memory fake suffice; no upstream
credential refactor. Availability is advisory; every job can fail.
Use QtKeychain jobs with `setInsecureFallback(false)` explicitly, handle completion
and ownership, and never surface raw backend error strings containing identifiers.
[Job API](https://github.com/frankosterfeld/qtkeychain/blob/987522a32ecdf71f48678e4b454d9461899101a9/qtkeychain/keychain.h).

## Backends and packaging prerequisites

QtKeychain documents these backends; this is library capability, **not verified
fork support**. [Backend documentation](https://github.com/frankosterfeld/qtkeychain/blob/987522a32ecdf71f48678e4b454d9461899101a9/ReadMe.md).

| Target                                  | Candidate backend                                             | Checkout evidence and required proof                                                                                                                                                                                                                                                                                                                     |
| --------------------------------------- | ------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Linux native/AppImage/deb               | GNOME Keyring/Secret Service via libsecret; KWallet via D-Bus | `build.yml` uses Ubuntu 22.04/24.04/26.04 containers; Arch tests install `qtkeychain-qt6`. Container contents unverified. Prove Qt DBus/libsecret development inputs, runtime libraries and desktop service availability; audit `.CI/CreateAppImage.sh` bundling and `.CI/CreateUbuntuDeb.sh` dependency lists.                                          |
| Linux Flatpak                           | Secret Service/libsecret, conditional on sandbox access       | `.CI/com.chatterino7yt.chatterino7yt.yml` builds libsecret 0.21.4 and QtKeychain 0.15.0, passes stale `BUILD_WITH_QT6`/`USE_SYSTEM_QTKEYCHAIN` options; current app CMake does not consume them. No Secret Service/KWallet talk permission is declared. Prove an explicit, reviewed sandbox route and browser/loopback behavior before claiming support. |
| macOS universal, deployment target 13.0 | macOS Keychain                                                | `build.yml` uses Conan, `arm64;x86_64`, and `.CI/MacDeploy.sh`/macdeployqt; macOS tests use Homebrew deps. Add matching QtKeychain slices and deployment proof; verify Keychain access/prompt behavior for packaged app identity.                                                                                                                        |
| Windows x64/ARM64                       | Windows Credential Store                                      | Build/tests use Conan plus installed Qt; `vcpkg.json` is an alternative manifest. Prove matching ABI/architecture, DLL deployment with existing Conan copy/install and windeployqt paths; keep `USE_CREDENTIAL_STORE=ON`.                                                                                                                                |
| FreeBSD                                 | Unix backend candidate only                                   | Paused `build-freebsd.yml` installs `qtkeychain-qt6`; no integration/runtime proof. Defer OAuth support rather than extending this task.                                                                                                                                                                                                                 |

[`CMakeLists.txt`](../../CMakeLists.txt) defaults to Qt6, warns Qt5 unsupported,
and finds no QtKeychain. Neither [`conanfile.py`](../../conanfile.py) nor
[`vcpkg.json`](../../vcpkg.json) declares it. Existing package installation does
not establish linkage. Select and pin a reviewed release/recipe separately;
do not upgrade the vcpkg baseline or global dependencies implicitly.
Upstream inspected at `987522a32ecdf71f48678e4b454d9461899101a9` is development
source, not a chosen release: Qt6 Core, Unix Qt6 DBus and optional libsecret build
inputs; translations need LinguistTools unless disabled; Apple uses Foundation/
Security. [Build definitions](https://github.com/frankosterfeld/qtkeychain/blob/987522a32ecdf71f48678e4b454d9461899101a9/CMakeLists.txt),
[backend linkage](https://github.com/frankosterfeld/qtkeychain/blob/987522a32ecdf71f48678e4b454d9461899101a9/qtkeychain/CMakeLists.txt).
QtKeychain is BSD-3-Clause: retain notices/disclaimer in source and distributed materials; inventory transitive library licenses in packaging.
[License](https://github.com/frankosterfeld/qtkeychain/blob/987522a32ecdf71f48678e4b454d9461899101a9/COPYING).

Windows QtKeychain <= 0.15.0 omitted service names: select a release with the
fix, disable compatibility naming/read fallback, and include fork ID in keys.
Do not use its DPAPI/QSettings alternative. OS guarantees and failures differ:
[Windows logon-session credential store](https://learn.microsoft.com/en-us/windows/win32/api/wincred/nf-wincred-credwritew),
[Apple Keychain services](https://developer.apple.com/documentation/security/keychain-services),
[Secret Service locking](https://specifications.freedesktop.org/secret-service/latest/unlocking.html).

## Storage, failure and deletion policy (proposed)

Use service `com.chatterino7yt.chatterino7yt.youtube.v1`, derived from the fork's [`ForkIdentity.hpp`](../../src/common/ForkIdentity.hpp) desktop ID across OSes.
Keys include that ID plus version, kind and a deterministic length-delimited
encoding/digest of client ID and stable selected YouTube channel identity.
Client secrets use client ID plus explicit client-level identity marker; refresh
records use client ID plus channel ID. Display names/emails are not keys.
Validate multi-channel grant sharing before supporting it; no extra identity scope.
Store labels/keys contain no secrets; lookup attributes may be unencrypted. [Secret Service attributes](https://specifications.freedesktop.org/secret-service/latest/lookup-attributes.html).

A future local import accepts only bounded Desktop `installed` JSON, validates
client ID/optional secret, and ignores supplied endpoint URLs in favor of fixed
Google endpoints. Keep the optional imported client secret in the store or memory;
it is not proof of desktop confidentiality. Never copy JSON/file path/content into
settings, logs, exports or backups; the original file remains user's responsibility.
Save refresh tokens only after identity resolution. Access tokens,
codes, PKCE verifiers and state remain memory-only, including in portable mode.
Settings may hold client/identity IDs, scope mode and deletion-pending metadata.

If locked, denied, absent service or write failure: explain persistence failed,
offer explicit session-only login or cancel/retry; no plaintext/encrypted-file
fallback, automatic unlock loops or later background persistence. Session-only keeps needed secrets in memory until sign-out/exit and requires login next launch.
Missing entry differs from locked store; never overwrite an inaccessible entry.

Sign-out invalidates the account generation, cancels listeners/refresh/readers,
clears memory, and deletes that account's refresh record. Remove imported client
secret on explicit client removal after its last account is removed. Confirm job
completion; deletion failure remains visible and leaves a non-secret pending-delete
marker blocking reload until deletion is retried. Never claim deletion succeeded.
No automatic Twitch/Kick/upstream credential migration or keychain enumeration. No YouTube plaintext migration is needed today. Future schema migration must be
explicit, limited to known fork keys, read/write/verify before deleting old entries;
failed writes retain the old entry. Do not promise erasure of historical backups.

## Desktop OAuth contract and offline tests (proposed)

Google supports system browser + Desktop loopback and PKCE; installed clients cannot keep secrets and do not support incremental authorization.
[Desktop guide](https://developers.google.com/identity/protocols/oauth2/native-app).
Use existing Qt Network/Core primitives; no Qt NetworkAuth dependency is required
by this proposal. Keep token/revoke traffic in a tiny OAuth-specific QNetworkAccessManager
transport with no cache, fixed endpoints, redirect rejection and unconditional
redaction; never feed auth URLs/bodies through the generic HTTP debug logger.

Bind `127.0.0.1:0` before opening browser, use the actual port and exact root `/`
callback URI, fresh unpredictable state and PKCE verifier (43–128 chars), `S256`.
Installed flow returns refresh tokens for offline use; do not force repeated
consent. Exchange once with the same redirect URI/verifier and imported secret
when supplied. Use a bounded listener (proposed 3 minutes, 8 KiB headers), GET only,
exact Host/path, unique query fields, validated state even on denial; return a
static page without echoing parameters. Consume state before exchange, close on
success/denial/cancel/timeout, and reject late completions by attempt generation.

Offline acceptance tests use synthetic strings, injected browser/transport/clock:
valid callback causes one exchange; wrong/missing state, replay/duplicate callback, wrong path/Host/method, duplicate code/state, malformed/oversized request cause none;
wrong requests do not consume a valid attempt. Timeout, cancel, bind/browser failure
close resources; late callback/exchange cannot restore a cancelled account. Verify S256 vectors, URI/port/verifier consistency, denial and partial-scope outcomes.

Serialize refresh per client/identity: concurrent callers share one in-flight
request, use expiry with a small skew; preserve old refresh token when omitted,
accept only valid nonempty replacement, serialize store updates with refresh.
Store failure becomes visible session-only state. `invalid_grant` stops retries
and requires reauthorization; transient errors use bounded backoff. Test omitted/
rotated/malformed tokens, expiry, concurrency, failures and sign-out during write;
a late write must be drained/deleted before sign-out deletion is reported complete.

Read mode: `youtube.readonly`; send/moderate: `youtube.force-ssl` (includes reads).
[Official discovery](https://www.googleapis.com/discovery/v1/apis/youtube/v3/rest)
revision `20261006` verifies REST scopes; [send](https://developers.google.com/youtube/v3/live/docs/liveChatMessages/insert)
and [ban](https://developers.google.com/youtube/v3/live/docs/liveChatBans/insert)
also document the write scope. It permits substantial non-chat access; explain
before consent. Check granted scopes; reauthorize on mode change. Roles remain
server-enforced; read-only streaming is unproven and belongs to T09d.

Use bearer headers, never query tokens; redact Authorization, imported JSON,
OAuth request/response bodies, callback/auth URLs, codes, state, verifier and
backend errors from diagnostics/UI/chat logs. Tests capture logs with synthetic
sentinels, including the debug body-override environment variable and error paths.
Separate opt-in revoke from local sign-out: send token in POST form body to the
fixed revoke endpoint before discarding it, then clear local state even on failure;
report failure and offer Google's account access page. Google's revocation can
affect all clients/scopes in a project, so disclose impact before action.
[Revocation contract](https://developers.google.com/identity/protocols/oauth2/native-app#tokenrevoke).

## Bounded next implementation gate

Deliver only adapter/dependency packaging, OAuth/account lifecycle and tests above;
no discovery, streaming, sending or moderation implementation. Require fake-store
failure/isolation/deletion tests, zero secret settings/backups/logs, then separately
authorized disposable-store smoke tests and packaging checks on Linux/macOS/Windows.
Heavy CI remains paused ([CI policy](CI.md)); no platforms are runtime-proven here. Live login awaits explicit authorization, a user's Desktop client/project with
YouTube API enabled, consent/test-user setup, working browser/loopback and store.
External/Testing YouTube refresh tokens expire after seven days; verification, admin policy and token revocation/expiry can block login or later refresh.
[Lifecycle limitations](https://developers.google.com/identity/protocols/oauth2#expiration).
