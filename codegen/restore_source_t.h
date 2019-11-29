#pragma once
#include "../syntax_parser/namespace.h"
class restore_source_t :
	public namespace_t
{
public:
	void GenerateCode(namespace_t * space);

	restore_source_t();
	virtual ~restore_source_t();
};

