# Type2C - Multiplayer Typing Test Game

A simple multiplayer typing test game implemented in C++ using socket programming. This project allows two players to connect to a server, type a given text, and compare their results (WPM and accuracy) in real-time.

---

## Features

- **Multiplayer Support**: Two players can connect to the server and play the typing test together.  
- **Real-Time Results**: Players receive their results (WPM and accuracy) after completing the test.  
- **Cross-Network Play**: The server binds to the local network IP, allowing clients on the same Wi-Fi or LAN to connect.  
- **Cross-Platform Compatibility**: Works on Windows and Linux systems.

---

## How It Works

### Server:

- Binds to the local IP address and listens for incoming connections.
- Once two players connect, it sends them a random text to type.
- Collects results from both players and broadcasts the final scores.

### Client:

- Connects to the server using its IP address.
- Receives the text to type, measures typing speed (WPM), and calculates accuracy.
- Sends the results back to the server and waits for the final scores.

---

## Requirements

- A C++ compiler (e.g., GCC or MinGW)
- Windows or Linux operating system
- Basic knowledge of networking (to provide the server's IP address)

---

## Setup Instructions

### 1. Compile the Server and Client

#### On Windows:

```bash
g++ -std=c++11 -static server.cpp -o server.exe -lws2_32
g++ -std=c++11 -static client.cpp -o client.exe -lws2_32
```

#### On Linux:

```bash
g++ -std=c++11 server.cpp -o server
g++ -std=c++11 client.cpp -o client
```

### 2. Run the Server

Start the server on one computer:

```bash
# Windows:
server.exe

# Linux:
./server
```

The server will display its local IP address (e.g., 192.168.x.x) and wait for clients to connect.

### 3. Run the Clients

Run `client.exe` on two different computers or terminals. When prompted, enter the server's IP address displayed in step 2:

```bash
# Example:
Enter the server's IP address: 192.168.x.x
```

Once both clients are connected, the game will start.

---

## Gameplay Instructions

- The server will send a random text to both players.
- Each player must type the text as quickly and accurately as possible.
- After typing, results (WPM and accuracy) are sent back to the server.
- The server broadcasts final results to both players.

---

## Example Output

### Server:

```
Server will bind to IP: 192.168.213.3
Server started on 192.168.213.3:8080

Client connected from: 192.168.213.5
Client connected from: 192.168.213.6
Starting game with 2 players...
Player 0 finished with WPM: 45, Accuracy: 98%
Player 1 finished with WPM: 50, Accuracy: 95%
Game finished, results sent to all players.
```

### Client:

```
Enter the server's IP address: 192.168.213.3
Connected to server at 192.168.213.3:8080
Waiting for another player to join...

=== Typing Test Started ===

Type the following text:
The quick brown fox jumps over the lazy dog.

Press Enter when ready, then type the text and press Enter when finished.

=== Your Results ===
Time: 12 seconds
WPM: 45
Accuracy: 98%

Waiting for other player to finish...

=== Final Results ===
Player 1:
  WPM: 45
  Accuracy: 98%

Player 2:
  WPM: 50
  Accuracy: 95%

Press Enter to quit...
```

---

## Troubleshooting

### Missing DLLs on Windows (e.g., `libgcc_s_seh-1.dll`)

If you encounter missing DLL errors when running `client.exe` or `server.exe`:

- Recompile with static linking (`-static` flag).
- Alternatively, copy required DLLs (`libgcc_s_seh-1.dll`, `libwinpthread-1.dll`, etc.) into the same folder as your `.exe`.

### Error Code 0xc000007b

This error is caused by architecture mismatches (32-bit vs 64-bit). Ensure that:

- Both your compiler and target system match in architecture.
- Recompile specifically for your friend's system architecture.

---

## Future Enhancements

- Add support for more than two players.
- Implement real-time progress updates during typing.
- Add a leaderboard system for competitive play.
- Allow custom texts or difficulty levels.

---

## License

This project is open-source and free to use for educational purposes.
