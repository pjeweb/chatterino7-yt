# T08 IPC coexistence follow-up

Parent checkout: c7yt-night/T08ipc, branch night/T08-ipc-isolation.

The separate browser host from PR7 still used upstream chatterino_gui for client sends, server creation and destruction. Boost1.84 message_queue in the Linux build uses shared_memory_object; its global POSIX name can be replaced/unlinked by another application despite separate profiles. Source starts native messaging on release builds on all platforms.

Use the fork-specific chatterino7yt_gui constant for each transport path and the legacy executable-hash name helper. Preserve upstream queues; no attempt to delete/migrate the old shared name. Add a randomized Linux regression that creates both namespaces, routes a message through the fork queue, removes it, then verifies the upstream queue remains reachable with its queued and subsequent messages. Unique names avoid live application queues; try_receive avoids hangs. Windows/macOS runtime validation is unavailable.

Status: C++ formatting/diff checks pass. Full build/test and independent review are pending. Parent waits for nativeT06 ordinary Debug build to release one of the two C++ slots. Heavy CI remains paused/manual-only; no external services, credentials or billing changes.
