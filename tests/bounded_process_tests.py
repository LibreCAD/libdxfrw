#!/usr/bin/env python3
"""Dependency-free, actual subprocess controls for tools/bounded_process.py."""

import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from bounded_process import OutputLimitExceeded, ProcessCapabilityError, _PosixChild, run_bounded


class BoundedProcessTests(unittest.TestCase):
    def python(self, source, timeout=2, output_limit=65536, **kwargs):
        return run_bounded([sys.executable, "-c", source], timeout,
                           output_limit=output_limit, **kwargs)

    def test_normal_unicode_nonzero_and_argv(self):
        source = ("import os,sys; "
                  "os.write(1,sys.argv[1].encode('utf-8')); "
                  "os.write(2,'err €\\n'.encode('utf-8')); sys.exit(7)")
        result = run_bounded([sys.executable, "-c", source, 'space € "quoted"'], 2)
        self.assertEqual(result.returncode, 7)
        self.assertEqual(result.stdout, 'space € "quoted"'.encode())
        self.assertEqual(result.stderr, 'err €\n'.encode())
        with self.assertRaises(subprocess.CalledProcessError):
            result.check_returncode()

    def test_environment_and_cwd(self):
        with tempfile.TemporaryDirectory(prefix="bounded process € ") as directory:
            environment = os.environ.copy()
            environment["LIBDXFRW_BOUNDED_CONTROL"] = "value €"
            result = self.python("import json,os; print(json.dumps([os.getcwd(),"
                                 "os.environ['LIBDXFRW_BOUNDED_CONTROL']]))",
                                 cwd=directory, env=environment)
            actual = json.loads(result.stdout)
            self.assertEqual(Path(actual[0]).resolve(), Path(directory).resolve())
            self.assertEqual(actual[1], "value €")

    def test_timeout_has_bounded_capture(self):
        started = time.monotonic()
        with self.assertRaises(subprocess.TimeoutExpired) as context:
            self.python("import os,time; os.write(1,b'ready\\n'); "
                        "os.write(2,'€'.encode()); time.sleep(10)", timeout=.1)
        self.assertLess(time.monotonic() - started, 1)
        self.assertEqual(context.exception.stdout, b"ready\n")
        self.assertEqual(context.exception.stderr, "€".encode())

    def test_per_stream_limit_and_exact_boundary(self):
        for descriptor in (1, 2):
            with self.subTest(stream=descriptor):
                started = time.monotonic()
                with self.assertRaises(OutputLimitExceeded) as context:
                    self.python("import os,time; os.write(%d,b'x'*2048); time.sleep(10)"
                                % descriptor, output_limit=128)
                self.assertLess(time.monotonic() - started, 1)
                self.assertLessEqual(len(context.exception.stdout), 128)
                self.assertLessEqual(len(context.exception.stderr), 128)
                selected = context.exception.stdout if descriptor == 1 else context.exception.stderr
                self.assertEqual(selected, b"x" * 128)
        result = self.python("import os; os.write(1,b'a'*128); os.write(2,b'b'*128)",
                             output_limit=128)
        self.assertEqual((result.stdout, result.stderr), (b"a" * 128, b"b" * 128))

    def test_fast_exit_overflow(self):
        for descriptor in (1, 2):
            for attempt in range(3):
                with self.subTest(stream=descriptor, attempt=attempt):
                    with self.assertRaises(OutputLimitExceeded) as context:
                        self.python("import os; os.write(%d,b'x'*4096)" % descriptor,
                                    output_limit=128)
                    self.assertLessEqual(len(context.exception.stdout), 128)
                    self.assertLessEqual(len(context.exception.stderr), 128)

    @unittest.skipUnless(os.name == "posix", "POSIX cleanup capability control")
    def test_permission_retry_still_fails_closed(self):
        child = _PosixChild.__new__(_PosixChild)
        child.process = Mock(pid=123)
        child.process.wait.return_value = 0
        with patch("bounded_process.os.killpg", side_effect=PermissionError("denied")) as kill:
            with self.assertRaises(ProcessCapabilityError):
                child.cleanup()
        self.assertEqual(kill.call_count, 2)
        self.assertTrue(child.process.wait.called)
        child.process.wait.side_effect = subprocess.TimeoutExpired(["control"], .25)
        with patch("bounded_process.os.killpg", side_effect=PermissionError("denied")) as kill:
            with self.assertRaises(ProcessCapabilityError):
                child.cleanup()
        self.assertEqual(kill.call_count, 1)

    def descendant(self, wait_parent):
        # The child's file lock proves resource cleanup, not merely an early
        # return while a log-holding process survives in the background.
        with tempfile.TemporaryDirectory(prefix="bounded retained logs ") as directory:
            ready = Path(directory) / "ready.json"
            lock = Path(directory) / "owned.lock"
            child = """import json,os,pathlib,sys,time
f=open(sys.argv[2],'a+b'); f.write(b'x'); f.flush(); f.seek(0)
if os.name == 'nt':
    import msvcrt
    msvcrt.locking(f.fileno(),msvcrt.LK_NBLCK,1)
else:
    import fcntl
    fcntl.flock(f,fcntl.LOCK_EX | fcntl.LOCK_NB)
p=pathlib.Path(sys.argv[1]); t=p.with_suffix('.tmp')
t.write_text(json.dumps({'pid':os.getpid()}))
os.write(1,b'descendant-ready\\n'); os.replace(t,p); time.sleep(10)
"""
            parent = (
                "import pathlib,subprocess,sys,time; "
                "subprocess.Popen([sys.executable,'-c',sys.argv[1],sys.argv[2],sys.argv[3]],close_fds=False); "
                "p=pathlib.Path(sys.argv[2]); "
                "exec('while not p.exists(): time.sleep(.001)'); "
                + ("time.sleep(10)" if wait_parent else "sys.exit(0)"))
            command = [sys.executable, "-c", parent, child, str(ready), str(lock)]
            started = time.monotonic()
            if wait_parent:
                with self.assertRaises(subprocess.TimeoutExpired):
                    run_bounded(command, .1)
            else:
                result = run_bounded(command, .1)
                self.assertEqual(result.returncode, 0)
                self.assertIn(b"descendant-ready", result.stdout)
            self.assertLess(time.monotonic() - started, 1)
            metadata = json.loads(ready.read_text())
            released = False
            deadline = time.monotonic() + .25
            while time.monotonic() < deadline:
                try:
                    with lock.open("r+b") as probe:
                        if os.name == "nt":
                            import msvcrt
                            msvcrt.locking(probe.fileno(), msvcrt.LK_NBLCK, 1)
                            probe.seek(0)
                            msvcrt.locking(probe.fileno(), msvcrt.LK_UNLCK, 1)
                        else:
                            import fcntl
                            fcntl.flock(probe, fcntl.LOCK_EX | fcntl.LOCK_NB)
                    released = True
                    break
                except OSError:
                    time.sleep(.005)
            if not released:
                # Only this from-scratch control's exact descendant is targeted.
                os.kill(metadata["pid"], signal.SIGTERM)
            self.assertTrue(released, "owned descendant retained its file lock after cleanup")

    def test_retained_descendant_after_parent_exit(self):
        self.descendant(False)

    def test_retained_descendant_after_timeout(self):
        self.descendant(True)

    def test_invalid_arguments_and_missing_executable(self):
        for timeout in (0, -1, True, float("nan"), float("inf"), 61):
            with self.subTest(timeout=timeout), self.assertRaises(ValueError):
                run_bounded([], timeout)
        for limit in (0, -1, True, 1.5):
            with self.subTest(limit=limit), self.assertRaises(ValueError):
                run_bounded([sys.executable], 1, limit)
        for command in ([], "echo control", [sys.executable, "bad\0argument"]):
            with self.subTest(command=command), self.assertRaises(ValueError):
                run_bounded(command, 1)
        with self.assertRaises((OSError, subprocess.SubprocessError)):
            run_bounded(["libdxfrw-intentionally-missing-command"], 1)


if __name__ == "__main__":
    unittest.main()
