#pragma once
#include <string>
#include <stdexcept>

// HTTP/1.0 status codes
enum class StatusCode {
	Missing = -1,
	OK = 200,
	Created = 201,
	Accepted = 202,
	NoContent = 204,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	NotModified = 304,
	BadRequest = 400,
	Unauthorized = 401,
	Forbidden = 403,
	NotFound = 404,
	InternalServerError = 500,
	NotImplemented = 501,
	BadGateway = 502,
	ServiceUnavailable = 503
};

std::string httpReasonForCode(StatusCode statusCode);