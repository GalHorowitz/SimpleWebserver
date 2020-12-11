#include "status_code.h"

std::string httpReasonForCode(StatusCode statusCode) {
	switch (statusCode) {
	case StatusCode::OK:
		return "OK";
	case StatusCode::Created:
		return "Created";
	case StatusCode::Accepted:
		return "Accepted";
	case StatusCode::NoContent:
		return "No Content";
	case StatusCode::MovedPermanently:
		return "Moved Permanently";
	case StatusCode::MovedTemporarily:
		return "Moved Temporarily";
	case StatusCode::NotModified:
		return "Not Modified";
	case StatusCode::BadRequest:
		return "Bad Request";
	case StatusCode::Unauthorized:
		return "Unauthorized";
	case StatusCode::Forbidden:
		return "Forbidden";
	case StatusCode::NotFound:
		return "Not Found";
	case StatusCode::InternalServerError:
		return "Internal Server Error";
	case StatusCode::NotImplemented:
		return "Not Implemented";
	case StatusCode::BadGateway:
		return "Bad Gateway";
	case StatusCode::ServiceUnavailable:
		return "Service Unavailable";
	default:
		throw std::invalid_argument("Unknown status code");
	}
}