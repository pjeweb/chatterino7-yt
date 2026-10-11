# Incoming-message translation (proposal)

Translation is not implemented. Recommend **on-device first**, starting with
one explicit language pair and a manual “Translate” action. Keep translation
disabled until selected; enabling it does not authorize downloads or remote
translation. Preserve the original message. Never switch providers automatically
when a model is missing, detection is uncertain, or a request fails.

## Options and costs

| Option                           | Chat text leaves the app?                                  | Build/package cost                                                          | Main limit                                                            |
| -------------------------------- | ---------------------------------------------------------- | --------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| Native Bergamot + Firefox models | No, after model installation                               | C++ engine, Marian/SentencePiece/sentence splitter, per-pair files          | Compatibility, CPU, memory and chat-language quality need measurement |
| User-key DeepL only              | Yes, to DeepL                                              | Small HTTP adapter; no model bundle                                         | Account, billed usage, network latency, provider terms                |
| User-hosted LibreTranslate       | To the configured server; loopback can stay on this device | Small HTTP adapter; user maintains Python/container server and Argos models | Deployment, server logging/security, model coverage and CPU           |

[Firefox uses WASM Bergamot/Marian and directional models](https://firefox-source-docs.mozilla.org/toolkit/components/translations/resources/01_overview.html);
a missing direct pair can require two translations through English. That adds
another inference pass and model; latency and CPU cost must be measured.
For Qt, evaluate the [native C++ library](https://github.com/mozilla/bergamot-translator)
rather than embedding a browser/WASM runtime. Its
[CMake configuration](https://github.com/mozilla/bergamot-translator/blob/main/CMakeLists.txt)
defaults to host CPU tuning, sets Windows runtime flags and can fetch dependencies.
Pin engine/submodule revisions, disable build-time fetching, reconcile flags with
the fork, and verify portable x86-64 and macOS ARM64 builds before promising support.

Binary/package growth and resident memory are **unknown until built**. As a
scale example, Mozilla's [production model metadata](https://firefox.settings.services.mozilla.com/v1/buckets/main/collections/translations-models-v2/records)
for `en → de`, version `3.0`, `base-memory`, lists model/lexicon/vocabulary files
totalling 24.5 MB compressed and 36.7 MB unpacked (decimal MB, checked 2026-10-12).
These are file sizes, not RAM usage, engine size, or proof that this native build
accepts those files. [Mozilla distributes its trained models under MPL 2.0](https://github.com/mozilla/translations#readme);
[Bergamot is MPL 2.0](https://github.com/browsermt/bergamot-translator#license).
Record each selected artifact's provenance/license and dependency notices in the
release SBOM; do not infer other model collections' licenses from the engine.

There is no measured **CPU cost per chat message** for this fork. Benchmark cold
load and warm inference separately: 20/100/500-character multilingual messages,
one pair then a pivot pair, sustained/burst traffic, p50/p95 wall time, CPU time
per message, peak RSS and package delta on representative CPUs. Include slang,
emoji and mixed languages in a human quality check. Approve resource/latency
budgets from those measurements before adding automatic translation.

“BYO-key only” is the smallest useful implementation: manual DeepL translation,
no native dependency or model manager. It cannot advertise offline translation
or satisfy the roadmap's on-device goal. It can ship separately as an explicit
optional provider if the native feasibility slice fails; retain the offline goal
as unimplemented rather than silently routing its traffic to a server.

## Model installation and consent

Ship a reviewed manifest with exact pair/direction, engine compatibility, HTTPS
URL, compressed/unpacked size, SHA-256 for both, revision and license. Authenticate
that manifest through the signed release; a hash fetched beside a model is not
an independent trust anchor. A remote catalog would need signature verification
with a pinned key and explicit refresh consent.

Before downloading, show pair(s), publisher/host, bytes and disk requirement,
including pivot dependencies. No startup/catalog prefetch or background refresh.
Download into temporary files with size/time limits; reject unapproved redirects,
verify hashes, bound decompression and atomically install before loading. Cancel
cleans partial files; corruption leaves the old verified version intact. Offer
manual import with the same checks and deletion. Add the exact download hosts to
[NETWORK.md](NETWORK.md) when chosen; this proposal chooses no distribution host.

## Qt hooks, privacy and lifetime

Keep service/backends/model management in `src/providers/translation/`. Use the
existing `WindowManager::channelViewContextMenuRequested` signal (invoked by
[`ChannelView::addContextMenuItems`](../../src/widgets/helper/ChannelView.cpp))
for the manual action; add one settings tab via
[`SettingsDialog::addTabs`](../../src/widgets/dialogs/SettingsDialog.cpp).
Copy plain text from [`Message`](../../src/messages/Message.hpp) into a bounded
request, not a `MessageLayout` reference. Exclude names, badges, channel IDs and
reply/history context; preserve emotes/URLs rather than sending their metadata.
Start with a separate plain-text result popup retaining an original-text toggle;
do not mutate shared immutable messages, reinterpret results as HTML/commands,
or change moderation, search, copy or notification behavior.

Run native inference on a dedicated worker with a bounded queue/cache and capped
text length. Return on the GUI thread with a guarded owner, weak channel/message
identity and generation token. Closing a view, disabling translation, deletion,
channel changes or shutdown cancels queued jobs and discards stale completions.
If the engine cannot interrupt running inference, document that cancellation
suppresses its result; stop/join workers before unloading models. Keep results
in memory only and clear them on disable/channel teardown.

Remote providers require separate consent naming the destination and text sent.
[DeepL](https://developers.deepl.com/api-reference/translate/request-translation)
uses `POST /v2/translate`; its [Free/Pro endpoints and key header](https://developers.deepl.com/docs/getting-started/auth)
are `api-free.deepl.com` / `api.deepl.com` and `DeepL-Auth-Key`. Apply request/response
limits, a user character budget, timeout and bounded rate-limit backoff; show
errors without retrying ambiguous paid POSTs automatically. Never bundle a key.
[LibreTranslate's API](https://docs.libretranslate.com/api/operations/translate/)
uses `POST /translate`, plain-text format and an optional server API key. It is
an [Argos-backed self-hosted service](https://docs.libretranslate.com/), not
embedded Bergamot; require HTTPS except explicit loopback HTTP, and never pick
a public mirror. Server logs, retention, updates and initial model downloads
remain the operator's responsibility. Keep its
[AGPL-3.0 server](https://github.com/LibreTranslate/LibreTranslate/blob/main/LICENSE)
outside the app package; review server/model licensing separately if bundling later.

[`NetworkRequest`](../../src/common/network/NetworkRequest.hpp) offers timeout,
`caller()`, `followRedirects(false)` and `hideRequestBody()`, but no public abort
handle. Lifetime guarding is not transport cancellation. A small cancellable
transport adapter is needed for active requests; cancellation cannot retract text
already sent. Redact bodies/keys/URLs in all builds: the current
[`NetworkTask`](../../src/common/network/NetworkTask.cpp) debug override can expose
hidden request bodies. Do not cache provider requests or log provider error bodies.
No keychain adapter was found; Twitch tokens currently use settings. Add a system
credential-store adapter, or accept keys for this session only; do not copy that
persistence pattern. Register provider hosts in NETWORK.md with the implementation.

## Mergeable slices and acceptance

1. **Feasibility:** isolated native engine + one pinned pair, license inventory,
   platform/build-size and chat benchmarks above; explicit go/no-go report.
2. **Local contract/UI:** fake backend, manual action/popup, bounded queue;
   deterministic teardown/deletion/stale-result tests and original-text checks.
3. **Verified models:** consent/import/install/remove; fixture tests for bad hashes,
   truncation, oversize/decompression limits, redirects and canceled installs.
4. **Local backend:** opted-in inference with installed models; disconnected-network
   acceptance, missing-pair errors, bounded burst behavior and shutdown checks.
5. **Optional HTTP backends:** credential adapter, explicit provider selection,
   documented hosts/budget; local HTTP fixtures for redaction, timeout, abort,
   malformed/oversize responses and quota/rate errors. No real keys in CI.
6. **Rules/pivoting:** explicit per-channel/per-language opt-in after measured
   budgets; local uncertain-language handling, original preservation and proof
   that failure/missing models never initiate a remote request.
