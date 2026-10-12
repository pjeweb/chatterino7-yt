# Offline YouTube resource fixtures

`events.json` records sanitized resource **shapes**, constructed by hand from
Google's official reference checked 2026-10-12 (page updated 2026-09-14):
https://developers.google.com/youtube/v3/live/docs/liveChatMessages
and its linked official streaming schema:
https://developers.google.com/youtube/v3/live/streaming-live-chat

These are synthetic examples, not captured live Google responses. All IDs,
display names, timestamps and text are invented. No credentials, personal data,
URLs or downloaded assets are included. The largest uint64 amount is a deliberate
parser boundary case, not a realistic purchase. No Google API request was made.

Order: text/Unicode and role hints, Super Chat, Super Sticker alt text, new member,
milestone, gifting, gift received, tombstone, temporary ban, permanent ban, poll,
Jewel gift, future type, ended, incomplete text, empty/null/scalar/wrong shapes,
and malformed temporary ban. Poll/Jewel gift use supplied display text and remain
unknown structured types. The current reference/proto omit messageDeletedEvent;
deletions normalize documented tombstones by their resource ID. No invented
messageDeletedDetails contract is used.
