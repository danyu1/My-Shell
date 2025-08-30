# myshell — A Minimal Unix‑like Shell (C)

A compact educational shell that implements a subset of common shell behavior in C. It supports interactive and batch modes, basic command execution via `execvp`, three built‑ins (`cd`, `pwd`, `exit`), a command separator (`;`), and two forms of stdout redirection: **basic** (`>`) and an **advanced merge** (`>+`).

> **Note:** The implementation deliberately favors simplicity and predictable grading behavior. Many advanced shell features are intentionally omitted.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Files & Layout](#files--layout)
3. [Build Instructions](#build-instructions)
4. [How It Works](#how-it-works)

   * [Interactive vs. Batch Mode](#interactive-vs-batch-mode)
   * [Parsing & Command Separation](#parsing--command-separation)
   * [Built‑ins](#built-ins)
   * [External Commands](#external-commands)
   * [Redirection (`>` and `>+`)](#redirection--and-)
   * [Error Handling](#error-handling)
   * [Input Size & Newline Rules](#input-size--newline-rules)
5. [Examples](#examples)
6. [Limitations & Non‑Goals](#limitations--non-goals)
7. [Testing Hints](#testing-hints)
8. [Security Notes](#security-notes)
9. [Script: `clean.sh`](#script-cleansh)
10. [Function‑by‑Function Map](#function-by-function-map)
11. [Troubleshooting](#troubleshooting)
12. [License](#license)

---

## Quick Start

```bash
# Build (with Makefile)
make

# or build manually
gcc -std=c11 -Wall -Wextra -O2 myshell.c -o myshell

# Run in interactive mode
./myshell

# Run in batch mode (reads commands from file, echoes each non-blank line)
./myshell commands.txt
```

When interactive, you’ll see the prompt:

```
myshell>
```

---

## Files & Layout

* **`myshell.c`** – Main shell implementation in C.
* **`Makefile`** – (If provided) Builds the `myshell` binary.
* **`clean.sh`** – Safety‑gated cleanup script for the `p2shell/test-scripts` directory.
* May coexist with grader assets (e.g., `grade.py`, `runOneTest.sh`) in `test-scripts/`.

> The shell prints **all errors** as the exact string: `An error has occurred` followed by a newline. This is written to **STDOUT** by design for deterministic grading.

---

## Build Instructions

### With Makefile

```bash
make            # builds ./myshell
make clean      # if defined in your Makefile
```

### Without Makefile

```bash
gcc -std=c11 -Wall -Wextra -O2 myshell.c -o myshell
```

---

## How It Works

### Interactive vs. Batch Mode

* **Interactive** (no argv): reads from `stdin` and prints the prompt `myshell> ` before each line.
* **Batch** (`./myshell file`): reads from the given file; **echoes each non‑whitespace line exactly as read** before executing it.
* Supplying **more than one** argument is an error.

### Parsing & Command Separation

* Each input line may contain **multiple commands** separated by semicolons `;`.
* Leading and trailing **spaces/tabs** around commands are trimmed.
* Arguments are split on spaces/tabs (no quoting or escaping support).
* Empty commands are ignored.

### Built‑ins

* **`exit`**

  * Usage: `exit`
  * No arguments and **no redirection** allowed. Otherwise, prints error.
  * On success, terminates the shell process.
* **`cd [dir]`**

  * At most **one** optional argument.
  * With no argument, changes to `$HOME`.
  * Redirection with `cd` is invalid.
* **`pwd`**

  * Prints the current working directory and a trailing newline.
  * Takes **no** arguments and disallows redirection.

### External Commands

* Executed via `fork()` + `execvp(args[0], args)` and waited with `waitpid`.
* On `execvp` failure (e.g., command not found), the child prints the standard error string and exits with status `1`.
* Environment `PATH` is honored by `execvp`.

### Redirection (`>` and `>+`)

The shell supports **stdout** redirection only (no `2>` or `>>`). Filenames must be a **single token** with **no spaces/tabs**.

* **Basic redirection**: `cmd ... > file`

  * Fails if `file` **already exists** (the shell refuses to overwrite).
  * Otherwise, creates/truncates `file` and directs **stdout** into it.
* **Advanced merge redirection**: `cmd ... >+ file`

  * Captures **stdout** of `cmd` into memory, then **writes it first** to `file`, followed by the **old contents** of `file` (if it existed).
  * Net effect: **prepends** the new output to the existing file content.
* Multiple `>` tokens on the same command are invalid.
* Redirection is rejected for built‑ins (`cd`, `pwd`, `exit`).

> **Note:** Only **stdout** is redirected. A command’s **stderr** still goes to the terminal.

### Error Handling

All detectable errors print exactly:

```
An error has occurred
```

…to **STDOUT**. Scenarios include (non‑exhaustive):

* Bad usage (too many program arguments).
* Batch file cannot be opened.
* Line too long (> 512 chars after trimming the trailing newline).
* Missing newline at end of an oversized input line (the shell consumes to newline and errors).
* Invalid redirection syntax (multiple `>`, space in filename, or redirection with built‑ins).
* `cd` or `pwd` misuse/OS errors (e.g., `chdir` fails).
* `fork`, `execvp`, `pipe`, or `open` failures.

### Input Size & Newline Rules

* After reading a line, if no newline is present (i.e., the input line is longer than the buffer), the shell **echoes** the partial line (batch mode), prints the error message, then discards characters until the next newline/EOF.
* After trimming the trailing `\n`, the shell **rejects** lines that exceed **512** characters.

---

## Examples

### Interactive basics

```text
myshell> pwd
/Users/alex/project
myshell> cd src
myshell> pwd
/Users/alex/project/src
myshell> exit
```

### Command separator

```text
myshell> echo one; echo two; echo three
one
two
three
```

### Basic redirection (create‑only)

```text
myshell> echo hello > out.txt   # creates out.txt with "hello\n"
myshell> echo again > out.txt   # ERROR: out.txt already exists
An error has occurred
```

### Advanced merge redirection (prepend behavior)

Suppose `log.txt` initially contains:

```
old line A
old line B
```

Running:

```text
myshell> printf "new1\nnew2\n" >+ log.txt
```

`log.txt` becomes:

```
new1
new2
old line A
old line B
```

### Batch mode echoing

Given `commands.txt`:

```
pwd
   echo hi   

# lines without non‑whitespace are not echoed
```

Run:

```bash
./myshell commands.txt
```

Output (echoed lines shown verbatim before their results):

```
pwd
/abs/path
   echo hi   
hi
```

---

## Limitations & Non‑Goals

* No quoting, escaping, or globbing.
* No pipelines (`|`), backgrounding (`&`), or job control.
* No input redirection (`<`) or append (`>>`).
* No variable expansion or shell scripting constructs.
* Only **stdout** redirection is supported and only to a **single** file per command.

These constraints keep the implementation compact and grading deterministic.

---

## Testing Hints

* Prefer small, isolated tests for each feature:

  * Built‑ins: `cd`, `pwd`, `exit` edge cases.
  * External command success/failure paths.
  * `>` vs `>+` behavior (file exists / doesn’t exist; large outputs).
  * Lines exactly 512 chars vs 513 chars.
  * Semicolon separation with empty segments: `; ; echo ok ;`.
* Validate that **stderr** from programs still appears on the terminal when redirecting stdout.

---

## Security Notes

* The shell uses `execvp`, inheriting the current environment and `PATH`. Be mindful when running untrusted commands.
* `advanced_redirection` buffers command output in memory; extremely large outputs may increase memory usage.

---

## Script: `clean.sh`

A defensive cleanup script meant to run **only** inside `p2shell/test-scripts`.

### What it does

* Verifies the current directory **path contains** `p2shell/test-scripts`. If not, prints an error and exits.
* Iterates through the directory and **deletes everything** except the allow‑list:

  * `myshell.c`, `grade.py`, `clean.sh`, `runOneTest.sh`, `myshell`, `Makefile`
* If it finds an `out/` directory, it removes it **recursively**.

### Usage

```bash
cd path/to/p2shell/test-scripts
bash clean.sh
```

> This guard reduces the chance of accidental deletion outside the expected test directory.

---

## Function‑by‑Function Map

* **`print_error()`** – Writes `An error has occurred\n` to STDOUT.
* **`trim_whitespace(char *s)`** – Trims leading/trailing spaces/tabs in‑place.
* **`has_non_whitespace(char *s)`** – Returns 1 if the string contains any non‑whitespace.
* **`contains_whitespace(char *s)`** – Returns 1 if the string contains any spaces/tabs.
* **`execute_command(char **args)`** – `fork`/`execvp` an external command; parent waits.
* **`basic_redirection(char **args, char *outfile)`** – Create‑only `>` redirection; refuses to overwrite.
* **`advanced_redirection(char **args, char *outfile)`** – `>+` redirection: capture stdout, then write new output followed by old file content (prepend effect).
* **`process_command(char *command)`** – Parses one command: handles built‑ins and redirection, argument tokenization, and validations.
* **`main(int argc, char *argv[])`** – Chooses interactive vs batch mode, reads lines, enforces size rules, splits by `;`, and dispatches each command via `process_command`.

---

## Troubleshooting

* **"An error has occurred" with `>`**: The destination file probably already exists, or the filename contains whitespace, or multiple `>` tokens were found.
* **`cd` fails**: Directory doesn’t exist or you passed too many args.
* **Long line errors**: Ensure each line (after removing its trailing newline) is **≤ 512** characters.
* **Nothing happens in batch mode**: Blank/whitespace‑only lines are **not echoed** or executed.

---

## License

Specify a license of your choice (e.g., MIT) if distributing this code.
