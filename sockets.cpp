#include "sockets.h"
#include <stdlib.h>
#include <stdio.h>

// Simple Windows TCP socket server written according to the MSDN docs

SOCKET createServer() {
	WSADATA wsaData;
	int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (startupResult != 0) {
		printf("WSAStartup() failed: %d\n", startupResult);
		exit(1);
	}

	ADDRINFO* addrResult = NULL;
	ADDRINFO hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO::IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int addrinfoResult = getaddrinfo(NULL, LISTEN_PORT, &hints, &addrResult);
	if (addrinfoResult != 0) {
		printf("getaddrinfo() failed: %d\n", addrinfoResult);
		WSACleanup();
		exit(1);
	}

	SOCKET listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("socket() failed: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		WSACleanup();
		exit(1);
	}

	DWORD timeout = 3 * 1000; // 3 seconds in ms
	setsockopt(listenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	if (bind(listenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen) == SOCKET_ERROR) {
		printf("bind() failed: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		exit(1);
	}

	freeaddrinfo(addrResult);

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("listen() failed: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		exit(1);
	}

	return listenSocket;
}

SOCKET acceptClient(SOCKET serverSocket) {
	SOCKET clientSocket = accept(serverSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		printf("accept() failed: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}
	return clientSocket;
}

void endClient(SOCKET clientSocket) {
	// Shutdown local send half of the connection
	if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
		printf("shutdown() failed: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		WSACleanup();
		exit(1);
	}
	closesocket(clientSocket);
}

void endServer(SOCKET serverSocket) {
	closesocket(serverSocket);
	WSACleanup();
}
