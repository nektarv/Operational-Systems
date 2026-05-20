# Overview

This exercise implements a TCP client in C that communicates with a remote server over sockets. The context is a quarantine movement permit system. The server acts as an authority that provides sensor data (temperature, light level) and approves or rejects movement permit requests.

## How It Works

The client connects to the server and enters an interactive loop. The user can issue commands from the terminal, and the client handles communication with the server transparently.

### Commands

| Command | Description |
|---|---|
| `help` | Print available commands |
| `exit` | Close the connection and exit |
| `get` | Fetch the latest sensor reading from the server |
| `N name surname reason` | Submit a movement permit request |

### `get` - Sensor Data

The server responds with the latest sensor event in the format:

```
X YYY ZZZZ WWWWWWWWWW
```

Where:
- `X` - event type: `0=boot, 1=setup, 2=interval, 3=button, 4=motion`
- `YYY` - light level
- `ZZZZ` - temperature × 100
- `WWWWWWWWWW` - Unix timestamp

Example output:
```
---------------------------
Latest event:
interval (2)
Temperature is: 29.50
Light level is: 58
Timestamp is: 2000-01-01 00:00:00
```

### `N name surname reason` - Movement Permit

A two-round-trip protocol:

1. Client sends the permit request (e.g. `1 john doe groceries`)
2. Server responds with a verification code (e.g. `5fdd09689ffe`), or `try again` if the request was malformed
3. Client displays the code and waits for the user to type it back
4. Client sends the code to the server
5. Server responds with `ACK ...` confirming the permit

## Build

```bash
gcc lab.c -o a
```

## Usage

```bash
./a [--ip IP] [--port PORT] [--debug]
```

| Flag | Description | Default |
|---|---|---|
| `--ip IP` | Server IP address | `147.102.75.201` |
| `--port PORT` | Server port | `4217` |
| `--debug` | Print sent/received messages | off |