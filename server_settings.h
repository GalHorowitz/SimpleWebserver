#pragma once

#define SERVE_ROOT "./server_root/"
#define MAX_SIMULTANEOUS_CONNECTIONS 3 // This is essentially the number of threads in the pool

#define HTTP_VERSION "1.0"
#define SERVER_HEADER "SimpleWebserver/1.0"
#define RESPONSE_400 "./default_responses/400.html"
#define RESPONSE_404 "./default_responses/404.html"
#define RESPONSE_503 "./default_responses/503.html"
