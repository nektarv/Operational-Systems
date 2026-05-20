## Overview

This exercise implements a parent-child process management system in C using signals. The parent spawns N child processes and acts as a signal relay - forwarding signals it receives to all its children. Each child maintains a counter that it increments or decrements based on the signals it receives, and periodically reports its status. If a child dies unexpectedly, the parent automatically replaces it.

## How It Works

The parent creates N children, prints a process tree, then enters an infinite `pause()` loop waiting for signals. All interesting behavior is driven by signal handlers.

The child code lives in a separate file (`child.c`) and is loaded via `execv()`, meaning each child is a completely separate program, not just a forked copy of the parent.

### Signals - Parent

| Signal | Behavior |
|---|---|
| `SIGUSR1` | Forward to all children |
| `SIGUSR2` | Forward to all children |
| `SIGTERM` | Forward `SIGTERM` to all children, then exit |
| `SIGCHLD` | Inspect which child changed state - resume it if stopped, recreate it if dead |

### Signals - Child

| Signal | Behavior |
|---|---|
| `SIGUSR1` | Increment internal counter `val` by 1 |
| `SIGUSR2` | Decrement internal counter `val` by 1 |
| `SIGTERM` | Print exit message and exit |
| `SIGALRM` | Print status report, reschedule next alarm in 10 seconds |

### Periodic Status Report

Each child calls `alarm(10)` on startup. Every 10 seconds `SIGALRM` fires, the child prints its index, total runtime, and current counter value, then reschedules the next alarm.

### Auto-restart

If a child dies for any reason (killed by a signal, called `exit()`, etc.), the OS sends `SIGCHLD` to the parent. The parent's handler calls `waitpid()` in a loop with `WNOHANG` to reap all finished children, identifies which one died by PID, and calls `create_child()` to fork and exec a fresh replacement at the same index.

The same handler also catches `SIGSTOP` on children (via `WUNTRACED`) and immediately sends `SIGCONT` to resume them.

## Build

```bash
gcc assignment2.c -o a
gcc child.c -o child.exe
```

Both binaries must be in the same directory. The parent locates the child executable via the hardcoded path `./child.exe`.

## Usage

```bash
./a <N>
```

| Argument | Description |
|---|---|
| `N` | Number of child processes to spawn (must be a positive integer) |

Example:
```bash
./a 3
```

```
Child 1 ID: 18421
Child 2 ID: 18422
Child 3 ID: 18423

Process Tree
    Parent : 18420
        Child 1 : 18421
        Child 2 : 18422
        Child 3 : 18423


Child 1:    Total time: 10s    Current val: 0
Child 2:    Total time: 10s    Current val: 0
Child 3:    Total time: 10s    Current val: 0
```