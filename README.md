# Operating Systems Project 1

Build and run on a POSIX system (Linux or macOS) with a C99 compiler:

```sh
make
./bin/shell
```

The executable is generated in `bin/`; generated binaries and object files are
ignored by Git. `make clean` removes build output. `make test` runs the integration
tests using Python 3.6+ and its standard library (override the interpreter with `make test PYTHON=...`); Python is not needed to build or run the shell.

## Supported commands

The shell expands whole-argument environment variables and `~`/`~/`, searches
`$PATH`, and executes programs with `fork()` and `execv()`.

Parts 7-9 add:

- Up to two pipes (three concurrent commands), for example
  `cat input.txt | sort | wc -l`. Unused pipe descriptors are closed in every
  process, and foreground execution waits for all pipeline stages.
- Background execution with a trailing `&`, including pipelines and redirection:
  `sleep 2 &`, `cat input.txt | sort &`, and `cat < input.txt > output.txt &`.
  Start messages use `[number] PID`; the PID is that of the last pipeline stage.
  Completion messages use `[number] + done command`. A pipeline completes only
  when every stage finishes. Job numbers start at 1 and are never reused; up to
  ten jobs are tracked at once. Completion is checked in the main loop, so a
  notification can be delayed while the shell waits for input or a foreground command.
- `cd [PATH]` changes the shell's directory, defaults to `$HOME`, and updates
  `$PWD`. Extra arguments, nonexistent paths, and nondirectory targets report errors.
- `jobs` lists active jobs as `[number]+ PID command`, or reports that none exist.
- `exit` waits for all background processes and displays the three most recent
  valid commands in chronological order. If only one or two valid commands exist,
  it displays just the latest; if none exist, it says so. EOF performs the same cleanup.

History preserves the original input, including variable references and `&`.
Successful built-ins and successfully started external commands count as valid;
an external program's nonzero exit status does not invalidate its command.
Blank input, syntax errors, failed built-ins, command lookup failures, and launch
failures are excluded. `exit` itself is not added to history.

Standalone foreground built-ins run in the shell, including when redirected.
Built-ins in pipelines or background jobs run in child processes, so their `cd`
and `exit` affect only that child. Quotes, escapes, globs, and terminal job control
are outside this project's scope.

## Extra credit

Piping and I/O redirection can be combined, including in the background:

```text
cat < input.txt | sort | uniq > output.txt &
```

Each stage applies its own redirections after connecting its pipe descriptors.
Output files use permissions `-rw-------` and are overwritten rather than appended.

## File listing

- `src/main.c`: prompt/input loop, job polling, and history recording.
- `src/lexer.c`, `include/lexer.h`: input reading and tokenization.
- `src/prompt.c`, `include/prompt.h`: user, machine, and directory prompt.
- `src/expand.c`, `include/expand.h`: environment-variable and tilde expansion.
- `src/path.c`, `include/path.h`: executable lookup.
- `src/redirect.c`, `include/redirect.h`: redirection parsing and descriptor setup.
- `src/execute.c`, `include/execute.h`: pipeline parsing and process execution.
- `src/jobs.c`, `include/jobs.h`: background process tracking and completion.
- `src/builtins.c`, `include/builtins.h`: `cd`, `jobs`, `exit`, and history.
- `include/shell.h`: shared shell state and command results.
- `tests/test_shell.py`: integration tests for parts 7-9 and their interaction
  with the existing command execution and redirection.
- `Makefile`: build, test, dependency tracking, and clean targets.
