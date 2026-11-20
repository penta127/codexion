"""Run after make: python3 tests/test_codexion.py."""
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "codexion"
LINE = re.compile(r"(\d+) (\d+) (has taken a dongle|is compiling|"
                  r"is debugging|is refactoring|burned out)")


def run(args, code=0):
    result = subprocess.run([str(BIN), *map(str, args)], cwd=ROOT,
                            capture_output=True, text=True, timeout=8)
    assert result.returncode == code, (args, result.stderr)
    return result.stdout.splitlines()


def check_run(args, burnout=False):
    n, burn, compile_ms, debug_ms, refactor_ms, required, cooldown, _ = args
    lines = run(args)
    counts = [0] * n
    last_start = [0] * n
    last_event = [None] * n
    takes = [0] * n
    available = [0] * n
    previous_time = 0
    deaths = 0
    for line in lines:
        match = LINE.fullmatch(line)
        assert match and deaths == 0, (args, line)
        timestamp, ident = map(int, match.group(1, 2))
        event = match.group(3)
        i = ident - 1
        assert 0 <= i < n and timestamp >= previous_time, (args, line)
        previous_time = timestamp
        if event == "has taken a dongle":
            takes[i] += 1
        elif event == "is compiling":
            assert n > 1 and takes[i] == 2, (args, line)
            assert timestamp - last_start[i] < burn + 2, (args, line)
            if last_event[i]:
                old_time, old_event = last_event[i]
                assert old_event == "is refactoring", (args, line)
                assert timestamp - old_time >= refactor_ms, (args, line)
            for dongle in (i, (i + 1) % n):
                assert timestamp >= available[dongle], (args, line)
                available[dongle] = timestamp + compile_ms + cooldown
            last_start[i] = timestamp
            counts[i] += 1
            takes[i] = 0
        elif event == "is debugging":
            old_time, old_event = last_event[i]
            assert old_event == "is compiling", (args, line)
            assert timestamp - old_time >= compile_ms, (args, line)
        elif event == "is refactoring":
            old_time, old_event = last_event[i]
            assert old_event == "is debugging", (args, line)
            assert timestamp - old_time >= debug_ms, (args, line)
        elif event == "burned out":
            deaths += 1
            delay = timestamp - last_start[i] - burn
            assert -1 <= delay <= 10, (args, delay, line)
        if event != "has taken a dongle":
            last_event[i] = (timestamp, event)
    assert bool(deaths) == burnout, (args, lines)
    if not burnout:
        assert all(count == required for count in counts), (args, counts)


def defense_cases():
    for policy in ("fifo", "edf"):
        for n in (2, 5, 50, 200):
            check_run([n, 3000, 60, 60, 60, 3, 60, policy])
        check_run([1, 120, 60, 60, 60, 2, 60, policy], burnout=True)
        check_run([2, 120, 180, 60, 60, 2, 60, policy], burnout=True)
    print("PASS: 12 defense scenarios (at most 200 coders, times >= 60ms)")


def main():
    cases = 0
    for policy in ("fifo", "edf"):
        for n in (2, 3, 4, 5, 10, 30):
            for cooldown in (0, 7):
                check_run([n, 3000, 8, 3, 4, 4, cooldown, policy])
                cases += 1
        for args in ([1, 40, 10, 10, 10, 2, 0, policy],
                     [2, 30, 80, 1, 1, 3, 0, policy],
                     [3, 0, 5, 5, 5, 1, 0, policy]):
            check_run(args, burnout=True)
            cases += 1
        check_run([4, 3000, 0, 0, 0, 10, 0, policy])
        assert run([4, 100, 10, 10, 10, 0, 0, policy]) == []
    valid = [2, 1000, 10, 10, 10, 2, 0, "fifo"]
    for index, value in ((0, 0), (0, "2147483648"), (1, -1),
                         (2, "1x"), (3, ""), (4, "+1"),
                         (5, "1.5"), (6, -1), (7, "invalid")):
        args = valid.copy()
        args[index] = value
        run(args, code=1)
    run([], code=1)
    run(valid + [1], code=1)
    print(f"PASS: {cases} timed scenarios, zero cases and input errors")
    defense_cases()


if __name__ == "__main__":
    main()
