# T10 translation design — night worker

Branch: `night/T10-translation`. Scope: design only, `docs/fork/translation.md`.
Read `.agents/TASKS.md` T10, `.agents/REVIEW.md`, `ROADMAP.md`, and `NETWORK.md`.
No shared board, C++ implementation, dependency/build, workflow, billing or reset
changes. No network fallback or automatic model download is proposed.

Recommend a measured native Bergamot feasibility slice, then manual local
translation. BYO-key DeepL only is explicitly the low-code alternative without
offline support; self-hosted LibreTranslate stays an optional HTTP service.
Shipping any provider remains conditional on its acceptance slices.

## Upstream hooks verified

- `ChannelView.cpp` invokes `WindowManager::channelViewContextMenuRequested`;
  `WindowManager.hpp` exposes it outside plugin compilation guards.
- `Message.hpp` uses `shared_ptr<const Message>`; its layout expectations require
  immutability. The design starts with a result popup, not message replacement.
- `SettingsDialog::addTabs` is the single tab insertion point.
- `NetworkRequest.hpp` exposes lifetime guarding, timeout, redirect control and
  body hiding, but not transport abort; `NetworkTask::timeout` internally aborts.
- `NetworkTask::logReply` permits `CHATTERINO_HTTP_ALWAYS_SHOW_REQUEST_BODY` to
  override body redaction in Qt 6.10+ debug builds. Release acceptance must cover
  redaction in every build rather than claim the current wrapper guarantees it.
- Searches of `src`, `cmake`, `lib` and top-level CMake found no keychain adapter.
  `TwitchAccountManager.cpp` reads tokens from pajlada settings; design requires a
  new credential adapter or session-only keys instead of reusing that pattern.

## Primary sources checked 2026-10-12

- [Firefox overview](https://firefox-source-docs.mozilla.org/toolkit/components/translations/resources/01_overview.html):
  WASM Bergamot/Marian, directional pairs, English pivot. Firefox behavior is not
  taken as proof of native Qt platform compatibility.
- [Mozilla engine](https://github.com/mozilla/bergamot-translator), its
  [CMake](https://github.com/mozilla/bergamot-translator/blob/main/CMakeLists.txt)
  and `.gitmodules`: native build/API, dependency fetching, host CPU and MSVC flags.
- [Mozilla models README](https://github.com/mozilla/translations#readme): model
  MPL 2.0 and compatibility claim; engine/model revision compatibility still needs
  an actual pinned build. No engine/package size or CPU timing was invented.
- [Production metadata](https://firefox.settings.services.mozilla.com/v1/buckets/main/collections/translations-models-v2/records):
  fetched directly via Python urllib (web tool could not render this JSON; model
  dashboard also could not render its dynamic table). Filtered `sourceLanguage=en`,
  `targetLanguage=de`: three `base-memory`, version `3.0` records. Compressed bytes:
  model 22,461,496 + lex 1,696,246 + vocab 352,776 = 24,510,518; decompressed bytes:
  31,561,787 + 4,347,672 + 810,073 = 36,719,532. Metadata contains compressed and
  decompressed hashes/sizes. No model payload was downloaded or executed.
- [DeepL auth](https://developers.deepl.com/docs/getting-started/auth) and
  [translate](https://developers.deepl.com/api-reference/translate/request-translation):
  Free/Pro hosts, user key in authorization header, JSON translation endpoint.
- [LibreTranslate API](https://docs.libretranslate.com/api/operations/translate/),
  [overview](https://docs.libretranslate.com/), and
  [license](https://github.com/LibreTranslate/LibreTranslate/blob/main/LICENSE):
  Argos-backed service, plain text/optional server key, separate AGPL server.

## Verification

- Prettier `3.8.1` write/check and `git diff --check` pass.
- Source claims cross-checked against actual upstream files and official docs.
- No runtime tests/build required: only Markdown added; acceptance tests are
  future implementation gates, not claimed as executed here.
