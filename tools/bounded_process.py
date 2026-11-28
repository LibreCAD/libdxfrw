#!/usr/bin/env python3
"""Bounded capture for trusted, noninteractive comparison executables.

Return subprocess.CompletedProcess with byte strings, including nonzero exits.
TimeoutExpired and OutputLimitExceeded carry capped stdout/stderr. Temporary
file capture avoids buffered-reader threads whose close can hang on inherited
pipes. File sizes are polled: captured/read bytes are strictly capped, but the
temporary spool can transiently exceed the limit between polls. This is not a
sandbox or a hard disk quota, and process creation itself is an OS operation
which Python cannot put on a portable deadline.

POSIX children run in an owned session/process group. Windows uses a private
kill-on-close job, a restricted handle list, and suspended creation followed by
job assignment before ResumeThread. Rejected native capabilities fail closed;
there is no unbounded/direct-child-only Windows fallback. The Windows path is
source-reviewed, not a claim of native Windows qualification.
"""

from __future__ import annotations

import math
import os
import shutil
import signal
import subprocess
import tempfile
import time


POLL_SECONDS = 0.005
CLEANUP_SECONDS = 0.25


class ProcessCapabilityError(subprocess.SubprocessError):
    """The platform cannot provide the requested owned-process cleanup."""


class OutputLimitExceeded(subprocess.SubprocessError):
    def __init__(self, command, limit, stdout, stderr):
        super().__init__("process exceeded per-stream log byte limit %d" % limit)
        self.cmd = command
        self.limit = limit
        self.stdout = self.output = stdout
        self.stderr = stderr


