#pragma once

#include <string>

#ifdef _WIN32
#define PATH_SEPERATOR '\\'
#else
#define PATH_SEPERATOR '/'
#endif

class StringParser {
	std::string data;
	size_t cur_offset;
public:
	StringParser(std::string data) : data(data), cur_offset(0) {}

	// Returns a substring starting at the end of the last part, ending at the first occurrence of the specified delimiter.
	// Throws std::runtime_error if the delimiter cannot be found.
	std::string next_by_delim(const std::string& delim);

	bool has_data_left() const;
};

bool caseInsensitiveEquals(const std::string& a, const std::string& b);

// Hex char to int value. Throws if illegal char.
int hexNibble(char c);

// Decodes URI "%HH" escape sequences. Throws if contains an illegal escape sequence.
std::string decodeEscapeSequences(const std::string& str);

// Returns the file extension of the path, including the dot. e.g. `.txt`
// Returns an empty string if no extension was found.
std::string getFileExtension(const std::string& filepath);