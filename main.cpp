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

struct thread_data {
	HANDLE work_event;
	SOCKET client_socket;
};

// Thread entry
DWORD WINAPI winThreadProc(_In_ LPVOID lpParameter) {
	volatile thread_data* data = (volatile thread_data*)lpParameter;

	while (true) {
		// Wait for work to be assigned to this thread.
		WaitForSingleObject(data->work_event, INFINITE);
		
		SOCKET client_socket = data->client_socket;
		serveClient(client_socket);

		endClient(client_socket);
		std::cout << "Client disconnected." << std::endl;

		// We reset the event to signal that we finished our work.
		ResetEvent(data->work_event);
	}
	return 0;
}

int main() {
	HANDLE thread_pool[MAX_SIMULTANEOUS_CONNECTIONS];
	volatile thread_data* thread_data[MAX_SIMULTANEOUS_CONNECTIONS];

	// Setup the thread pool
	for (int i = 0; i < MAX_SIMULTANEOUS_CONNECTIONS; i++) {
		thread_data[i] = new struct thread_data;
		// The event will be used to alert the thread about new work.
		// We setup the event as manual reset: The thread will reset it to signal that it finished its work.
		thread_data[i]->work_event = CreateEvent(0, TRUE, FALSE, 0);
		if (!thread_data[i]->work_event) {
			std::cerr << "CreatEvent() failed: " << GetLastError() << std::endl;
			return 1;
		}
		thread_pool[i] = CreateThread(0, 0, winThreadProc, (LPVOID) thread_data[i], 0, 0);
	}

	SOCKET server_socket = createServer();

	try {
		while (true) {

			SOCKET client_socket = acceptClient(server_socket);
			std::cout << "Client connected." << std::endl;

			int available_thread = -1;
			// Check if any of the threads are available
			for (int i = 0; i < MAX_SIMULTANEOUS_CONNECTIONS; i++) {
				// A wait with a timeout of 0 is used to check the signal state of an event.
				// If the event is not signaled, the thread is not working.
				if (WaitForSingleObject(thread_data[i]->work_event, 0) == WAIT_TIMEOUT) {
					available_thread = i;
					break;
				}
			}

			if (available_thread == -1) {
				// We have too many simulatenuous connections, we respond with a 503 error code
				string resp = ResponseBuilder().setStatusCode(StatusCode::ServiceUnavailable).addFileBody(RESPONSE_503, false).build();
				if (send(client_socket, resp.c_str(), resp.length(), 0) == SOCKET_ERROR) {
					std::cout << "send() failed: " << WSAGetLastError() << std::endl;
				}

				std::cout << "\tFailed to respond to request, server overloaded. (Reached max thread count)" << std::endl;

				endClient(client_socket);
				std::cout << "Client disconnected." << std::endl;
			} else {
				// Pass the client socket to the thread
				thread_data[available_thread]->client_socket = client_socket;
				
				// According to MSDN, functions that signal sync object use appropriate memory barriers,
				// so no extra works needs to done to ensure client_socket is written before the thread
				// will be alerted.
				
				// Alert the thread about new work
				SetEvent(thread_data[available_thread]->work_event); 
				// The thread is responisble for closing the client socket when finished.
			}	
		}
	}
	catch (...) {

	}

	endServer(server_socket);

	return 0;
}