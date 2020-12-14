# Simple Multithreaded Webserver
A simple multithreaded [HTTP/1.0](https://tools.ietf.org/html/rfc1945) compliant toy webserver written in C++ for Windows.

## Usage
The server only serves static content, so currently POST requests result in a `501 Not Implemented` response.
The settings can be configured in `server_settings.h`:
- `LISTEN_PORT`: the port the server listens on.
- `SERVE_ROOT`: path to the root directory of the server.
- `MAX_SIMULTANEOUS_CONNECTIONS`: the number of threads in the thread pool.

## Layout
- `main.cpp` contains the main socket loop, and the basic thread pool implementation.
- `http_server.cpp` contains the actual HTTP server implementation.
- `sockets.cpp` abstracts Windows' socket stuff.

## TODO
- Support [HTTP/1.1](https://tools.ietf.org/html/rfc2616)
