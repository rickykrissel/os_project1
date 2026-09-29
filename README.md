Team Members: Ricky Krissel, Elias Elradi, Zechariah Zhong

Division of Labor:
Part 1: Prompt
- Ricky and Zechariah


Part 2: Environment Variables
- Ricky and Elias


Part 3: Tilde Expansion
- Elias and Zechariah


Part 4: $PATH Search
- Ricky and Elias


Part 5: External Command Execution
- Zechariah and Elias


Part 6: I/O Redirection
- Ricky and Zechariah


Part 7: Piping
- Zechariah and Elias


Part 8: Background Processing
- Ricky and Elias


Part 9: Internal Command Execution
- Ricky and Zechariah


Extra Credit
- Ricky, Elias, and Zechariah

## Use of AI
Used AI to set up general structure of files like the suggested one from the syllabus, fix errors, and clean up code

## File listing

- Makefile: builds the shell

- .gitignore: keeps the generated bin and obj directories out of Git.

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

```sh

# How to compile

On linprog, or any Linux system with `gcc` and `make`, run this from the
repository root:

```sh
make
./bin/shell
make            # builds bin/shell
./bin/shell     # runs the shell
make clean      # removes bin/ and obj/
```


## Development log
Zechariah - I added in  parts 7 8 9. I added pipelines supporting 2 pipes. Background jobs with IDs, PID reporting, and completion tracking. 
I also added the commands cd, jobs, and exit, and it includes history and wiaitng for background processess. 

Ricky - Set up github and files, completed parts 2 and 4, assisted with development on parts 8 and 9

Elias - Contributed to parts 1,3,5 and 6. 

## Description of Group Meetings:
Meeting 1: Divided labor

Meeting 2: Worked on steps 1-6

Meeting 3: Finished steps 4-6

Meeting 4: Finished steps 7-9 + extra credit

Meeting 5: Final code clean up and documentation


## Extra credit

Piping and I/O redirection can be combined, including in the background:
Piping combined with I/O redirection, including in the background is implemented.
For example:

```text
cat < input.txt | sort | uniq > output.txt
cat < input.txt | sort | uniq > output.txt &
```

Each command in the pipeline connects its pipe descriptors first and then applies
its own `<` or `>` redirection. The integration test
`test_background_redirection_and_pipeline` covers this.