#pragma once

#include "../include/statement_t.h"

struct space_code_t
{
	std::list<type_t*>								space_types_list;
	std::list<variable_base_t*>						space_variables_list;
	std::list<statement_t *>						space_code;
	std::list<class function_parser*>				function_list;
};

struct space_t : public space_code_t
{
	std::map<std::string, type_t*>					space_types_map;
	std::map<std::string, variable_base_t*>			space_variables_map;
	std::map<std::string, class function_parser*>	function_map;
	std::map<std::string, int>						enum_map;
};