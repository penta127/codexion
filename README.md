*This project has been created as part of the 42 curriculum by koito.*

# Codexion

## Description

A C simulation of coders sharing USB dongles around a ring. Each coder needs
both adjacent dongles to compile, then debugs and refactors. A separate monitor
detects burnout. A custom binary min-heap orders resource requests using FIFO
or earliest-deadline-first (EDF).

The original Japanese task notes are preserved in [docs/subject_ja.md](docs/subject_ja.md).

For a Japanese walkthrough connecting the requirements to the actual code,
see [the implementation guide](docs/implementation_ja.md).

## Instructions

Build with `make`. The executable is compiled with
`-Wall -Wextra -Werror -pthread`.

```sh
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
time_to_refactor number_of_compiles_required dongle_cooldown scheduler
./codexion 5 1000 100 100 100 3 20 edf
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
  then arrival sequence. During each dispatch pass, an ungrantable request marks
  both of its dongles as blocked for lower-priority requests. This is a logical
  reservation, not held mutexes: current owners can still release their dongles.
  Requests with disjoint unblocked pairs can proceed. Reservations are reset
  and rebuilt in priority order on every pass, so an earlier EDF deadline can
  supersede an older request. Strict per-dongle ordering can leave a free dongle
  idle while its higher-priority requester waits for its other dongle.
  Infeasible timings still result in burnout.
- **Cooldown:** releasing a pair sets each dongle's next-available timestamp to
  the release time plus cooldown. It is checked before every grant.
- **Burnout:** the dedicated monitor polls shared timestamps every 500 microseconds.
  Tests check the requested 10 ms logging tolerance; OS scheduling and blocked
  output can affect actual timing.
- **Log serialization:** state updates precede logging under a shared state lock,
  then a dedicated log lock. Burnout sets the stop state under that same state
  lock, preventing subsequent normal messages.
- **Partial startup:** all threads wait behind a start gate. If any creation
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

## Code reading order

1. `main.c`, `src/a1/`: argument validation and config.
2. `src/a2/`: coder/dongle/shared initialization and ownership cleanup.
3. `src/a3/start_coders.c`: create, common start gate, failure stop, join.
4. `src/runtime/coder_routine.c`: compile, debug, refactor cycle.
5. `src/runtime/scheduler.c`, `src/heap/`: requests and priorities.
6. `src/runtime/dongles.c`: pair acquisition, release and cooldown.
7. `src/runtime/monitor.c`: burnout and all-completed termination.
8. `src/runtime/time.c`, `log.c`: elapsed time, interruptible waits and logging.

## Validation

```sh
make
make test
python3 tests/test_codexion.py
norminette codexion.h main.c basic_code src
valgrind --leak-check=full --error-exitcode=99 ./codexion 5 3000 5 3 3 3 2 edf
valgrind --tool=helgrind --error-exitcode=99 ./codexion 4 3000 5 3 3 2 1 fifo
```

The scheduler regression test checks FIFO non-overtaking, independent pairs,
EDF higher-ID tie-breaking despite later arrival, and deadline precedence.
The Python tests validate log format, per-coder ordering, compile counts, minimum
action durations, pair exclusion, cooldown, zero values, invalid input, one-coder
burnout and burnout latency. Initialization and thread-creation failure paths
were also checked using temporary linker fault-injection wrappers.

## Resources

- POSIX threads manual pages: `man pthread_create`, `man pthread_join`,
  `man pthread_mutex_lock`, `man pthread_cond_wait`.
- Time APIs: `man clock_gettime`, `man usleep`.
- Coffman et al., "System Deadlocks" (1971), for deadlock conditions.
- AI assistance: Codex helped implement and review initialization, failure
  cleanup, threading, the scheduler, heap operations, monitoring, Makefile and
  integration tests. The author must understand and review the resulting code
  before evaluation.
