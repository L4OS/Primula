#pragma once

#include <list>

class Errors
{
	char * filename;

public:
	
	typedef struct {
		int error;
		std::string description;
		char * filename;
		int line_number;
	} rexord_t;

	std::list<rexord_t> errors;

	void SetName(char * filename)
	{
		this->filename = filename;
	}

	void Add(int error, std::string description, int linenum )
	{
		rexord_t e;
		e.error = error;
		e.description = description;
		e.line_number = linenum;
		errors.push_back(e);
	}
};