# Night T12r: embed consent migration correction

Worktree: `/home/pje/.t3/projects/c7yt-night/T12r`.
Branch: `night/T12-embed-consent-review`.
Base: original T12 committed `HEAD` `75fcc1386`.
Scope: narrow independent-review P2 fix only. No push/PR. Supervisor integrates
into the existing T12 proposal.

## P2 finding (Codex independent review)

`Application.cpp` previously guarded the entire
`networkconsent::ensureFirstRunDecision` call with `!isFramelessEmbed`. That
skipped **existing-profile migration** when a prior GUI profile was opened as a
frameless embed, so `networkConsentCompleted` stayed false and the new optional
request gates silently disabled already-enabled message history / presence.

## Fix

Minimal startup / decision-hook change:

- `ensureFirstRunDecision(settings, parent, allowPrompt)` — Application passes
  `allowPrompt = !args.isFramelessEmbed`.
- Existing profiles (`currentVersion` non-empty) migrate and preserve saved
  service toggles **regardless of embed mode**.
- Fresh profiles with `allowPrompt == false` (embed): **no modal**, marker left
  false, **no implicit consent**; optional requests stay gated.
- No duplicated dialog / migration logic; embed never shows the first-run modal.

Docs: `docs/fork/NETWORK.md` first-run section corrected for embed migration vs
fresh-embed gating. Original T12 deferred note that embeds “never complete
consent” was too broad for existing profiles; this report + NETWORK.md are the
correction.

## Tests added

Decision boundary used by `Application::initialize`
(`allowPrompt = !isFramelessEmbed`):

- `ExistingProfileEmbedMigratesPreservingEnabledChoices` — `allowPrompt=false`,
  existing profile with history/presence enabled → marker true, choices kept,
  `allowsOptionalRequests` true.
- `FreshEmbedLeavesConsentIncompleteWithoutPrompt` — `allowPrompt=false`, fresh
  profile → no completion, toggles off, IVR optional request still fails.

## Validation (executed)

| Command                                                                  | Result                                                                           |
| ------------------------------------------------------------------------ | -------------------------------------------------------------------------------- |
| `git submodule update --init --recursive` (this worktree only)           | OK                                                                               |
| `/tmp/clang-format-19 -i` / `--dry-run --Werror` on changed C++ (19.1.0) | OK                                                                               |
| `npx prettier@3.8.1 --write docs/fork/NETWORK.md`                        | OK (unchanged after edit)                                                        |
| `python3 scripts/fork/check-network-hosts.py`                            | OK (`api.ivr.fi` already documented)                                             |
| `python3 -m unittest discover -s scripts/fork -v`                        | **17/17** OK                                                                     |
| `C7YT_JOBS=4 scripts/dev/docker-build.sh check`                          | **693/693** passed (11 NetworkConsent\* tests). Log: `/tmp/night-T12r-check.log` |

`check` builds `chatterino-lib` with tests (`BUILD_APP=OFF`); NetworkConsent
dialog / Application hook compile in that library. Did not start a second
parallel C++ build beyond the one allowed alongside the supervisor T12 review
build slot. Jobs capped at 4.

## Deferred / limitations

- Fresh frameless embeds still have no in-embed UI to complete consent; optional
  requests stay off until a normal GUI launch (or other completion path).
- No dedicated full “Network services” settings page (unchanged from T12).
- Pronouns / link previews remain separate toggles (unchanged).
- Kick private endpoints still have no off switch (pre-existing).
- Interactive Accept/Cancel of the live modal still covered by logic + dialog
  default construction tests, not full GUI automation.

## Source refs

- Startup: `Application::initialize` →
  `networkconsent::ensureFirstRunDecision(..., !isFramelessEmbed)`
- Logic: `src/singletons/NetworkConsent.cpp`
- Tests: `tests/src/NetworkConsent.cpp`
- Docs: `docs/fork/NETWORK.md` (First-run network consent)
