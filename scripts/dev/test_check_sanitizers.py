"""Regression checks for the fail-closed sanitizer compile-command gate."""

import contextlib
import io
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location(
    "check_sanitizers", Path(__file__).with_name("check-sanitizers.py")
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)
check, TARGETS = MODULE.check, MODULE.TARGETS


class CheckSanitizers(unittest.TestCase):
    def entries(self):
        return [
            {
                "file": f"/src/{target}.cpp",
                "output": f"CMakeFiles/{target}.dir/source.cpp.o",
                "arguments": [
                    "c++",
                    "-fsanitize=address,undefined",
                    "-o",
                    f"CMakeFiles/{target}.dir/source.cpp.o",
                    f"/src/{target}.cpp",
                ],
            }
            for target in TARGETS
        ]

    def run_check(self, entries):
        with tempfile.TemporaryDirectory() as directory:
            database = Path(directory) / "compile_commands.json"
            database.write_text(json.dumps(entries))
            output = io.StringIO()
            with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
                return check(database), output.getvalue()

    def test_combined_flags(self):
        self.assertTrue(self.run_check(self.entries())[0])

    def test_missing_address_or_undefined_fails(self):
        for sanitizer in ("address", "undefined"):
            with self.subTest(sanitizer=sanitizer):
                entries = self.entries()
                entries[0]["arguments"][1] = f"-fsanitize={sanitizer}"
                passed, output = self.run_check(entries)
                self.assertFalse(passed)
                self.assertIn("missing ASan/UBSan", output)

    def test_missing_target_fails(self):
        passed, output = self.run_check(self.entries()[:-1])
        self.assertFalse(passed)
        self.assertIn(f"{TARGETS[-1]}: no compile commands found", output)

    def test_later_disable_flags_fail(self):
        for disabled in ("address", "undefined", "all"):
            with self.subTest(disabled=disabled):
                entries = self.entries()
                entries[0]["arguments"].append(f"-fno-sanitize={disabled}")
                self.assertFalse(self.run_check(entries)[0])

    def test_shell_commands_and_pch_do_not_confuse_target_detection(self):
        entries = self.entries()
        entries[-1].pop("output")
        arguments = entries[-1].pop("arguments")
        entries[-1]["command"] = (
            "c++ -include CMakeFiles/chatterino-lib.dir/cmake_pch.hxx "
            + " ".join(arguments[1:])
        )
        passed, output = self.run_check(entries)
        self.assertTrue(passed)
        self.assertIn(f"{TARGETS[-1]} (1 commands)", output)


if __name__ == "__main__":
    unittest.main()
