import os
import queue
import signal
import subprocess
import sys
import threading


def run_command(binary, *arguments, expected=0):
    result = subprocess.run(
        [binary, *arguments], text=True, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, timeout=5, check=False
    )
    if result.returncode != expected:
        raise AssertionError(
            f"{arguments!r} returned {result.returncode}, expected {expected}: "
            f"stdout={result.stdout!r} stderr={result.stderr!r}"
        )
    return result.stdout + result.stderr


def run_start(binary, stop_signal):
    creationflags = 0
    if os.name == "nt":
        creationflags = subprocess.CREATE_NEW_PROCESS_GROUP
    process = subprocess.Popen(
        [binary, "start"], text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, creationflags=creationflags
    )
    first_line = queue.Queue(maxsize=1)
    output_parts = []

    def read_startup_line():
        startup_line = process.stdout.readline()
        first_line.put(startup_line)
        output_parts.append(startup_line + process.stdout.read())

    reader = threading.Thread(target=read_startup_line, daemon=True)
    reader.start()
    try:
        startup = first_line.get(timeout=8)
        if "running in the foreground" not in startup:
            raise AssertionError(f"start did not report readiness: {startup!r}")
        process.send_signal(stop_signal)
        process.wait(timeout=5)
        reader.join(timeout=5)
        if reader.is_alive() or not output_parts:
            raise AssertionError("failed to collect complete start output")
        output = output_parts[0]
        if process.returncode != 0:
            raise AssertionError(f"start returned {process.returncode}: {output!r}")
        if "Networking is not implemented; no listener has been started." not in output:
            raise AssertionError(f"start output was misleading or incomplete: {output!r}")
        if "LinkOS stopped." not in output:
            raise AssertionError(f"start did not report graceful shutdown: {output!r}")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=5)
            reader.join(timeout=5)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: test_cli_integration.py <linkos-executable>")
    binary = sys.argv[1]
    help_text = run_command(binary, "help")
    if "Usage:" not in help_text:
        raise AssertionError("help output is missing usage")
    version = run_command(binary, "--version")
    if "LinkOS" not in version:
        raise AssertionError("version output is missing product name")
    doctor = run_command(binary, "doctor")
    if "Platform:" not in doctor or "Config directory:" not in doctor:
        raise AssertionError("doctor output is missing platform/config diagnostics")
    run_command(binary, "not-a-command", expected=2)

    if os.name == "nt":
        # A child created as a new process group reliably receives CTRL_BREAK;
        # CTRL_C is not generated for a process group via GenerateConsoleCtrlEvent.
        run_start(binary, signal.CTRL_BREAK_EVENT)
    else:
        run_start(binary, signal.SIGINT)
        run_start(binary, signal.SIGTERM)
    print("CLI integration checks passed.")


if __name__ == "__main__":
    main()
