#pragma once

#include "../include/statement_t.h"

class farg_t : public variable_base_t
{
public:
	expression_t		*	default_value;
public:
	farg_t(
		namespace_t		*	space,
		type_t			*	type,
		std::string			name) 
			: variable_base_t(space, type, name, linkage_t::sc_argument)
	{
		default_value = nullptr;
	}
};

typedef std::list<farg_t>	arg_list_t;

struct function_overload_t
{
    int                         access_count;
	std::string					mangle;		// Without parent name
	class function_parser		*	function;
	arg_list_t					arguments;
	class namespace_t		*	space;
	linkage_t					linkage;
	bool						read_only_members;

	variable_base_t * FindArgument(std::string name);

	void Parse(int line_num, namespace_t * parent, Code::statement_list_t * source); // TODO: move to parser space
	Code::statement_list_t		*	source_code;	// For second pass. TODO: move to parser space

    function_overload_t()
    {
        access_count = 0;
    }
};

typedef std::list<function_overload_t *> function_overload_list_t;

struct function_t : public type_t
{
	std::string									name;
	type_t									*	type;
	function_overload_list_t					overload_list;
	enum { method, constructor, destructor }	method_type;

	function_t(
		type_t			*	type,
		std::string			name)
			: type_t(type->name.c_str(), funct_ptr_type, type->bitsize)
	{
		this->name = name;
		this->type = type;
		method_type = method;
	}
};
