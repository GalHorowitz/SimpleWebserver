#pragma once

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

#define LISTEN_PORT "8080"

SOCKET createServer();
SOCKET acceptClient(SOCKET server_socket);
void endClient(SOCKET client_socket);
void endServer(SOCKET server_socket);