class _PosixChild:
    def __init__(self, command, stdout, stderr, stdin, cwd, env):
        self.process = subprocess.Popen(command, stdout=stdout, stderr=stderr,
                                        stdin=stdin, cwd=cwd, env=env,
                                        start_new_session=True)

    def poll(self):
        return self.process.poll()

    def cleanup(self):
        # This process group was created solely for the requested invocation.
        # Kill it even after the direct child exits: descendants may retain logs.
        # Darwin can return EPERM while an exiting direct child is still an
        # unreaped group member. Reap and retry within the same cleanup budget;
        # never ignore a permission failure that could leave descendants alive.
        deadline = time.monotonic() + CLEANUP_SECONDS
        def reap():
            try:
                self.process.wait(timeout=max(0, deadline - time.monotonic()))
            except subprocess.TimeoutExpired as exc:
                raise ProcessCapabilityError("owned POSIX process could not be reaped within cleanup budget") from exc
        self.process.poll()
        try:
            os.killpg(self.process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        except PermissionError:
            reap()
            try:
                os.killpg(self.process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            except PermissionError as exc:
                raise ProcessCapabilityError("owned POSIX process group cleanup was denied") from exc
        reap()


def _windows_child(command, stdout, stderr, stdin, cwd, env):
    """Public Win32 APIs only; create suspended to close the job-assignment race.

    Primary references:
    https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
    https://learn.microsoft.com/windows/win32/api/jobapi2/nf-jobapi2-assignprocesstojobobject
    https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute
    https://learn.microsoft.com/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information
    """
    import ctypes as c
    from ctypes import wintypes as w
    import msvcrt

    class IO_COUNTERS(c.Structure):
        _fields_ = [(name, c.c_ulonglong) for name in (
            "ReadOperationCount", "WriteOperationCount", "OtherOperationCount",
            "ReadTransferCount", "WriteTransferCount", "OtherTransferCount")]

    class BASIC_LIMIT(c.Structure):
        _fields_ = [("PerProcessUserTimeLimit", c.c_longlong),
                    ("PerJobUserTimeLimit", c.c_longlong), ("LimitFlags", w.DWORD),
                    ("MinimumWorkingSetSize", c.c_size_t), ("MaximumWorkingSetSize", c.c_size_t),
                    ("ActiveProcessLimit", w.DWORD), ("Affinity", c.c_size_t),
                    ("PriorityClass", w.DWORD), ("SchedulingClass", w.DWORD)]

    class EXTENDED_LIMIT(c.Structure):
        _fields_ = [("BasicLimitInformation", BASIC_LIMIT), ("IoInfo", IO_COUNTERS)] + [
            (name, c.c_size_t) for name in ("ProcessMemoryLimit", "JobMemoryLimit",
                                          "PeakProcessMemoryUsed", "PeakJobMemoryUsed")]

    class STARTUPINFO(c.Structure):
        _fields_ = [("cb", w.DWORD), ("lpReserved", w.LPWSTR),
                    ("lpDesktop", w.LPWSTR), ("lpTitle", w.LPWSTR)] + [
            (name, w.DWORD) for name in ("dwX", "dwY", "dwXSize", "dwYSize",
                "dwXCountChars", "dwYCountChars", "dwFillAttribute", "dwFlags")] + [
            ("wShowWindow", w.WORD), ("cbReserved2", w.WORD),
            ("lpReserved2", c.POINTER(w.BYTE)), ("hStdInput", w.HANDLE),
            ("hStdOutput", w.HANDLE), ("hStdError", w.HANDLE)]

    class STARTUPINFOEX(c.Structure):
        _fields_ = [("StartupInfo", STARTUPINFO), ("lpAttributeList", c.c_void_p)]

    class PROCESS_INFORMATION(c.Structure):
        _fields_ = [("hProcess", w.HANDLE), ("hThread", w.HANDLE),
                    ("dwProcessId", w.DWORD), ("dwThreadId", w.DWORD)]

    api = c.WinDLL("kernel32", use_last_error=True)
    signatures = {
        "CreateJobObjectW": ([c.c_void_p, w.LPCWSTR], w.HANDLE),
        "SetInformationJobObject": ([w.HANDLE, c.c_int, c.c_void_p, w.DWORD], w.BOOL),
        "AssignProcessToJobObject": ([w.HANDLE, w.HANDLE], w.BOOL),
        "TerminateJobObject": ([w.HANDLE, w.UINT], w.BOOL),
        "GetCurrentProcess": ([], w.HANDLE),
        "DuplicateHandle": ([w.HANDLE, w.HANDLE, w.HANDLE, c.POINTER(w.HANDLE),
                              w.DWORD, w.BOOL, w.DWORD], w.BOOL),
        "InitializeProcThreadAttributeList": ([c.c_void_p, w.DWORD, w.DWORD,
                                                c.POINTER(c.c_size_t)], w.BOOL),
        "UpdateProcThreadAttribute": ([c.c_void_p, w.DWORD, c.c_size_t, c.c_void_p,
                                        c.c_size_t, c.c_void_p, c.c_void_p], w.BOOL),
        "DeleteProcThreadAttributeList": ([c.c_void_p], None),
        "CreateProcessW": ([w.LPCWSTR, w.LPWSTR, c.c_void_p, c.c_void_p, w.BOOL,
                             w.DWORD, c.c_void_p, w.LPCWSTR, c.c_void_p,
                             c.POINTER(PROCESS_INFORMATION)], w.BOOL),
        "ResumeThread": ([w.HANDLE], w.DWORD),
        "TerminateProcess": ([w.HANDLE, w.UINT], w.BOOL),
        "WaitForSingleObject": ([w.HANDLE, w.DWORD], w.DWORD),
        "GetExitCodeProcess": ([w.HANDLE, c.POINTER(w.DWORD)], w.BOOL),
        "CloseHandle": ([w.HANDLE], w.BOOL),
    }
    try:
        for name, (arguments, result) in signatures.items():
            function = getattr(api, name)
            function.argtypes, function.restype = arguments, result
    except AttributeError as exc:
        raise ProcessCapabilityError("Windows job/handle-list APIs unavailable") from exc

    def checked(success, operation):
        if not success:
            raise ProcessCapabilityError("Windows %s unavailable: %s" % (
                operation, c.WinError(c.get_last_error())))

    job = api.CreateJobObjectW(None, None)
    checked(job, "private job creation")
    information = PROCESS_INFORMATION()
    inherited = []
    attribute_list = None
    attributes_initialized = False
    try:
        limits = EXTENDED_LIMIT()
        limits.BasicLimitInformation.LimitFlags = 0x2000  # KILL_ON_JOB_CLOSE
        checked(api.SetInformationJobObject(job, 9, c.byref(limits), c.sizeof(limits)),
                "kill-on-close job limit")
        current = api.GetCurrentProcess()
        for stream in (stdin, stdout, stderr):
            handle = w.HANDLE()
            checked(api.DuplicateHandle(current, msvcrt.get_osfhandle(stream.fileno()),
                    current, c.byref(handle), 0, True, 2), "inheritable log handle")
            inherited.append(handle.value)
        size = c.c_size_t()
        api.InitializeProcThreadAttributeList(None, 1, 0, c.byref(size))
        if not size.value:
            raise ProcessCapabilityError("Windows handle-list allocation size unavailable")
        storage = c.create_string_buffer(size.value)
        attribute_list = c.cast(storage, c.c_void_p)
        checked(api.InitializeProcThreadAttributeList(attribute_list, 1, 0, c.byref(size)),
                "handle-list initialization")
        attributes_initialized = True
        handles = (w.HANDLE * len(inherited))(*inherited)
        checked(api.UpdateProcThreadAttribute(attribute_list, 0, 0x20002,
                    c.byref(handles), c.sizeof(handles), None, None), "restricted inherited handles")
        startup = STARTUPINFOEX()
        startup.StartupInfo.cb = c.sizeof(startup)
        startup.StartupInfo.dwFlags = 0x100  # STARTF_USESTDHANDLES
        startup.StartupInfo.hStdInput, startup.StartupInfo.hStdOutput, startup.StartupInfo.hStdError = inherited
        startup.lpAttributeList = attribute_list
        environment = None
        if env is not None:
            environment = c.create_unicode_buffer("\0".join(
                "%s=%s" % (key, env[key]) for key in sorted(env, key=str.upper)) + "\0\0")
        search_environment = os.environ if env is None else env
        search_path = next((value for key, value in search_environment.items()
                            if key.upper() == "PATH"), os.defpath)
        executable = shutil.which(command[0], path=search_path)
        if executable is None:
            candidate = os.path.join(cwd or os.getcwd(), command[0])
            if os.path.isfile(candidate):
                executable = os.path.abspath(candidate)
            else:
                raise FileNotFoundError(command[0])
        flags = 0x4 | 0x80000 | 0x400  # SUSPENDED | EXTENDED_STARTUPINFO | UNICODE_ENV
        line = c.create_unicode_buffer(subprocess.list2cmdline(command))
        checked(api.CreateProcessW(executable, line, None, None, True, flags,
                    environment, cwd, c.byref(startup), c.byref(information)), "suspended process creation")
        checked(api.AssignProcessToJobObject(job, information.hProcess), "process job assignment")
        if api.ResumeThread(information.hThread) == 0xFFFFFFFF:
            checked(False, "primary-thread resume")
        api.CloseHandle(information.hThread)
        information.hThread = None
    except BaseException:
        if information.hProcess:
            api.TerminateProcess(information.hProcess, 1)
            api.WaitForSingleObject(information.hProcess, int(CLEANUP_SECONDS * 1000))
            api.CloseHandle(information.hProcess)
        if information.hThread:
            api.CloseHandle(information.hThread)
        api.CloseHandle(job)
        raise
    finally:
        if attributes_initialized:
            api.DeleteProcThreadAttributeList(attribute_list)
        for handle in inherited:
            api.CloseHandle(handle)

    class WindowsChild:
        def poll(self):
            state = api.WaitForSingleObject(information.hProcess, 0)
            if state == 258:  # WAIT_TIMEOUT
                return None
            checked(state == 0, "process wait")
            code = w.DWORD()
            checked(api.GetExitCodeProcess(information.hProcess, c.byref(code)), "process exit code")
            return code.value

        def cleanup(self):
            try:
                checked(api.TerminateJobObject(job, 1), "owned-job termination")
                checked(api.WaitForSingleObject(information.hProcess,
                        int(CLEANUP_SECONDS * 1000)) == 0, "bounded process cleanup")
            finally:
                api.CloseHandle(job)  # Kill-on-close also covers descendants.
                api.CloseHandle(information.hProcess)

    return WindowsChild()


def run_bounded(command, timeout, output_limit=65536, *, cwd=None, env=None):
    """Run an argv sequence without a shell; bound runtime and each log stream.

    The runtime budget starts after process creation; cleanup has a separate
    250ms direct-process reap budget. Descendants in the owned group/job are
    terminated even on a successful direct-child exit. Callers must not use
    this for services intended to survive the invocation.
    """
    if type(timeout) not in (int, float) or not math.isfinite(timeout) or not 0 < timeout <= 60:
        raise ValueError("timeout must be finite and in (0, 60]")
    if type(output_limit) is not int or output_limit <= 0:
        raise ValueError("output limit must be a positive integer")
    if not isinstance(command, (list, tuple)) or not command:
        raise ValueError("command must be a nonempty argv sequence")
    command = [os.fspath(argument) for argument in command]
    if any(not isinstance(argument, str) or "\0" in argument for argument in command) or not command[0]:
        raise ValueError("argv must contain text without NUL")
    if cwd is not None:
        cwd = os.fsdecode(os.fspath(cwd))
        if "\0" in cwd:
            raise ValueError("cwd must not contain NUL")
    if env is not None and any(type(key) is not str or type(value) is not str
            or not key or "=" in key or "\0" in key or "\0" in value for key, value in env.items()):
        raise ValueError("environment must contain valid text keys/values")
    if os.name not in ("posix", "nt"):
        raise ProcessCapabilityError("bounded process cleanup unsupported on " + os.name)
    with tempfile.TemporaryFile() as stdout, tempfile.TemporaryFile() as stderr, open(os.devnull, "rb") as stdin:
        child = (_PosixChild(command, stdout, stderr, stdin, cwd, env) if os.name == "posix"
                 else _windows_child(command, stdout, stderr, stdin, cwd, env))
        deadline = time.monotonic() + timeout
        reason = None
        try:
            while True:
                if any(os.fstat(stream.fileno()).st_size > output_limit for stream in (stdout, stderr)):
                    reason = "overflow"
                    break
                status = child.poll()
                if status is not None:
                    break
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    reason = "timeout"
                    break
                time.sleep(min(POLL_SECONDS, remaining))
        finally:
            child.cleanup()
        sizes = [os.fstat(stream.fileno()).st_size for stream in (stdout, stderr)]
        captured = []
        for stream in (stdout, stderr):
            stream.seek(0)
            captured.append(stream.read(output_limit))
        if reason == "timeout":
            raise subprocess.TimeoutExpired(command, timeout, output=captured[0], stderr=captured[1])
        if reason == "overflow" or any(size > output_limit for size in sizes):
            raise OutputLimitExceeded(command, output_limit, *captured)
        return subprocess.CompletedProcess(command, status, *captured)
