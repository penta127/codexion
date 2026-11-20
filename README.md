*This project has been created as part of the 42 curriculum by koito.*

# Codexion

## Description

A C simulation of coders sharing USB dongles around a ring. Each coder needs
both adjacent dongles to compile, then debugs and refactors. A separate monitor
detects burnout. A custom binary min-heap orders resource requests using FIFO
or earliest-deadline-first (EDF).

The original Japanese task notes are preserved in [docs/subject_ja.md](docs/subject_ja.md).

For a Japanese walkthrough connecting the requirements to the actual code,
see [the implementation guide](docs/implementation_ja.md). Evaluation answers,
demonstration commands and follow-up questions are in
[the Japanese defense guide](docs/defense_qa_ja.md).

The [requirements audit](docs/audit_ja.md) records fixes, validation evidence
and the remaining interpretation and guarantee limits.

## Instructions

Build with `make`. The executable is compiled with
`-Wall -Wextra -Werror -pthread`.
Strict C89 compatibility is outside the current agreed scope: this version uses
`stdbool.h` and does not claim strict C89 compliance.

```sh
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
time_to_refactor number_of_compiles_required dongle_cooldown scheduler
./codexion 5 1000 100 100 100 3 60 edf
```

All eight arguments are mandatory. Numeric arguments must contain digits only
and fit in an `int`. The coder count must be positive; other values may be zero.
A zero required count exits immediately without logs. Zero burnout with a
positive required count produces a burnout. Invalid inputs and initialization
failures return status 1; a completed simulation (including burnout) returns 0.

Times are milliseconds. Logs use elapsed monotonic time from the common start.
Each successful compile acquisition prints two dongle lines and one compile
line. Completed compiles, rather than starts, count toward the quota. A coder
retires after completing its quota and is no longer monitored for burnout.
With one coder, the sole dongle is logged once; compilation is impossible.

`make clean` removes objects, `make fclean` also removes the executable, and
`make re` rebuilds. Repeating `make` does not relink unchanged objects.

## Blocking cases handled

- **Deadlock / Coffman conditions:** both dongles are assigned together while
  holding the shared state mutex. A waiting coder holds neither resource, removing
  hold-and-wait. Nested dongle mutexes are always acquired in ascending ID order,
  removing circular lock ordering.
- **Scheduling / starvation:** pending requests are popped from a min-heap in
  priority order. FIFO uses an increasing request sequence. EDF uses
  `last_compile_start + time_to_burnout`, then higher coder ID on equal deadlines,
  then arrival sequence. FIFO logically reserves both dongles of an ungrantable
  request against later requests during that dispatch pass. These reservations
  do not hold mutexes and are rebuilt on the next pass. EDF instead scans
  requests in deadline order and grants available pairs without reserving a
  free dongle for a request waiting on another occupied dongle. This avoids
  chains of idle reservations in EDF. Neither finite regression tests nor this
  priority rule alone prove the strong requirement of no burnout for every
  mathematically feasible input. That universal guarantee remains unproven.
- **Cooldown:** releasing a pair sets each dongle's next-available timestamp to
  the release time plus cooldown. It is checked before every grant.
- **Burnout:** the dedicated monitor polls shared timestamps every 500 microseconds.
  Tests check the requested 10 ms logging tolerance; OS scheduling and blocked
  output can affect actual timing. Compile completion checks the old burnout
  deadline before increasing the count, so a delayed monitor cannot turn an
  already expired final compile into successful retirement. The monitor remains
  the only thread that emits burnout logs.
- **Log serialization:** state updates precede logging under a shared state lock,
  then a dedicated log lock. Burnout sets the stop state under that same state
  lock, preventing subsequent normal messages.
- **Partial startup:** all coders enqueue their first request behind a start
  gate. The main thread waits for every request before setting the common start
  time, so initial equal-deadline EDF decisions include all coders. If creation
  fails, created threads are awakened, stopped and joined before cleanup.
- **Allocation / initialization failures:** only initialized mutexes are
  destroyed; all owned allocations are freed, including heap storage.

## Thread synchronization mechanisms

`t_config` contains only the eight command-line settings.
`t_shared` contains runtime resources, timestamps, the queue, start gate and stop
state. Every coder holds a pointer to the same shared object.

`mutex_stop` protects the stop state, start state, compile counters/timestamps,
and queue operations. Despite its historical name, it is the simulation state
lock. Dongle mutexes protect dongle ownership and cooldown timestamps; they are
taken while the state mutex is held. The order is always state mutex, then
ascending dongle mutexes; logging uses state mutex, then log mutex.

The `changed` condition variable gates startup. Runtime waiting releases the
state lock, sleeps for 500 microseconds and reacquires the lock before checking
the predicate again. Workers can therefore stop during compilation, debugging,
refactoring and resource waits. Compile ownership is released even after stop.
All threads are joined before mutexes, arrays and queue storage are destroyed.

`prepared_count` counts coders that submitted their first request. `queued`
prevents a coder from registering the same request twice. Initial EDF deadlines
are shifted by the same start-time offset without changing their heap order.
The scheduler passes the timestamp used in its deadline check through to compile
logging rather than replacing it with a later timestamp.

## Code reading order

1. `main.c`, `src/a1/`: argument validation and config.
2. `src/a2/`: coder/dongle/shared initialization and ownership cleanup.
3. `src/a3/start_coders.c`: create, common start gate, failure stop, join.
4. `src/runtime/coder_routine.c`: compile, debug, refactor cycle.
5. `src/runtime/request.c`, `scheduler.c`, `src/heap/`: requests and priorities.
6. `src/runtime/dongles.c`: pair acquisition, release and cooldown.
7. `src/runtime/monitor.c`: burnout and all-completed termination.
8. `src/runtime/time.c`, `log.c`: elapsed time, interruptible waits and logging.

## Validation

```sh
make
make test
make test-failures
norminette codexion.h main.c basic_code src
valgrind --leak-check=full --error-exitcode=99 ./codexion 5 3000 60 60 60 2 60 edf
valgrind --tool=helgrind --error-exitcode=99 ./codexion 4 3000 60 60 60 2 60 fifo
```

`make test` runs scheduler and deterministic deadline-boundary tests plus the
Python integration suite. The default timed integration tests use at most 200
coders and time values of at least 60 ms: 12 defense scenarios and four repeated
tight EDF regressions. They check initial EDF higher-ID priority, final compile
duration, log format, state transitions, action durations, pair exclusion,
cooldown, invalid input and burnout latency. `tests/test_deadlines.c` replaces
`now_ms` through a test-only linker wrapper to check exact deadline boundaries.

`make test-failures` injects 17 allocation, mutex, condition-variable and
thread-creation failures and checks cleanup with Valgrind. Its wrappers are test
code and are not linked into the production executable.
If Valgrind is unavailable, the runner still checks failure handling and reports
memory validation as skipped rather than claiming it passed.

Optional short-duration and zero-value tests are available with
`python3 -B tests/test_codexion.py --extended`. These additional cases are not
the grading-time timing conditions. Passing these tests is evidence for the
tested executions, not a proof for every possible input or thread schedule.

## Resources

- POSIX threads manual pages: `man pthread_create`, `man pthread_join`,
  `man pthread_mutex_lock`, `man pthread_cond_wait`.
- Time APIs: `man clock_gettime`, `man usleep`.
- Coffman et al., "System Deadlocks" (1971), for deadlock conditions.
- AI assistance: Codex helped implement and review initialization, failure
  cleanup, threading, the scheduler, heap operations, monitoring, Makefile and
  integration tests. The author must understand and review the resulting code
  before evaluation.
