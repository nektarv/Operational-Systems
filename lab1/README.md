## Overview

This is the introductory exercise for the OS lab. A parent process creates one child process, and both write a message containing their PID and parent PID to a file specified by the user. The child writes first, then the parent waits for it to finish and writes second.

## How It Works

1. The program validates command-line arguments
2. It checks whether the output file already exists. If it does, it exits with an error
3. It creates the file with `open()`
4. It forks a child process
5. The child writes its message to the file
6. The parent waits for the child to finish with `wait()`, then writes its own message
7. The file ends up with two lines - child first, parent second

## Output File Format

```
[CHILD] getpid()=<child_pid>, getppid()=<parent_pid>
[PARENT] getpid()=<parent_pid>, getppid()=<shell_pid>
```

## Build

```bash
gcc lab1.c -o a
```

## Usage

```bash
./a <filename>
```

| Argument | Description |
|---|---|
| `filename` | Name of the output file to create |

```bash
./a output.txt
cat output.txt
```

Passing `--help` prints usage and exits with code 0:
```bash
./a --help
```