"""Tests for check-network-hosts.py. Run: python3 -m unittest discover -s scripts/fork"""

import importlib.util
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location(
    "check_network_hosts", HERE / "check-network-hosts.py"
)
cnh = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(cnh)

NETWORK_MD = """\
| Host | Purpose |
| --- | --- |
| `api.twitch.tv`, `id.twitch.tv` | Helix |
| `7tv.io/v3/users/*/presences` | presences |
| `wss://events.example.org:443/v1` | events |

Crash dumps stay on disk (see `LastRunCrashDialog`). Build with
`scripts/dev/docker-build.sh {app|test}`. Not in backticks: plain.example.com
"""
ALLOWED = cnh.documented_hosts(NETWORK_MD) | cnh.LINK_ONLY


def diff(path, *body, start=10):
    """A git diff for one file with a single hunk."""
    return "\n".join(
        [
            f"diff --git a/{path} b/{path}",
            "index 1111111..2222222 100644",
            f"--- a/{path}",
            f"+++ b/{path}",
            f"@@ -{start},1 +{start},{len(body)} @@",
            *body,
        ]
    )


def missing(diff_text):
    return cnh.undocumented(cnh.added_hosts(diff_text), ALLOWED)


class DocumentedHostsTest(unittest.TestCase):
    def test_backticked_hosts_paths_and_schemes(self):
        self.assertEqual(
            cnh.documented_hosts(NETWORK_MD),
            {"api.twitch.tv", "id.twitch.tv", "7tv.io", "events.example.org"},
        )

    def test_real_network_md_parses(self):
        hosts = cnh.documented_hosts(cnh.NETWORK_MD.read_text(encoding="utf-8"))
        for host in ("irc.chat.twitch.tv", "7tv.io", "www.youtube.com", "api.ivr.fi"):
            self.assertIn(host, hosts)
        self.assertNotIn("scripts", hosts)


class AddedHostsTest(unittest.TestCase):
    def test_documented_host_passes(self):
        d = diff("src/a.cpp", '+    QUrl("https://api.twitch.tv/helix/users");')
        self.assertEqual(cnh.added_hosts(d), [cnh.Hit("api.twitch.tv", "src/a.cpp", 10)])
        self.assertEqual(missing(d), [])

    def test_undocumented_host_fails_with_file_and_line(self):
        d = diff(
            "src/b.cpp",
            "+int x = 1;",
            '+auto u = "https://tracker.example.com/collect";',
            start=40,
        )
        self.assertEqual(missing(d), [cnh.Hit("tracker.example.com", "src/b.cpp", 41)])
        report = cnh.format_report(missing(d))
        self.assertIn("tracker.example.com", report)
        self.assertIn("src/b.cpp:41", report)

    def test_link_only_host_passes(self):
        d = diff("src/c.cpp", '+label("https://console.cloud.google.com/apis");')
        self.assertEqual(missing(d), [])

    def test_removed_and_context_lines_ignored(self):
        d = diff(
            "src/d.cpp",
            '-get("https://removed.example.com");',
            ' get("https://context.example.com");',
            "+int unrelated = 0;",
        )
        self.assertEqual(cnh.added_hosts(d), [])

    def test_subdomains_and_parents_are_not_covered(self):
        d = diff(
            "src/e.cpp",
            '+get("https://evil.api.twitch.tv/x");',
            '+get("https://twitch.tv/");',
        )
        self.assertEqual(
            [h.host for h in missing(d)], ["evil.api.twitch.tv", "twitch.tv"]
        )

    def test_case_port_path_query_and_websocket(self):
        d = diff(
            "src/f.cpp",
            '+get("HTTPS://API.Twitch.TV:443/helix?x=https");',
            '+ws("wss://events.example.org/v1");',
            '+get("https://7tv.io/v3/emote-sets/1.");',
        )
        self.assertEqual(
            [h.host for h in cnh.added_hosts(d)],
            ["api.twitch.tv", "events.example.org", "7tv.io"],
        )
        self.assertEqual(missing(d), [])

    def test_comment_only_lines_skipped(self):
        d = diff(
            "src/g.cpp",
            "+\ufeff// SPDX <https://dev.example.com>",
            '+    // get("https://dev.example.com/x");',
            "+/** See https://dev.example.com/docs */",
            "+ * https://dev.example.com/docs/liveChatMessages",
            "+ */",
        )
        self.assertEqual(cnh.added_hosts(d), [])

    def test_code_on_comment_lines_checked(self):
        d = diff(
            "src/g.cpp",
            '+/* endpoint */ get("https://tracker.example.com");',
            '+* out = "https://tracker.example.com";',
            '+*out = "https://tracker.example.com";',
            '+get("https://cdn.example.net/x"); // https://dev.example.com',
        )
        self.assertEqual(
            [(h.host, h.line) for h in missing(d)],
            [
                ("tracker.example.com", 10),
                ("tracker.example.com", 11),
                ("tracker.example.com", 12),
                ("cdn.example.net", 13),
                ("dev.example.com", 13),
            ],
        )

    def test_is_comment_only(self):
        for line in ("// x", "  /* x", " * x", " */", "/* x */", "*"):
            self.assertTrue(cnh.is_comment_only(line), line)
        for line in ('/* x */ f("u");', '* p = "u";', "/* a */ f(); /* b */", "f(); // x"):
            self.assertFalse(cnh.is_comment_only(line), line)

    def test_loopback_ignored_other_ips_flagged(self):
        d = diff(
            "src/h.cpp",
            '+redirect("http://127.0.0.1:8080/cb");',
            '+redirect("http://localhost:8080/cb");',
            '+get("http://203.0.113.7/x");',
        )
        self.assertEqual([h.host for h in missing(d)], ["203.0.113.7"])

    def test_multiple_files_and_hunks(self):
        d = "\n".join(
            [
                diff("src/one.cpp", '+get("https://one.example.com");', start=5),
                "@@ -50,0 +51,2 @@",
                "+++i;",
                '+get("https://two.example.com");',
                diff("src/two.cpp", '+get("https://three.example.com");', start=1),
            ]
        )
        self.assertEqual(
            cnh.added_hosts(d),
            [
                cnh.Hit("one.example.com", "src/one.cpp", 5),
                cnh.Hit("two.example.com", "src/one.cpp", 52),
                cnh.Hit("three.example.com", "src/two.cpp", 1),
            ],
        )

    def test_new_file_header_is_not_an_added_line(self):
        d = "\n".join(
            [
                "diff --git a/src/new.cpp b/src/new.cpp",
                "new file mode 100644",
                "--- /dev/null",
                "+++ b/src/new.cpp",
                "@@ -0,0 +1 @@",
                '+get("https://api.twitch.tv");',
            ]
        )
        self.assertEqual(cnh.added_hosts(d), [cnh.Hit("api.twitch.tv", "src/new.cpp", 1)])


if __name__ == "__main__":
    unittest.main()
