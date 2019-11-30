#pragma once

#include "../include/type_t.h"
#include "../include/statement_t.h"
#include "../include/function_t.h"
#include "primula.h"


class function_parser;

class function_overload_parser : public function_overload_t
{
public:
/*
	std::string					mangle; // Without parent name
	function_parser				*	function;
	arg_list_t					arguments;
	class namespace_t		*	space;
	linkage_t					linkage;
	bool						read_only_members;

	Code::statement_list_t		*	source_code;	// For second pass
*/

	void MangleArguments();
	void ParseArgunentDefinition(namespace_t * parent_space, SourcePtr source);
	statement_t * CallParser(Code::lexem_list_t args_sequence);

	function_overload_parser(function_parser * parent, linkage_t * linkage)
	{
		function = parent;
		this->linkage = *linkage;
		space = nullptr;
		source_code = nullptr;
	}
};


class function_parser : public function_t// public type_t
{

public:
	/*
	std::string									name;
	type_t									*	type;
	function_overload_list_t					overload_list;
	enum { method, constructor, destructor }	method_type;
	*/

	function_parser(
		type_t			*	type,
		std::string			name) 
		: 
		function_t(type, name)
//			: type_t((char*)type->name.data(), funct_ptr_type, type->bitsize)
	{
		//this->name = name;
		//this->type = type;
		//method_type = method;
	}

	void RegisterFunctionOverload(function_overload_t * overload);
	void FindBestFunctionOverload(call_t * call);
	function_overload_t * FindOverload(call_t * call);
	call_t * TryCallFunction(namespace_t * space, SourcePtr & arg);
};
