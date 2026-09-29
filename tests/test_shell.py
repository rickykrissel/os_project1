"""Integration tests for the C shell; uses only Python's standard library."""

import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
import unittest


SHELL = Path(os.environ.get("SHELL_UNDER_TEST", "bin/shell")).resolve()
PROMPT = re.compile(r"shelltest@shelltest:[^>\n]*>")


class ShellTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="shell-test-")
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name).resolve()
        self.home = self.directory / "home"
        self.home.mkdir()
        self.env = dict(os.environ, USER="shelltest", MACHINE="shelltest",
                        HOME=str(self.home), PATH="/usr/bin:/bin")

    def run_shell(self, commands, *, auto_exit=True):
        if auto_exit:
            commands += "\nexit\n"
        result = subprocess.run([str(SHELL)], input=commands, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                cwd=self.directory, env=self.env, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        output = PROMPT.sub("", result.stdout)
        body, separator, history = output.partition("Last valid commands:\n")
        return body, history.splitlines() if separator else [], result.stderr

    def test_one_pipe_without_spaces(self):
        body, _, error = self.run_shell("echo alphabet|tr a-z A-Z")
        self.assertEqual(body, "ALPHABET\n")
        self.assertEqual(error, "")

    def test_two_pipes_larger_than_pipe_buffer(self):
        body, _, error = self.run_shell("head -c 2097152 /dev/zero | cat | wc -c")
        self.assertEqual(int(body.strip()), 2097152)
        self.assertEqual(error, "")

    def test_early_reader_exit_closes_pipe_ends(self):
        body, _, error = self.run_shell("yes | head -n 1 | wc -l")
        self.assertEqual(int(body.strip()), 1)
        self.assertEqual(error, "")

    def test_foreground_waits_for_every_stage(self):
        start = time.monotonic()
        _, _, error = self.run_shell("sleep 0.3 | true")
        self.assertGreaterEqual(time.monotonic() - start, 0.25)
        self.assertEqual(error, "")

    def test_invalid_pipelines_do_not_execute_or_enter_history(self):
        commands = ["| cat", "echo bad |", "echo bad || cat", "&",
                    "echo bad & echo worse", "echo bad | cat | cat | cat",
                    "echo bad | > output", "echo bad | missing_shell_command"]
        for command in commands:
            with self.subTest(command=command):
                body, history, error = self.run_shell(command)
                self.assertEqual(body, "No valid commands.\n")
                self.assertEqual(history, [])
                self.assertTrue(error)
        self.assertFalse((self.directory / "output").exists())

    def test_background_returns_to_prompt_and_exit_waits(self):
        start = time.monotonic()
        body, _, error = self.run_shell("sleep 0.7 &\necho responsive\njobs")
        self.assertGreaterEqual(time.monotonic() - start, 0.6)
        pid = re.search(r"\[1\] (\d+)\n", body).group(1)
        self.assertIn(f"[1]+ {pid} sleep 0.7\n", body)
        self.assertLess(body.index("responsive"), body.index("[1] + done"))
        self.assertEqual(body.count("[1] + done sleep 0.7"), 1)
        self.assertEqual(error, "")

    def test_background_pipeline_tracks_earlier_stage(self):
        body, _, error = self.run_shell("sleep 0.7 | true &\nsleep 0.1\njobs")
        self.assertRegex(body, r"\[1\]\+ \d+ sleep 0\.7 \| true\n")
        self.assertEqual(body.count("[1] + done sleep 0.7 | true"), 1)
        self.assertEqual(error, "")

    def test_background_pipeline_reports_last_pid(self):
        # The external script records its own PID, which execv preserves.
        stage = self.directory / "stage"
        stage.write_text("#!/bin/sh\necho $$ > last-pid\nsleep 0.3\n")
        stage.chmod(0o700)
        body, _, error = self.run_shell("echo data | cat | ./stage &\njobs")
        pid = (self.directory / "last-pid").read_text().strip()
        self.assertIn(f"[1] {pid}\n", body)
        self.assertIn(f"[1]+ {pid} echo data | cat | ./stage\n", body)
        self.assertEqual(error, "")

    def test_background_numbers_are_not_reused(self):
        body, _, error = self.run_shell("true &\nsleep 0.1\ntrue &\nsleep 0.1\njobs")
        self.assertRegex(body, r"\[1\] \d+\n")
        self.assertRegex(body, r"\[2\] \d+\n")
        self.assertEqual(body.count("+ done true"), 2)
        self.assertIn("No active background processes.", body)
        self.assertEqual(error, "")

    def test_ten_concurrent_jobs(self):
        body, _, error = self.run_shell("\n".join(["sleep 0.7 &"] * 10 + ["jobs"]))
        for number in range(1, 11):
            self.assertRegex(body, rf"\[{number}\]\+ \d+ sleep 0\.7\n")
            self.assertEqual(body.count(f"[{number}] + done sleep 0.7\n"), 1)
        self.assertEqual(error, "")

    def test_background_redirection_and_pipeline(self):
        source = self.directory / "source"
        source.write_text("pear\napple\npear\n")
        commands = ("cat < source > copy &\n"
                    "cat > reversed < source &\n"
                    "cat < source | sort | uniq > sorted &")
        _, _, error = self.run_shell(commands)
        for name in ("copy", "reversed"):
            self.assertEqual((self.directory / name).read_text(), source.read_text())
        self.assertEqual((self.directory / "sorted").read_text(), "apple\npear\n")
        for name in ("copy", "reversed", "sorted"):
            self.assertEqual((self.directory / name).stat().st_mode & 0o777, 0o600)
        self.assertEqual(error, "")

    def test_cd_default_expansion_and_pwd(self):
        nested = self.home / "nested"
        nested.mkdir()
        body, _, error = self.run_shell("cd\npwd\ncd ~/nested\necho $PWD\ncd ..\npwd")
        self.assertEqual(body.splitlines(), [str(self.home), str(nested), str(self.home)])
        self.assertEqual(error, "")

    def test_cd_errors_leave_directory_unchanged(self):
        (self.directory / "file").write_text("data")
        for command in ("cd missing", "cd file", "cd home extra"):
            with self.subTest(command=command):
                body, history, error = self.run_shell(command + "\npwd")
                self.assertEqual(body, str(self.directory) + "\n")
                self.assertEqual(history, ["pwd"])
                self.assertIn("cd:", error)

    def test_cd_with_home_unset(self):
        del self.env["HOME"]
        body, _, error = self.run_shell("cd")
        self.assertEqual(body, "No valid commands.\n")
        self.assertIn("HOME is not set", error)

    def test_child_builtins_do_not_change_parent(self):
        body, _, error = self.run_shell("cd home | cat\npwd\ncd home &\npwd\n"
                                        "exit | cat\necho still-running")
        self.assertEqual(body.splitlines().count(str(self.directory)), 2)
        self.assertIn("still-running\n", body)
        self.assertEqual(error, "")

    def test_builtin_redirection_restores_shell_descriptors(self):
        body, _, error = self.run_shell("jobs > job-list\ncd home\npwd\n"
                                        "jobs > ../missing/output\necho restored")
        self.assertEqual((self.directory / "job-list").read_text(),
                         "No active background processes.\n")
        self.assertEqual(body.splitlines(), [str(self.home), "restored"])
        self.assertIn("missing/output", error)

    def test_exit_history_counts(self):
        for count in range(5):
            with self.subTest(count=count):
                commands = [f"echo command-{i}" for i in range(count)]
                body, history, error = self.run_shell("\n".join(commands))
                expected = commands[-3:] if count >= 3 else commands[-1:]
                self.assertEqual(history, expected)
                if count == 0:
                    self.assertEqual(body, "No valid commands.\n")
                self.assertEqual(error, "")

    def test_invalid_commands_excluded_from_history(self):
        commands = ("echo good\nmissing_shell_command\ncd missing\necho bad |\n"
                    "echo bad > missing/output\ncat < missing-input\njobs extra\nexit extra")
        _, history, error = self.run_shell(commands)
        self.assertEqual(history, ["echo good"])
        self.assertTrue(error)

    def test_exec_failure_excluded_from_history(self):
        invalid = self.directory / "invalid"
        invalid.write_text("not an executable format\n")
        invalid.chmod(0o700)
        body, history, error = self.run_shell("./invalid &")
        self.assertEqual(body, "No valid commands.\n")
        self.assertEqual(history, [])
        self.assertIn("./invalid", error)

    def test_nonzero_program_exit_is_valid(self):
        _, history, error = self.run_shell("false")
        self.assertEqual(history, ["false"])
        self.assertEqual(error, "")

    def test_eof_waits_for_jobs(self):
        body, history, error = self.run_shell("sleep 0.2 &\n", auto_exit=False)
        self.assertIn("[1] + done sleep 0.2\n", body)
        self.assertEqual(history, ["sleep 0.2 &"])
        self.assertEqual(error, "")

    def test_repeated_pipelines_do_not_leak_descriptors(self):
        commands = "\n".join(["echo data | cat | cat > output"] * 100)
        _, _, error = self.run_shell(commands)
        self.assertEqual((self.directory / "output").read_text(), "data\n")
        self.assertEqual(error, "")


if __name__ == "__main__":
    unittest.main(verbosity=2)
