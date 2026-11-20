"""Test-only linker wrappers; never linked into the production codexion."""
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PROBE = ROOT / "tests" / "test_failures_probe"


def main():
    cases = [("FAIL_MALLOC", i) for i in range(1, 6)]
    cases += [("FAIL_MUTEX", i) for i in range(1, 7)]
    cases += [("FAIL_THREAD", i) for i in range(1, 6)]
    cases += [("FAIL_COND", 1)]
    valgrind = shutil.which("valgrind")
    prefix = []
    if valgrind:
        prefix = [valgrind, "--leak-check=full", "--error-exitcode=99"]
    for kind, index in cases:
        environment = dict(os.environ)
        for key in ("FAIL_MALLOC", "FAIL_MUTEX", "FAIL_THREAD", "FAIL_COND"):
            environment.pop(key, None)
        environment[kind] = str(index)
        command = prefix + [str(PROBE), "4", "3000", "60", "60", "60",
                            "2", "60", "edf"]
        result = subprocess.run(command, cwd=ROOT, env=environment,
                                capture_output=True, text=True, timeout=10)
        assert result.returncode == 1, (kind, index, result.stderr)
        assert result.stdout == "", (kind, index, result.stdout)
        if valgrind:
            assert "ERROR SUMMARY: 0 errors" in result.stderr, result.stderr
            assert "in use at exit: 0 bytes in 0 blocks" in result.stderr, (
                kind, index, result.stderr)
    print("PASS: 17 allocation, mutex, condition and thread creation failures")
    if valgrind:
        print("PASS: Memcheck found no errors or remaining allocations")
    else:
        print("SKIP: memory validation (Valgrind is not installed)")


if __name__ == "__main__":
    main()
