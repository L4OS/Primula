#pragma once

#include "type_t.h"

class variable_base_t : public statement_t
{
public:
	std::string						name;
	type_t						*	type;
	class namespace_t			*	space;
	class statement_t			*	declaration;
	static_data_t				*	static_data;
	linkage_t::storage_class_t		storage;
	bool							hide;

	variable_base_t(
		namespace_t		*	space,
		type_t			*	type,
		std::string			name,
		linkage_t::storage_class_t storage) 
		: statement_t(statement_t::_variable)
	{
		this->space = space;
		this->type = type;
		this->name = name;
		this->storage = storage;
		declaration = nullptr;
		static_data = nullptr;
		hide = false; // This is useful for reconstruction to source code of "for" loop
	}
};

