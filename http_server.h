#pragma once
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>

#include "server_settings.h"
#include "status_code.h"
#include "mime_types.h"
#include "string_utils.h"
#include "sockets.h"

using std::string;

// Builds an HTTP response
class ResponseBuilder {
	StatusCode statusCode;
	std::vector<std::pair<string, string>> headers;
	bool hasBody;
	string body;
	string bodyType;
	bool isHead;
public:
	ResponseBuilder() : statusCode(StatusCode::Missing), hasBody(false), isHead(false) {
		addHeader("Server", SERVER_HEADER);
	}

	ResponseBuilder& setStatusCode(StatusCode code);

	ResponseBuilder& addHeader(const string& name, const string& value);

	// Throws if the response already has a body
	ResponseBuilder& addBody(const string& contents);

	// Throws if the response already has a body.
	// By default sets status code and body to 404 response if file can not be read,
	// else if fail_with_404 is false throws std::runtime_error.
	ResponseBuilder& addFileBody(const string& filepath, bool fail_with_404 = true);

	ResponseBuilder& setHead();

	// Builds the response string. Throws if no status code was set.
	string build() const;
};

// Resolves a request uri to a local absolute path. Throws is the URI is invalid.
string resolveRequestURI(const string& request_uri);

// Serve an HTTP client
void serveClient(SOCKET client_socket);