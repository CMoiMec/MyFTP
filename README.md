# 📡 myFTP – FTP Server in C++ (RFC959-compliant)

> A custom FTP server written in C++, implementing the core specifications of the RFC959 protocol.  
> Designed to support multiple clients, standard FTP commands, and robust file transfer capabilities.

---

## 🚀 Project Overview

**myFTP** is a lightweight, multi-client FTP server developed in modern C++.  
It follows the [RFC959 specification](https://datatracker.ietf.org/doc/html/rfc959) to handle FTP sessions, client authentication, command parsing, and data transfer — all without relying on external FTP libraries.

The server handles standard commands such as `USER`, `PASS`, `LIST`, `RETR`, `STOR`, and supports both **active and passive modes**, simulating the behavior of real-world FTP servers.

---

## 🛠️ Key Features

- 📁 File listing and transfer (`LIST`, `RETR`, `STOR`)
- 🔐 User login support (`USER`, `PASS`)
- 🌐 Dual-channel architecture (control/data)
- ⚙️ Active (PORT) and Passive (PASV) mode support
- 👥 Concurrent multi-client handling (via threads or `select`)
- 🧩 Modular and extensible C++ design

---

## 📜 Protocol Compliance

This project strictly adheres to the **FTP protocol RFC959**, managing:

- Control connections over TCP (port 21 by default)
- Separate data connections for file transfers
- Proper response codes and syntax (e.g. `220`, `331`, `226`)
- Stateful session handling between commands

---

## 🧠 Educational Goals

- Learn and apply low-level socket programming in C++
- Understand FTP architecture, modes, and commands
- Handle concurrent connections and client state tracking
- Implement a standards-compliant network protocol from scratch

---

## ▶️ Getting Started

1. **Build the project**:
   ```bash
   make
   ```
2. **Launch the server**:
   ```bash
   ./myftp_server <port> <root_directory>
   ```
3. **Connect with an FTP client (like FileZilla, ftp, or curl)**:
   ```php-template
   ftp localhost <port>
   ```


---

## 🏫 Project Context

This project was developed as part of the systems and network programming module at [EPITECH](https://epitech.eu), focusing on practical implementation of network protocols in C++.


