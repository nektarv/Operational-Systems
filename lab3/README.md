## Overview

This exercise implements a parent-child process communication system in C using pipes. The parent process spawns N child processes and distributes integer jobs to them. Each child doubles the number it receives, waits 5 seconds, and sends it back. The parent can distribute jobs either in round-robin order or randomly.

## How It Works

The parent creates N children, each with two pipes - one for sending jobs to the child, and one for receiving results back. The parent then enters an interactive loop, accepting commands from the terminal while simultaneously watching all child pipes for results.

### Commands

| Command | Description |
|---|---|
| `help` | Print usage hint |
| `exit` | Terminate all children and exit |
| Any integer | Send that number as a job to a child |

### Job Distribution Modes

**Round-robin** (default): jobs are assigned to children in order - child 1, child 2, ..., child N, child 1, ...

**Random**: each job is assigned to a randomly selected child.

### Child Behavior

Each child loops forever waiting for a job from the parent. When it receives one it:
1. Prints the received number
2. Doubles it
3. Sleeps 5 seconds
4. Sends the result back to the parent
5. Prints a completion message

The parent prints the result as soon as it arrives.

## Build

```bash
gcc lab3.c -o a
```

## Usage

```bash
./a <N> [--round-robin | --random]
```

| Argument | Description | Default |
|---|---|---|
| `N` | Number of child processes to spawn | required |
| `--round-robin` | Distribute jobs in round-robin order | default if omitted |
| `--random` | Distribute jobs randomly | |

Example session:
```
Child 1 was created
Child 2 was created
Child 3 was created

12
[Parent] Assigned 12 to child 1
[Child 1] [PID 8342]: Received 12!
7
[Parent] Assigned 7 to child 2
[Child 2] [PID 8343]: Received 7!
[Child 1] [PID 8342]: Finished hard work, writing back 24
[Parent] Child 1 returned 24
exit
Exiting
```