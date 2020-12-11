#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <utility>
#include <iostream>
#include <fstream>
#include <list>

#include "server_settings.h"
#include "sockets.h"
#include "string_utils.h"
#include "http_server.h"

// Thread entry
DWORD WINAPI winThreadProc(_In_ LPVOID lpParameter) {
	SOCKET client_socket = (SOCKET)lpParameter;
	serveClient(client_socket);

	endClient(client_socket);
	std::cout << "Client disconnected." << std::endl;
	return 0;
}

int main() {
	std::list<HANDLE> thread_handles;

	SOCKET server_socket = createServer();

	try {
		while (true) {

			SOCKET client_socket = acceptClient(server_socket);
			std::cout << "Client connected." << std::endl;

			// Check if any of the threads finished their work, and if they did, remove them from the list
			auto it = thread_handles.begin();
			while (it != thread_handles.end()) {
				DWORD exit_code;
				GetExitCodeThread(*it, &exit_code);

				if (exit_code != STILL_ACTIVE) {
					it = thread_handles.erase(it);
				} else {
					it++;
				}
			}

			if (thread_handles.size() >= MAX_THREADS) {
				// We have too many simulatenuous connections, we respond with a 503 error code
				string resp = ResponseBuilder().setStatusCode(StatusCode::ServiceUnavailable).addFileBody(RESPONSE_503, false).build();
				if (send(client_socket, resp.c_str(), resp.length(), 0) == SOCKET_ERROR) {
					std::cout << "send() failed: " << WSAGetLastError() << std::endl;
				}

				std::cout << "\tFailed to respond to request, server overloade. (Reached max thread count)" << std::endl;

				endClient(client_socket);
				std::cout << "Client disconnected." << std::endl;
			} else {
				// Spawn a thread to handle the connection.
				// It is also responsible for closing the client socket when finished.
				thread_handles.push_back(CreateThread(0, 0, winThreadProc, (LPVOID)client_socket, 0, 0));
			}	
		}
	}
	catch (...) {

	}

	endServer(server_socket);

	return 0;
}