#  Custom Shell in C

This project is a **custom shell implementation in C** that supports standard shell commands, sequential and parallel execution, I/O redirection, and pipelines. It also handles signals (`Ctrl+C`, `Ctrl+Z`) gracefully and provides a basic interactive prompt.

---

## ✨ Features

* **Basic command execution**

  * Runs external commands like `ls`, `pwd`, `echo`, etc.
  * Built-in support for:

    * `cd` (change directory)
    * `exit` (terminate the shell)

* **Sequential execution (`##`)**

  * Commands separated by `##` run one after another.

  ```bash
  ls ## pwd ## date
  ```

* **Parallel execution (`&&`)**

  * Commands separated by `&&` run in parallel.

  ```bash
  ls && date && echo "Done"
  ```

* **Output redirection (`>`)**

  * Redirects standard output of a command to a file.

  ```bash
  ls > out.txt
  ```

* **Pipelines (`|`)**

  * Connects multiple commands with pipes.

  ```bash
  ls -l | grep ".c" | wc -l
  ```

* **Signal handling**

  * `Ctrl+C (SIGINT)` and `Ctrl+Z (SIGTSTP)` don’t kill the shell — they just refresh the prompt.

---

## 📂 Project Structure

```
.
├── myshell.c       # Main source code
└── README.md     # Documentation
```

---

## ⚙️ Compilation

Use `gcc` to compile:

```bash
gcc myshell.c -o myshell
```

---

## 🚀 Usage

Run the shell:

```bash
./myshell
```

You’ll see a prompt showing the current directory:

```
/home/user$
```

### Examples:

```bash
/home/user$ ls
/home/user$ cd Documents
/home/user/Documents$ ls && date
/home/user/Documents$ ls ## pwd
/home/user/Documents$ ls -l | grep ".txt"
```

---

## 🧩 Implementation Details

* **Parsing**

  * Custom `parseInput()` function to tokenize input by delimiters (`&&`, `##`, `>`, `|`).
* **Execution**

  * Uses `fork()`, `execvp()`, and `waitpid()` for process handling.
* **Parallel Execution**

  * Forks multiple processes and waits for all.
* **Sequential Execution**

  * Executes commands one after another.
* **Redirection**

  * Uses `open()`, `dup2()` to redirect standard output.
* **Pipelines**

  * Sets up multiple pipes and forks processes in a chain.
* **Signal Handling**

  * Catches `SIGINT` and `SIGTSTP` to refresh the prompt instead of terminating the shell.

---


