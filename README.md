# Operating Systems Project 1

A small Unix shell written in C99. It shows a `USER@MACHINE:PWD>` prompt, expands
environment variables and `~`, searches `$PATH`, runs programs with `fork()`/`execv()`,
and supports I/O redirection, pipes, background jobs, and the `cd`, `jobs`, and
`exit` built-ins.

## File listing

- `Makefile`: builds the shell into `bin/shell`, with `test`, `clean`, and
  header dependency tracking.
- `README.md`: this file.
- `.gitignore`: keeps the generated `bin/` and `obj/` directories out of Git.
- `src/main.c`: main loop that prints the prompt, reads input, polls background
  jobs, runs commands, and records history.
- `src/lexer.c`, `include/lexer.h`: reads a line of input and splits it into tokens.
- `src/prompt.c`, `include/prompt.h`: prints the `USER@MACHINE:PWD>` prompt.
- `src/expand.c`, `include/expand.h`: environment variable (`$VAR`) and tilde
  (`~`, `~/dir`) expansion.
- `src/path.c`, `include/path.h`: searches `$PATH` for an executable.
- `src/redirect.c`, `include/redirect.h`: parses `<` and `>` and sets up the file
  descriptors in the child.
- `src/execute.c`, `include/execute.h`: parses pipelines and `&`, then forks, pipes,
  and executes each command.
- `src/jobs.c`, `include/jobs.h`: tracks background jobs and reports when they finish.
- `src/builtins.c`, `include/builtins.h`: the `cd`, `jobs`, and `exit` built-ins,
  plus command history.
- `include/shell.h`: shared shell state (job table and history) and command result codes.
- `tests/test_shell.py`: automated integration tests (Python 3 standard library only).

## How to compile

On linprog, or any Linux system with `gcc` and `make`, run this from the
repository root:

```sh
make            # builds bin/shell
./bin/shell     # runs the shell
make clean      # removes bin/ and obj/
```

`make test` builds the shell and runs the integration tests with `python3`. To use
a different interpreter, run `make test PYTHON=...`. Python is only needed for the
tests, not to build or run the shell.

## Known bugs and unfinished portions

We know of no bugs in the required features, and all integration tests pass on
linprog. These limitations are by design or outside the project's scope:

- Variables expand only when they are a whole argument: `echo $HOME` works, but
  `echo $HOME/dir` is not expanded.
- Quotes, escape characters, globs (`*`), and append redirection (`>>`) are not
  supported. Quote characters are passed through literally.
- A command line can contain at most two pipes (three commands). More pipes
  cause a syntax error.
- At most ten background jobs can run at once. An eleventh is refused with an error.
- Background completion is checked each time around the main loop. A `[n] + done`
  message can therefore appear only after the next command is entered or finishes.
- Terminal job control (`fg`, `bg`, Ctrl+Z) is not implemented.

## Notes for grading

- Start messages for background jobs use `[number] PID`. The PID is that of the
  last command in the pipeline. Completion messages use `[number] + done command`.
  A pipeline is done only when every command in it has finished. Job numbers
  start at 1 and are never reused.
- `jobs` prints each active job as `[number]+ PID command`, or reports that there
  are no active jobs.
- `cd` with no argument goes to `$HOME` and updates `$PWD`. Extra arguments,
  nonexistent paths, and targets that are not directories print an error.
- `exit`, and Ctrl+D (EOF), wait for all background jobs and then print the last
  valid commands. If there are three or more, they print the three most recent in
  order. If there are only one or two, they print the latest. If there are none,
  they say so.
- History stores the original input, including `$VAR` references and `&`. A command
  is valid if it is a successful built-in or an external program that started
  successfully, even if the program exits with a nonzero status. Blank lines, syntax
  errors, failed built-ins, unknown commands, and failed launches are not recorded.
  `exit` itself is not recorded.
- A built-in run on its own in the foreground executes in the shell process, even
  with redirection. A built-in in a pipeline or background job runs in a child
  process, so `cd` or `exit` there does not affect the shell.
- Output files created with `>` have permissions `-rw-------`. Existing files are
  overwritten.
- `$MACHINE` is used for the prompt if it is set. Otherwise the shell uses the
  system host name.

## Extra credit

**Piping combined with I/O redirection (including in the background) is implemented.**
For example:

```text
cat < input.txt | sort | uniq > output.txt
cat < input.txt | sort | uniq > output.txt &
```

Each command in the pipeline connects its pipe descriptors first and then applies
its own `<` or `>` redirection. The integration test
`test_background_redirection_and_pipeline` covers this.
