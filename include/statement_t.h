#pragma once

#include "lexem.h"
#include "type_t.h"
#include <string.h>

class reg_t
{
public:
    // To be union
    int				integer_value;
    int             offset;
    const char	*	char_pointer;
    float			float_value;

    reg_t()
    {
        integer_value = 0;
        float_value = 0;
        char_pointer = nullptr;
        offset = 0;
    }
};

class expression_node_t
{
public:
	type_t					*	type;
	bool						is_constant;
	lexem_type_t				lexem;
	expression_node_t		*	left;
	expression_node_t		*	right;
	class variable_base_t	*	variable;
	class  call_t			*	call;
    reg_t	        	    value;

	expression_node_t(lexem_type_t	lexem);
	expression_node_t(const expression_node_t * node);

#if true // Interpretaror
    expression_node_t()
    {
        lexem = lt_empty;
    }
#endif
    bool IsConstZero()
    {
        return ((lexem == lt_integer) & is_constant) && (value.integer_value == 0);
    }
};

class statement_t
{
public:
	enum types {
		_variable,
		_call,
		_expression,
		_return,
		_if,
		_do,
		_for,
		_while,
		_switch,
		_case,
		_default,
		_break,
		_continue,
		_goto,
		_label,
		_delete,
		_codeblock,
		_trycatch,
		_throw
	} type;

	statement_t(types t) { type = t; }
	virtual int debug() { return type; } // Better look in object inspector if defined
};

class variable_base_t : public statement_t
{
public:
    int                             access_count;
	std::string						name;
	type_t						*	type;
	class namespace_t			*	space;
	class statement_t			*	declaration;
#if COMPILER
	static_data_t				*	static_data;
#else
    reg_t	        	            value;

#if ! COMPILER
    char                        *   GetPayloadAddress(class run_call_t * context);
#endif

#endif
    variable_segment_t		        storage;
	bool							hide;

    variable_base_t(
        namespace_t		*	space,
        type_t			*	type,
        std::string			name,
        variable_segment_t storage);
};

class codeblock_t :public statement_t
{
public:
	namespace_t		*	block_space;

	codeblock_t() : statement_t(_codeblock) {}
};

class	expression_t : public statement_t
{
public:
	expression_node_t	*	root;
	bool					is_constant;
	const type_t		*	type;

	expression_t(expression_node_t * root) : statement_t(_expression)
	{
		this->root = root;
		is_constant = root->is_constant;
		type = root->type;
	}

    bool IsConstZero()
    {
        return root != nullptr && root->IsConstZero();
    }
};

class call_t : public statement_t
{
public:
	std::string							name;
	namespace_t						*	caller;
	struct function_overload_t		*	code;
	std::list<expression_t *>			arguments;
    int                                 argument_frame_size;

	call_t(namespace_t * caller_space, std::string name, struct function_overload_t * c) : statement_t(_call)
	{
		this->name = name;
		caller = caller_space;
		code = c;
        argument_frame_size = 0;
	}
};

class return_t : public statement_t
{
public:
	expression_t	*	return_value;
	return_t() : statement_t(_return) 
    {
        return_value = nullptr;
    }
};

class operator_LABEL : public statement_t
{
public:
	std::string		label;

	operator_LABEL(std::string label) : statement_t(_label) { this->label = label; }
};

class operator_GOTO : public statement_t
{
public:
	std::string		label;

	operator_GOTO() : statement_t(_goto) {}
};

class operator_DO : public statement_t
{
public:
	statement_t		*	body;
	expression_t	*	expression;

	operator_DO() : statement_t(_do) {}
};

class operator_FOR : public statement_t
{
public:
	type_t			*	init_type;
	expression_t	*	init;
	expression_t	*	condition;
	namespace_t		*	body;
	expression_t	*	iterator;

	operator_FOR() : statement_t(_for)
	{
		init_type = nullptr;
		init = nullptr;
		condition = nullptr;
		body = nullptr;
		iterator = nullptr;
	}
};

class operator_WHILE : public statement_t
{
public:
	expression_t	*	expression;
	statement_t		*	body;

	operator_WHILE() : statement_t(_while) {}
};

class operator_THROW : public statement_t
{
public:
#if GOOD
	variable_base_t		*	exception;
#else
	reg_t		        *	exception_constant;
#endif
	operator_THROW() : statement_t(_throw)
	{}
};

class operator_TRY : public statement_t
{
public:
	namespace_t					*	body;
	type_t						*	type;
	std::string						exception;
	struct function_overload_t	*	handler;

	operator_TRY() : statement_t(_trycatch)
	{
		body = nullptr;
		type = nullptr;
		handler = nullptr;
	}
};

class operator_IF : public statement_t
{
public:
	expression_t	*	expression;
	statement_t		*	true_statement;
	statement_t		*	false_statement;

	operator_IF() : statement_t(_if) { false_statement = nullptr; }
};

class operator_SWITCH : public statement_t
{
public:
	expression_t	*	expression;
	namespace_t		*	body;

	operator_SWITCH() : statement_t(_switch) {}
};

class operator_CASE : public statement_t
{
	namespace_t		*	break_space;
public:
	int				case_value;

	operator_CASE() : statement_t(_case) {}
};

class opeator_DEFAULT : public statement_t
{
	namespace_t		*	break_space;
public:
	opeator_DEFAULT() : statement_t(_default) {}
};

class opeator_BREAK : public statement_t
{
public:
	namespace_t		*	break_space;
	opeator_BREAK() : statement_t(_break) {}
};

class opeator_CONTINUE : public statement_t
{
public:
	namespace_t		*	continue_space;
	opeator_CONTINUE() : statement_t(_continue) {}
};

class operator_DELETE : public statement_t
{
public:
	variable_base_t		*	var;
	bool					as_array;

	operator_DELETE(variable_base_t * v, bool arr) : statement_t(_delete)
	{
		var = v;
		as_array = arr;
	}
};

