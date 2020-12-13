#include "http_server.h"

ResponseBuilder& ResponseBuilder::setStatusCode(StatusCode code) {
	statusCode = code;
	return *this;
}

ResponseBuilder& ResponseBuilder::addHeader(const string& name, const string& value) {
	headers.push_back(make_pair(name, value));
	return *this;
}

ResponseBuilder& ResponseBuilder::addBody(const string& contents) {
	if (hasBody) {
		throw std::runtime_error("Response already has body");
	}

	hasBody = true;
	body = contents;
	addHeader("Content-Length", std::to_string(contents.length()));
	return *this;
}

ResponseBuilder& ResponseBuilder::addFileBody(const string& filepath, bool fail_with_404) {
	std::ifstream served_file(filepath, std::ios::in | std::ios::binary);
	if (served_file) {
		// No need to actually read the file is this a head response
		if (!isHead) {
			// Read file contents
			string file_contents;
			served_file.seekg(0, std::ios::end);
			file_contents.resize(served_file.tellg());
			served_file.seekg(0, std::ios::beg);
			served_file.read(&file_contents[0], file_contents.size());
			served_file.close();
			addBody(file_contents);
		}
		addHeader("Content-Type", getMimeType(getFileExtension(filepath).c_str()));
	} else if (fail_with_404) {
		// Could not read file
		setStatusCode(StatusCode::NotFound);
		addFileBody(RESPONSE_404, false);
	} else {
		// TODO: We should handle client/server exceptions differently,
		// currently we will respond with a 400 if we can't find local response files.
		throw std::runtime_error("Failed to read file body");
	}

	return *this;
}

ResponseBuilder& ResponseBuilder::setHead() {
	isHead = true;
	return *this;
}

string ResponseBuilder::build() const {
	if (statusCode == StatusCode::Missing) {
		throw std::runtime_error("No status code set");
	}

	// Status line
	string response;
	response += "HTTP/";
	response += HTTP_VERSION;
	response += " ";
	response += std::to_string((int)statusCode);
	response += " ";
	response += httpReasonForCode(statusCode);
	response += "\r\n";

	// Headers
	for (auto header_pair : headers) {
		response += header_pair.first;
		response += ": ";
		response += header_pair.second;
		response += "\r\n";
	}

	// Headers end if marked with an empty line
	response += "\r\n";

	// Body
	if (!isHead && hasBody) {
		response += body;
	}

	return response;
}

string resolveRequestURI(const string& request_uri) {
	// Parse request uri
	if (request_uri[0] != '/')
		throw std::runtime_error("Missing / in the beggining of abs_path");

	string filepath = SERVE_ROOT;
	// We only need the path, we don't about the query parameters
	int query_start = request_uri.find("?");
	if (query_start != string::npos) {
		filepath += decodeEscapeSequences(request_uri.substr(0, query_start));
	} else {
		filepath += decodeEscapeSequences(request_uri);
	}

	// We resolve the relative path to an absolute path for two reasons:
	// - We want to turn the linux-style path into a windows-style path
	// - We need to make sure that the use of `..` path traversal did not cause the final path
	//   the end up outside of the serve root, which is not allowed.
	char resolved_path[256];
	if (_fullpath(resolved_path, filepath.c_str(), sizeof(resolved_path)) == NULL) {
		throw std::runtime_error("_fullpath() failed (File path too long?)");
	}
	char resolved_rootpath[256];
	if (_fullpath(resolved_rootpath, SERVE_ROOT, sizeof(resolved_rootpath)) == NULL) {
		throw std::runtime_error("_fullpath() failed (File path too long?)");
	}
	if (string(resolved_path).find(resolved_rootpath) != 0) {
		throw std::runtime_error("File path outside server root, possible path traversal");
	}

	// We initially assume that the final path is the resolved one, but it might be a directory
	// and in that case we should try serving the directory's index if one exists.
	string final_served_path = resolved_path;

	struct stat file_status;
	int stat_result = stat(resolved_path, &file_status);
	if (stat_result == 0 && (file_status.st_mode & S_IFDIR) != 0) {
		// Path is a directory, try serving local index.html
		if (final_served_path[final_served_path.length() - 1] != PATH_SEPERATOR) {
			final_served_path += PATH_SEPERATOR;
		}
		final_served_path += "index.html";
	}

	return final_served_path;
}

