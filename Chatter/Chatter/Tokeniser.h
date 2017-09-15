#pragma once
#include <string>


struct Tokeniser
{
	Tokeniser(const std::string& s, const std::string& delms)
		: str(s)
		, delims(delms)
		, offset(0lu)
		, delim_pos(str.find_first_of(delims, offset))
		, has_next(true)
	{}

	bool HasNext() const { return has_next; }

	std::string NextToken()
	{
		auto token = str.substr(offset, delim_pos - offset);
		offset = delim_pos + 1;
		if (offset == 0)
			has_next = false;
		delim_pos = str.find_first_of(delims, offset);
		return token;
	}

private:
	const std::string str;
	std::string delims;
	size_t offset;
	size_t delim_pos;
	bool has_next;
};
