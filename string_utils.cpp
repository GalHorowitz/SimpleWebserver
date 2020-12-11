#include <stdexcept>

#include "string_utils.h"

using std::string;

// Returns a substring starting at the end of the last part, ending at the first occurrence of the specified delimiter.
// Throws std::runtime_error if the delimiter cannot be found.
string StringParser::next_by_delim(const string& delim) {

	int part_end = data.find(delim, cur_offset);
	if (part_end == string::npos) {
		throw std::runtime_error("Failed to find next part by delimiter.");
	}

	string part = data.substr(cur_offset, part_end - cur_offset);

	cur_offset = part_end + delim.length();

	return part;
}

bool StringParser::has_data_left() const {
	return cur_offset < data.length();
}

bool caseInsensitiveEquals(const string& a, const string& b) {
	if (a.length() != b.length())
		return false;

	for (size_t i = 0; i < a.length(); i++) {
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	}

	return true;
}

// Hex char to int value. Throws if illegal char.
int hexNibble(char c) {
	if ('0' <= c && c <= '9') {
		return c - '0';
	} else if ('a' <= c && c <= 'f') {
		return 10 + c - 'a';
	} else if ('A' <= c && c <= 'F') {
		return 10 + c - 'A';
	} else {
		throw std::runtime_error("Illegal hex nibble");
	}
}

// Decodes URI "%HH" escape sequences. Throws if contains an illegal escape sequence.
string decodeEscapeSequences(const string& str) {
	// If the str doesn't contain escape seqences no work needs to be done
	if (str.find('%') == string::npos)
		return str;

	string decoded;
	// The actual length will be shorter, but most URIs don't contain many escape sequences,
	// so reserving enough memory for the entire string should be beneficial.
	decoded.reserve(str.length());

	for (size_t i = 0; i < str.length(); i++) {
		if (str[i] != '%') {
			decoded += str[i];
		} else if (i < str.length() - 2) {
			// Hex encoded ascii characters
			int decoded_char = hexNibble(str[i + 1]) * 16 + hexNibble(str[i + 2]);
			if (decoded_char >= 32 && decoded_char < 127) { // We allow printable characters only
				decoded += (char)decoded_char;
			}

			i += 2;
		} else {
			throw std::runtime_error("Invalid URI escape sequence");
		}
	}

	return decoded;
}

// Returns the file extension of the path, including the dot. e.g. `.txt`
// Returns an empty string if no extension was found.
string getFileExtension(const string& filepath) {
	int last_dot = filepath.find_last_of('.');
	if (last_dot == string::npos) {
		return "";
	} else {
		// TODO: Do we need to handle folders with dots in their name?
		// i.e. Should `some_dir/another.dir/file_with_no_ext` resolve to no extenstion
		return filepath.substr(last_dot);
	}
}