void serveClient(SOCKET client_socket) {
	string request;

	// Because of the format of HTTP requests, we can't know how long the request is (when including the body)
	// because its length is specified in the 'Content-Length' header. As such, we `recv()` data until we find
	// an empty new-line (CRLFCRLF) which marks the end of the headers. After we parse the headers, we may choose
	// to `recv()` the rest of the body if one exists and it is relevant to the response.

	char recv_buffer[1024 * 4];
	int end_of_headers = string::npos;
	do {
		int bytes = recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
		if (bytes == SOCKET_ERROR) {
			int err = WSAGetLastError();

			if (err == WSAETIMEDOUT) { // Timeout
				break;
			}

			std::cout << "recv() failed: " << err << std::endl;
			return;
		}

		request.append(recv_buffer, bytes);
		end_of_headers = request.find("\r\n\r\n") + 2; // The first CRLF is the ending of the last header.
	} while (end_of_headers == string::npos);

	// If we did not find the end-of-headers marker the request is either invalid or timed out.
	// In either case, we respond with a 400 and exit.
	if (end_of_headers == string::npos) {
		string resp = ResponseBuilder().setStatusCode(StatusCode::BadRequest).addFileBody(RESPONSE_400, false).build();
		if (send(client_socket, resp.c_str(), resp.length(), 0) == SOCKET_ERROR) {
			std::cout << "send() failed: " << WSAGetLastError() << std::endl;
			return;
		}

		std::cout << "\tTimeout: Did not find end of headers." << std::endl;
		return;
	}

	// Parse Request
	try {
		StringParser parser(string(recv_buffer, end_of_headers));

		// Request-Line: `METHOD Request-URI HTTP-Version CRLF`
		string method = parser.next_by_delim(" ");
		string request_uri = parser.next_by_delim(" ");
		string http_version = parser.next_by_delim("\r\n");
		std::cout << "Method: '" << method << "', Request URI: '" << request_uri << "', HTTP Version: '" << http_version << "'" << std::endl;


		// Methods are case-sensitive
		if (method != "GET" && method != "HEAD") {
			// We currently do not support POST requests.
			string resp = ResponseBuilder().setStatusCode(StatusCode::NotImplemented).build();
			if (send(client_socket, resp.c_str(), resp.length(), 0) == SOCKET_ERROR) {
				std::cout << "send() failed: " << WSAGetLastError() << std::endl;
				return;
			}
			return;
		}

		// Parse headers
		while (parser.has_data_left()) {
			string headerline = parser.next_by_delim("\r\n");

			int header_name_end = headerline.find(": ");
			if (header_name_end == string::npos) {
				throw std::runtime_error("Missing ': ' seperator in header line");
			}
			string header_name = headerline.substr(0, headerline.find(": "));
			// Header names are case-insensitive
			if (caseInsensitiveEquals(header_name, "Content-Length")) {
				// TODO: Receive the rest of the body
				// Is this relevant for GET?
				std::cout << "\tHas content!" << std::endl;
			}
		}

		// Parse request uri
		string served_path = resolveRequestURI(request_uri);

		std::cout << "\tResolved path: " << served_path << std::endl;

		// Serve file
		ResponseBuilder resp_builder = ResponseBuilder().setStatusCode(StatusCode::OK);
		if (method == "HEAD")
			resp_builder.setHead();
		// TODO: This current response model assumes we can load the file contents into memory and then send them.
		//       This means we cannot send very large files. A better approach could be to read the file in chunks
		//       and send them down the wire.
		string resp = resp_builder.addFileBody(served_path).build();
		if (send(client_socket, resp.c_str(), resp.length(), 0) == SOCKET_ERROR) {
			std::cout << "send() failed: " << WSAGetLastError() << std::endl;
			return;
		}
	}
	catch (const std::exception& e) {
		string resp = ResponseBuilder().setStatusCode(StatusCode::BadRequest).addFileBody(RESPONSE_400, false).build();
		if (send(client_socket, resp.c_str(), resp.length(), 0) == SOCKET_ERROR) {
			std::cout << "send() failed: " << WSAGetLastError() << std::endl;
			return;
		}

		std::cout << "\tFailed to parse request. Exception msg: " << e.what() << std::endl;
		return;
	}
}