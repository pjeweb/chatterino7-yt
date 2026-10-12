# T08 IPC coexistence follow-up

Parent checkout: c7yt-night/T08ipc, branch night/T08-ipc-isolation.

The separate browser host from PR7 still used upstream chatterino_gui for client sends, server creation and destruction. Boost1.84 message_queue in the Linux build uses shared_memory_object; its global POSIX name can be replaced/unlinked by another application despite separate profiles. Source starts native messaging on release builds on all platforms.

Use the fork-specific chatterino7yt_gui constant for each transport path and the legacy executable-hash name helper. Preserve upstream queues; no attempt to delete/migrate the old shared name. Add a randomized Linux regression that creates both namespaces, routes a message through the fork queue, removes it, then verifies the upstream queue remains reachable with its queued and subsequent messages. Unique names avoid live application queues; try_receive avoids hangs. Windows/macOS runtime validation is unavailable.

Status: C++ formatting/diff checks pass. Full build/test and independent review are pending. Parent waits for nativeT06 ordinary Debug build to release one of the two C++ slots. Heavy CI remains paused/manual-only; no external services, credentials or billing changes.

Independent review found that the original test bypassed native consumers. The revised regression now starts NativeMessagingServer in an injectable randomized namespace, sends through nm::client, observes receiver handling, destroys the server, and verifies upstream survives with old/new messages. The default constructor/client remain bound to the fork constant. Cooperative interruption wakes the blocking receive before queue cleanup and releases the owned QThread after join; this avoids termination/leaks during the new lifecycle test. Final build/test and rereview pending.

Final validation: committed IPC revisiond40cf8227 passed672/672 BUILD_APP=ON Debug tests with zero failed attempts. Independent rereview resolved the consumer-coverage finding with no actionable source findings. Combined main6dd64ada2 plus IPC passed676/676 fatal ASan/UBSan tests with leak detection and no retries,121.52s,zero sanitizer diagnostics; the native lifecycle regression passed. Logs:/tmp/night-T08-ipc-final-check.log and /tmp/night-T08-ipc-main-asan.log. Merged PR9 at2819cb56a after all enabled CI green. Earlier pending statuses above are historical checkpoints. Windows/macOS runtime remains unverified.
