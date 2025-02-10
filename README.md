# ft_irc

## Introduction
**ft_irc** is a project at 42, designed to implement a basic Internet Relay Chat (IRC) server and client system. The purpose of this project is to understand networking, client-server architectures, and the protocols that power modern communication systems. By building a functional IRC server and a corresponding client, students gain practical knowledge in socket programming, multi-threading, and the IRC protocol.

The project consists of two main parts:
1. **IRC Server**: The server handles connections from multiple clients, manages channels, and allows users to send messages.
2. **IRC Client**: The client connects to the server, sends commands, and displays messages from the server.

## Objectives
- Implement a multi-client IRC server that can handle multiple simultaneous connections.
- Develop a client program that can interact with the server and send/receive messages.
- Support basic IRC commands, such as:
  - `/join` to join a channel
  - `/part` to leave a channel
  - `/nick` to change the username
  - `/msg` to send private messages
  - `/list` to list available channels
- Implement message broadcasting to all connected clients.
- Implement channel management and private messaging between clients.
- Ensure the server can handle disconnections and reconnects gracefully.
- Implement user authentication and validation.
- Work with sockets and multi-threading to ensure scalability and efficiency.

## Features
- **Multi-client server**: The server should be able to handle multiple client connections at the same time, using **threads** or **select** for scalability.
- **Channel management**: Users can create, join, and leave channels. Each channel has its own chat room, and messages are broadcast to users within a channel.
- **Private messaging**: Users can send private messages to other users.
- **Authentication**: Basic authentication features, like nicknames, are required. Nickname conflicts should be handled, and invalid nicknames should be rejected.
- **Message handling**: The server should properly handle incoming messages, including validation of commands and correct routing to channels or users.
- **User management**: Manage user connections, disconnects, and track active users.

## Requirements
- **C Programming**: The project is written in C and focuses heavily on networking concepts, such as **TCP/IP sockets** and **multi-threading**.
- **Unix/Linux Environment**: Development should be done in a Unix-like environment (Linux, macOS, etc.), with access to the standard C libraries and networking tools.
- **IRC Protocol**: The server should be built to follow the IRC protocol to ensure compatibility with existing IRC clients.

## Implementation Details
- **Server**: The server should handle multiple clients using **select()**, **epoll()**, or **pthread** (multi-threading) to manage each client connection efficiently.
- **Client**: The client should connect to the server, send commands, and display messages on the screen. The client should handle basic command-line arguments and implement the IRC protocol.
- **Commands**: The server and client should support a set of common IRC commands:
  - `/join [channel]` – join a specific channel.
  - `/part [channel]` – leave a channel.
  - `/msg [user] [message]` – send a private message to a user.
  - `/nick [new-nickname]` – change the current nickname.
  - `/list` – list all available channels.
- **Networking**: The server uses **TCP sockets** to communicate with clients, and the server should be able to handle multiple simultaneous connections using **select** or **epoll**.
