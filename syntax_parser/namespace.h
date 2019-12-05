#pragma once

#include <string>
#include <list>
#include <map>
#include <stdarg.h>

#include "../include/lexem.h"
#include "lexem_tree_root.h"
#include "../include/type_t.h"
#include "../include/statement_t.h"
#include "../include/space_t.h"
#include "function.h"

#include <string>
#include "errors.h"
#include "primula.h"

class namespace_t : public space_t
{
//protected:
public:

    Errors                                          errors;
    function_overload_t                         *   owner_function;
    visibility_t                                    current_visibility;
    std::map<std::string, type_t*>                  space_types_map;
    std::map<std::string, variable_base_t*>         space_variables_map;
    std::map<std::string, class function_parser*>   function_map;
    std::map<std::string, int>                      enum_map;
    std::map<std::string, function_parser*>         template_function_map;
    std::map<std::string, type_t*>                  template_types_map;

    static type_t  builtin_types[18];
    bool no_parse_methods_body = false;
    std::map<std::string, type_t*>	instance_types;

public:
	namespace_t()
	{
		this->parent = nullptr;
		this->type = global_space;
	}

	void CreateError(int error, std::string description, int linenum, ...)
	{

		char buffer[256];
		va_list args;
		va_start(args, linenum);
		vsnprintf(buffer, 256, description.c_str(), args);
		va_end(args);
		
		fprintf(stderr, "%d: Error %d: %s\n", linenum, error, buffer);
		if (this != nullptr)
			errors.Add(error, buffer, linenum);
		else
			throw "Trying to log error into not declared namespace";
	}

	namespace_t * CreateSpace(spacetype_t type, std::string name)
	{
		namespace_t * space = new namespace_t(this, type, name);
		space->parent = this;
		return space;
	}

	void RegisterFunction(std::string name, function_parser * function, bool back);

	namespace_t * Parent() { return parent; }

	static type_t	*	GetBuiltinType(lexem_type_t type);
	type_t	*	TryLexenForType(SourcePtr & source);

	int ParseStatement(Code::lexem_list_t statement);
	int ParseStatement(SourcePtr &source);
	function_overload_t *  CheckCostructorDestructor(linkage_t * linkage, type_t * type, std::string name, SourcePtr &source);
	void CheckOverloadOperator(linkage_t * linkage, type_t * type, SourcePtr &overload);

	variable_base_t     *   TryEnumeration(std::string name, bool self_space);
    expression_node_t   *   TryEnumeration(std::string name);
    variable_base_t     *   FindVariable(std::string name);
	function_parser     *   FindFunctionInSpace(std::string name);
	function_parser     *   FindFunction(std::string name);
	function_parser     *   FindTemplateFunction(std::string name);
	type_t              *   FindType(std::string name);
	
	variable_base_t		* CreateVariable(type_t * type, std::string name, linkage_t	* linkage);
	function_overload_t	* CreateFunction(type_t *type, std::string name, Code::lexem_list_t * sequence, linkage_t	* linkage);
	type_t				* CreateType(type_t * type, std::string name);
	function_overload_t	* CreateOperator(type_t *type, std::string name, Code::lexem_list_t * sequence, linkage_t	* linkage);

	type_t			* ParseTypeDefinition(SourcePtr &source);
	type_t			* ParseCompoundDefinition(std::string parent_name, lexem_type_t kind, Code::statement_list_t *	statements);
	type_t			* ParseEnumeration(std::string parent_name, Code::statement_list_t *	statements);
	static_data_t	* BraceEncodedInitialization(type_t * type, SourcePtr & source);
    static_data_t   * BraceEncodedStructureInitialization(structure_t * structure, Code::statement_list_t * encoded_data);
    static_data_t   * ÑheckLexemeData(type_t * type, Code::lexem_node_t * node);
    static_data_t   * TryEnumsAndConstants(type_t * type, Code::lexem_node_t * node);

	namespace_t * findBreakableSpace(bool continues);
	namespace_t * findContinuableSpace();

	int namespace_t::CheckNamespace(SourcePtr &source);
	expression_t  * namespace_t::ParseExpression(SourcePtr &source);
	expression_t  * namespace_t::ParseExpressionExtended(SourcePtr &lexem, type_t ** ptype, bool hide);
	statement_t * namespace_t::CheckOperator_IF(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_RETURN(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_DO(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_WHILE(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_SWITCH(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_FOR(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_BREAK(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_CASE(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_CONTINUE(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_DEFAULT(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_OPERATOR(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_GOTO(SourcePtr & source);
	statement_t * namespace_t::CheckOperator_DELETE(SourcePtr & source);
	statement_t * namespace_t::CheckOperator_TRY(SourcePtr &source);
	statement_t * namespace_t::CheckOperator_THROW(SourcePtr &source);

	statement_t *  namespace_t::CheckOperators(SourcePtr &source);
	statement_t * CreateStatement(SourcePtr &source, spacetype_t soace_type);
	int namespace_t::Parse(Code::statement_list_t source);
	void namespace_t::ParseTemplate(SourcePtr & source);
	type_t * namespace_t::TryTranslateType(type_t * type);
	function_parser	* namespace_t::CreateFunctionFromTemplate(function_parser * func, SourcePtr & source);
	function_parser	* namespace_t::CreateFunctionInstance(function_parser * func, int line_number);
	variable_base_t * namespace_t::CreateObjectFromTemplate(type_t * type, linkage_t * linkage, SourcePtr & source);
	void namespace_t::TranslateNamespace(namespace_t * space, int line_num);
	variable_base_t *  namespace_t::TranslateVariable(namespace_t * space, variable_base_t * var);
	expression_t * namespace_t::TranslateExpression(namespace_t * space, expression_t * exp);
    char TranslateCharacter(SourcePtr source);

private:
	void namespace_t::CheckStorageClass(SourcePtr & source, linkage_t * linkage);
	void namespace_t::SelectStatement(type_t * type, linkage_t * linkage, std::string name, SourcePtr & source);
	type_t * namespace_t::TypeDefOpenBraket(SourcePtr & source, std::string & name, type_t * type);
	type_t * namespace_t::ParseIndex(SourcePtr & source, type_t * type);
	//inline template_t * namespace_t::CreateTemplateType(std::string type_name);

//	friend class function_overload_parser;
	friend struct function_overload_t;

	namespace_t(namespace_t * parent, spacetype_t type, std::string name)
	{
		this->parent = parent;
		this->type = type;
		this->name = name;
		owner_function = parent->owner_function;
	}

public:
};

