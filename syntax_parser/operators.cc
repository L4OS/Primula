#include "namespace.h"

#if ! MODERN_COMPILER
#  include <stdlib.h>
#endif
statement_t * namespace_t::CheckOperator_RETURN(SourcePtr &source)
{
	return_t * statement = nullptr;
	if (source.lexem == lt_semicolon)
	{
		if (owner_function->function->bitsize != 0)
		{
			CreateError(source.line_number, -7777795, "non void function '%s' must return value", owner_function->function->name.c_str());
			return nullptr;
		}
		++source;
		if (source == true)
		{
			throw "Split lexem error on previous stage";
		}
		statement = new return_t;
	}
	else
	{
		if (source.lexem == lt_operator)
		{
			source++;

			expression_t * call_statement = (expression_t *)CheckOperator_OPERATOR(source);
			call_t * call = call_statement->root->call;

			expression_node_t * node = new expression_node_t(lt_call);
			node->call = call;

			expression_t * retval = new expression_t(node);

			statement = new return_t;
			statement->return_value = retval;
		}
		else
		{
			expression_t * expression = ParseExpression(source);
			if (expression == nullptr)
			{
				CreateError(source.line_number, -7777000, "Internal error on parsing expression");
				return nullptr;
			}
			if (source == false)
			{
				CreateError(source.line_number, -7777000, "semicolon expected after return expression" );
				return nullptr;
			}

			source.lexem = source.index->lexem;

			bool zero_rval = false;
			if (expression->is_constant && expression->type->signed_type)
			{
				expression_node_t * en = expression->root;
				if (en->is_constant && en->lexem == lt_integer)
				{
					constant_node_t * con = en->constant;
					zero_rval = con->integer_value == 0;
				}
			}
			compare_t compare = CompareTypes(owner_function->function->type, expression->type, false, zero_rval);
			switch (compare)
			{
			case same_types:	// types exactly match
				break;
			case cast_type:	// r-value type can be casted to l-value type
				fprintf(stderr, "Add casting to an expression\n");
				break;
			case no_cast:		// r-value type cannot be casted to l-value type
				CreateError(source.line_number, -7777000, "function return argument type '%s' does not match to function type '%s'", 
                    owner_function->function->type->name.c_str(), 
                    expression->type->name.c_str());
				source.Finish();
			}
			statement = new return_t;
			statement->return_value = expression;
		}
	}
	return statement;
}

statement_t * namespace_t::CreateStatement(SourcePtr &source, spacetype_t space_type)
{
	statement_t * result = nullptr;
	switch (source.lexem)
	{
	case lt_openblock:
	{
		codeblock_t	* block = new codeblock_t;
		block->block_space = CreateSpace(space_type, std::string(""));
#if MODERN_COMPILER
        for (auto statement : *source.statements)
			block->block_space->ParseStatement(statement);
#else
        Code::statement_list_t::iterator    statement;
        for( statement = source.statements->begin(); statement != source.statements->end(); ++statement)
            block->block_space->ParseStatement(*statement);
#endif
		result = block;
		source++;
		break;
	}
	case lt_openbraket:
	case lt_namescope:
	case lt_addrres:
	case lt_mul:
    case lt_inc:
    case lt_dec:
		result = ParseExpression(source);
		break;
	case lt_word:
		if (FindVariable(source.value) != nullptr || FindFunction(source.value) != nullptr)
		{
			result = ParseExpression(source);
			break;
		}
        CreateError(source.line_number, -7777795, "'%s' not defined ", source.value.c_str());
        source.Finish();
        break;

    default:
		result = CheckOperators(source);
		if (result == nullptr)
		{
			CreateError(source.line_number, -7777795, "lexem not parsed ");
			source.Finish();
		}
		break;
	}
	return result;
}



statement_t * namespace_t::CheckOperator_THROW(SourcePtr &source)
{
	operator_THROW * result = new operator_THROW;

    if (source != false)
    {
        expression_t * exp = ParseExpression(source);
        if (exp == nullptr || exp->root == nullptr)
        {
            CreateError(source.line_number, -7777795, "throw argument format error");
            source.Finish();
            return nullptr;
        }
        expression_node_t * root = exp->root;
        switch (root->lexem)
        {
        case lt_string:
        {
            result->exception_constant = new constant_node_t(root->constant->char_pointer);
            break;
        }
        default:
            throw "Unparsed 'throw' state";
        }
    }
    if (source == false || source.lexem != lt_semicolon)
	{
		CreateError(source.line_number, -7777286, "Expect semicolon after 'throw operand'");
	}
	return result;
}

statement_t * namespace_t::CheckOperator_TRY(SourcePtr &source)
{
	operator_TRY * result = new operator_TRY;
	do
	{
		result->body = CreateSpace(codeblock_space, std::string(""));
		result->body->Parse(*source.statements);
		source++;
		if (source == false || source.lexem != lt_catch)
		{
			CreateError(source.line_number, -7777795, "'try' operator not followed with 'catch'");
			continue;
		}
		source++;
		if (source == false || source.lexem != lt_openbraket)
		{
			CreateError(source.line_number, -7777795, "'catch' operator must followed with '('");
			continue;
		}

		linkage_t	linkage;
		function_parser  * handler = new function_parser(GetBuiltinType(lt_void), "");
		function_overload_parser * overload = new function_overload_parser(handler, &linkage);
		result->handler = overload;
		result->handler->space = CreateSpace(exception_handler_space, "operator");
		if (source.sequence->size() > 0)
		{
			overload->ParseArgunentDefinition(this, source.sequence);
		}
		if (result->handler->arguments.size() != 1)
		{
			CreateError(source.line_number, -7777795, "'catch' wrong argument count for exception handler");
			continue;
		}

		farg_t  arg = result->handler->arguments.front();
		result->type = arg.type;
		result->exception = arg.name;
		source++;
		if (source == false || source.lexem != lt_openblock)
		{
			CreateError(source.line_number, -7777795, "'catch' exception handler body expected" );
			continue;
		}
        result->handler->space->owner_function = result->handler;
		result->handler->space->Parse(*source.statements);
		source++;
		return result;
	} while (false);
	source.Finish();
	return nullptr;

}

statement_t * namespace_t::CheckOperator_IF(SourcePtr &source)
{
	operator_IF * result = new operator_IF;
	enum {
		expression_state,
		true_condition_state,
		check_else_state,
		else_condition_state,
		no_more_lexem_state
	} state = expression_state;

	while (source != false)
	{
		switch (state)
		{
		case expression_state:
			if (source.lexem != lt_openbraket)
			{
				CreateError(source.line_number, -777782, "condition absent in operator if" );
				source.Finish();
				return nullptr;
			}
			if (source.sequence == nullptr || source.sequence->size() == 0)
			{
				CreateError(source.line_number, -777782, "empty condition in operator if" );
				source.Finish();
				return nullptr;
			}
#if MODERN_COMPILER
            result->expression = ParseExpression(SourcePtr(source.sequence));
#else
            {
                SourcePtr ptr(source.sequence);
                result->expression = ParseExpression(ptr);
            }
#endif
			source++;
			state = true_condition_state;
			continue;

		case true_condition_state:
			result->true_statement = CreateStatement(source, codeblock_space);
			if (source != false && (source.lexem == lt_semicolon || source.lexem == lt_openblock))
				++source; // Check this later
			state = check_else_state;
			continue;

		case check_else_state:
			if (source.lexem == lt_else)
			{
				state = else_condition_state;
				source++;
				continue;
			}
			if (source.lexem == lt_semicolon)
			{
				source++;
				state = no_more_lexem_state;
				continue;
			}
			throw "namespace_t::CheckOperator_IF - wrong lexem at input";
			break;

		case else_condition_state:
			result->false_statement = CreateStatement(source, codeblock_space);
			if (source == true && source.lexem == lt_semicolon)
				++source;
			state = no_more_lexem_state;
			continue;

		case no_more_lexem_state:
			throw "Lexem after parsing in namespace_t::CheckOperator_IF";

		default:
			throw "Wrong state in namespace_t::CheckOperator_IF";
		}
	}
	return result;
}

statement_t * namespace_t::CheckOperator_DO(SourcePtr &source)
{
	operator_DO * result = new operator_DO;
	result->body = CreateStatement(source, continue_space);

    if (source == true && source.lexem == lt_semicolon)
        source++;
	if (source == false || source.lexem != lt_while)
	{
		CreateError(source.line_number, -777313, "expect 'while' expression");
		source.Finish();
		return nullptr;
	}
	source++;
	if (source.lexem != lt_openbraket)
	{
		CreateError(source.line_number, -777312, "while condition format error");
		source.Finish();
		return nullptr;
	}
	if (source.sequence == nullptr || source.sequence->size() == 0)
	{
		CreateError(source.line_number, -777782, "empty condition in operator if" );
		source.Finish();
		return nullptr;
	}
#if MODERN_COMPILER
    result->expression = ParseExpression(SourcePtr(source.sequence));
#else
    SourcePtr ptr(source.sequence);
    result->expression = ParseExpression(ptr);
#endif
	source++;

	return result;
}

statement_t * namespace_t::CheckOperator_WHILE(SourcePtr &source)
{
	if (source.lexem != lt_openbraket)
	{
		CreateError(source.line_number, -778312, "while(){} condition format error" );
		return nullptr;
	}

	operator_WHILE * result = new operator_WHILE;
#if MODERN_COMPILER
    result->expression = this->ParseExpression(SourcePtr(source.sequence));
#else
    SourcePtr ptr(source.sequence);
    result->expression = this->ParseExpression(ptr);
#endif
	source++;
	result->body = CreateStatement(source, continue_space);
	return result;
}

statement_t * namespace_t::CheckOperator_FOR(SourcePtr &source)
{
	if (source.lexem != lt_openbraket)
	{
		CreateError(source.line_number, -777202, "empty condition in operator 'for'" );
		source.Finish();
		return nullptr;
	}

	operator_FOR * result = new operator_FOR;
	result->body = CreateSpace(continue_space, std::string(""));

	//	__debugbreak();
	SourcePtr ptr(source.sequence);

	if (ptr != false && (ptr.lexem != lt_semicolon && ptr.lexem != lt_closebraket))
	{
		result->init = result->body->ParseExpressionExtended(ptr, &result->init_type, true);
	}
	if (ptr != false && ptr.lexem == lt_semicolon)
	{
		ptr++;
        if (ptr.lexem != lt_semicolon)
            result->condition = result->body->ParseExpression(ptr);
        else
        {
            result->condition = nullptr;
        }
	}
	if (ptr != false && ptr.lexem == lt_semicolon)
	{
		ptr++;
		result->iterator = result->body->ParseExpression(ptr);
	}
	source++;
	if (source.lexem != lt_openblock)
		result->body->ParseStatement(source);
	else
	{
#if MODERN_COMPILER
        for (auto statement : *source.statements)
		{
			result->body->ParseStatement(statement);
		}
#else
        Code::statement_list_t::iterator    statement;
        for (
            statement = source.statements->begin();
            statement != source.statements->end();
            ++statement)
        {
            result->body->ParseStatement(*statement);
        }
#endif
		source++;
	}
	//
	return result;
}

statement_t * namespace_t::CheckOperator_SWITCH(SourcePtr &source)
{
	enum {
		startup_state,
		switch_expression_state,
		finish_state
	} state = startup_state;

	operator_SWITCH * result = new operator_SWITCH;
	namespace_t * space;

	while (source != false)
	{
		switch (state)
		{
		case startup_state:
			if (source.lexem != lt_openbraket)
			{
				CreateError(source.line_number, -7777732, "switch open barcket expected");
				delete result;
				return nullptr;
			}
#if MODERN_COMPILER
            result->expression = ParseExpression(SourcePtr(source.sequence));
#else
            {
                SourcePtr ptr(source.sequence);
                result->expression = ParseExpression(ptr);
            }
#endif
			state = switch_expression_state;
			source++;
			continue;

		case switch_expression_state:
			if (source.lexem != lt_openblock)
			{
				CreateError(source.line_number, -7777733, "switch open block expected");
				source.Finish();
				delete result;
				return nullptr;
			}
			space = CreateSpace(switch_space, std::string(""));
			result->body = space;
			space->Parse(*source.statements);
			source++;
			state = finish_state;
			continue;

		case finish_state:
			CreateError(source.line_number, -7777733, "wrong terms after switch block");
			source.Finish();
			delete result;
			return nullptr;
			continue;
		}
	}
	return result;
}

namespace_t * namespace_t::findBreakableSpace(bool continues)
{
	namespace_t *	space = this;
	while (space)
	{
		if (space->type == continue_space || (space->type == switch_space) && !continues)
			break;
		space = space->parent;
	}
	return space;
}

namespace_t * namespace_t::findContinuableSpace()
{
	namespace_t *	space = this;
	while (space)
	{
		if (space->type == continue_space)
			break;
		space = space->parent;
	}
	return space;
}

statement_t * namespace_t::CheckOperator_BREAK(SourcePtr &source)
{
	if (source.lexem != lt_semicolon)
	{
		CreateError(source.line_number, -7777737, "wroing input after break operator" );
		return nullptr;
	}

	namespace_t	*	breakeable = findBreakableSpace(false);
	if (breakeable == nullptr)
	{
		CreateError(source.line_number, -7777737, "operator break allowe only witin loops and switches" );
		return nullptr;
	}
	source++;
	opeator_BREAK	*	statement = new opeator_BREAK;
	statement->break_space = breakeable;
	return statement;
}

char namespace_t::TranslateCharacter(SourcePtr source)
{
    char ch = 0;
    if (source.lexem != lt_character)
        throw "FSM error: translate character lexem is not character";
    if ( source.value.length() == 2 && source.value[0] == '\\')
        switch (source.value[1])
        {
        case '0':
            ch = '\0';
            break;
        case 'r':
            ch = '\r';
            break;
        case 'n':
            ch = '\n';
            break;
        case 't':
            ch = '\t';
            break;
        case '\\':
            ch = '\\';
            break;
        case '\'':
            ch = '\'';
            break;
        case '\"':
            ch = '\"';
            break;
        default:
            CreateError(source.line_number, -7776137, "Unparsed character sequence");
            break;
        }
    else
        ch = source.value[0];

    return ch;
}

statement_t * namespace_t::CheckOperator_CASE(SourcePtr &source)
{
	operator_CASE	*	result = nullptr;
    variable_base_t *   en = nullptr;
	int case_value = 0;

	do {
        if (source.lexem == lt_word)
        {
            en = TryEnumeration(source.value, false);
            if (en->static_data->type != lt_integer)
                throw "namespace_t::CheckOperator_CASE - enumeration must be integer type";
            case_value = en->static_data->s_int;
        }
        if (en == nullptr)
        {
            switch (source.lexem)
            {
            case lt_integer:
#if MODERN_COMPILER
                case_value = std::stoi(source.value);
#else
                case_value = atoi(source.value.c_str());
#endif
                break;
            case lt_character:
                case_value = TranslateCharacter(source);
                break;
            default:
                CreateError(source.line_number, -7777938, "non integer case label");
                source.Finish();
                continue;
            }
        }
		source++;
		if (source.lexem != lt_colon)
		{
			CreateError(source.line_number, -7777734, "case colon expected");
			source.Finish();
			continue;
		}
		result = new operator_CASE;
		result->case_value = case_value;
	} while (false);

	return result;
}

statement_t * namespace_t::CheckOperator_DEFAULT(SourcePtr &source)
{
	if (source.lexem != lt_colon)
	{
		CreateError(source.line_number, -7777737, "wrong input after 'default' operator. Expected colon.");
		return nullptr;
	}
	opeator_DEFAULT * result = new opeator_DEFAULT;
	return result;
}

statement_t * namespace_t::CheckOperator_CONTINUE(SourcePtr &source)
{
	if (source.lexem != lt_semicolon)
	{
		CreateError(source.line_number, -7777737, "wrong input after 'continue' operator - colon expected" );
		return nullptr;
	}

	namespace_t	*	space = findBreakableSpace(true);
	if (space == nullptr)
	{
		CreateError(source.line_number, -7777737, "operator 'continue' allowed only witin loops");
		return nullptr;
	}

	opeator_CONTINUE	*	statement = new opeator_CONTINUE;
	statement->continue_space = space;
	return statement;
}

statement_t * namespace_t::CheckOperator_GOTO(SourcePtr & source)
{
	if (source.index == source.eof || source.lexem != lt_word)
	{
		CreateError(source.line_number, -7777737, "operator 'goto' expected a label" );
		return nullptr;
	}
	operator_GOTO	*	statement = new operator_GOTO;
	statement->label = source.value;
	source++;
	if (source.index == source.eof || source.lexem != lt_semicolon)
	{
		CreateError(source.line_number, -7777737, "operator 'goto' label format error");
		return nullptr;
	}
	source++;
	return statement;
}

statement_t * namespace_t::CheckOperator_DELETE(SourcePtr & source)
{
	bool	array = false;
	do
	{
		if (source == false)
			continue;
		if (source.lexem == lt_openindex)
		{
			source++;
			array = true;
		}
		if (source == false || source.lexem != lt_word)
			continue;

		variable_base_t * var = FindVariable(source.value);
		if (var == nullptr)
			continue;
		source++;
		return new operator_DELETE(var, array);

	} while (false);

	CreateError(source.line_number, -7777737, "wrong form of operator delete");
	return nullptr;
}

statement_t * namespace_t::CheckOperator_OPERATOR(SourcePtr &source)
{
	if (source.index == source.eof)
	{
		CreateError(source.line_number, -7777737, "operator definition format error");
		return nullptr;
	}

	lexem_type_t  operator_lexem = source.lexem;
	switch (operator_lexem)
	{
	case lt_inc:
	case lt_new:
	case lt_delete:
		source++;
		if (source.lexem == lt_openbraket)
		{
			Code::lexem_list_t	*	sequence = source.sequence;
			;
			source++;
			if (source.lexem == lt_semicolon)
			{
				function_parser		* function = nullptr;
				switch (operator_lexem)
				{
				case lt_inc:
					function = FindFunction("operator::++");
					break;
				case lt_new:
					function = FindFunction("operator::new");
					break;
				case lt_delete:
					function = FindFunction("operator::delete");
					break;
				default:
					throw "Need to fix ASAP. It must be easy.";
				}
				expression_t * ex = nullptr;
				if (function != nullptr)
				{
					expression_node_t * node = new expression_node_t(lt_call);
#if false
					node->call = new call_t(function->name, function);
					ex = new expression_t(node);
					this->space_code.push_back(node->call);
#endif
					return ex;
				}
				CreateError(source.line_number, -7771232, "Function not found" );
				return nullptr;
			}
		}
		break;

	default:
		CreateError(source.line_number, -7773737, "operator 'operator' not defined" );
		return nullptr;
	}
	return nullptr;
}

function_overload_t * namespace_t::CreateOperator(type_t *type, std::string name, Code::lexem_list_t * sequence, linkage_t	* linkage)
{
	// TODO: Create operator
	return CreateFunction(type, name, sequence, linkage);
}

void namespace_t::CheckOverloadOperator(linkage_t * linkage, type_t * type, SourcePtr &overload)
{
	function_overload_t	*	operator_body = nullptr;
	SourcePtr arg_ptr = overload;
	arg_ptr++;
	if (arg_ptr.lexem != lt_openbraket)
	{
		this->CreateError(overload.line_number, -777780, "overload operator format error");
		overload.Finish();
		return;
	}
	switch (overload.lexem)
	{
	case lt_inc:
		if (((type_t*)type)->GetType() == lt_addrres)
			operator_body = CreateFunction(type, "operator ++", arg_ptr.sequence, linkage);
		else
			operator_body = CreateFunction(type, "_operator++", arg_ptr.sequence, linkage);
		break;
	case lt_new:
		operator_body = CreateFunction(type, "operator new", arg_ptr.sequence, linkage);
		break;
	case lt_delete:
		operator_body = CreateFunction(type, "operator delete", arg_ptr.sequence, linkage);
		break;
	case lt_set:
		operator_body = CreateOperator(type, "operator=", arg_ptr.sequence, linkage);
		break;
	case lt_word:
	{
		type_t * type = FindType(overload.value);
		if (type == nullptr)
		{
			CreateError(overload.line_number, -7771732, "syntax error ( '%s' is not a type)", overload.value.c_str());
			return;
		}
		overload++;
		if (overload.lexem != lt_openbraket)
		{
			CreateError(overload.line_number, -7771733, "expected open parentese" );
			return;
		}
		operator_body = CreateOperator(type, "operator_conversion_type", overload.sequence, linkage);
		break;
	}

	default:
		this->CreateError(overload.line_number, -177750, "Unable overlooad operator");
	}
	overload++;
	overload++;
	if (overload == false || overload.lexem != lt_openblock)
	{
		CreateError(overload.line_number, -7777704, "non-terminated operator definition");
		overload.Finish();
		return;
	}
	if (this->type == function_space)
	{
		CreateError(overload.line_number, -7777705, "function inside function not allowed");
		overload.Finish();
		return;
	}
	operator_body->Parse(overload.line_number, this, overload.statements);
	overload++;
	return;
}


statement_t * namespace_t::CheckOperators(SourcePtr &source)
{
	statement_t * result = nullptr;
	lexem_type_t lexem = source.lexem;
	source++;

	switch (lexem)
	{
	case lt_return:
		result = CheckOperator_RETURN(source);
		break;
	case lt_if:
		result = CheckOperator_IF(source);
		break;
	case lt_do:
		result = CheckOperator_DO(source);
		break;
	case lt_while:
		result = CheckOperator_WHILE(source);
		break;
	case lt_switch:
		result = CheckOperator_SWITCH(source);
		break;
	case lt_for:
		result = CheckOperator_FOR(source);
		break;
	case lt_break:
		result = CheckOperator_BREAK(source);
		break;
	case lt_case:
		result = CheckOperator_CASE(source);
		break;
	case lt_continue:
		result = CheckOperator_CONTINUE(source);
		break;
	case lt_default:
		result = CheckOperator_DEFAULT(source);
		break;
	case lt_goto:
		result = CheckOperator_GOTO(source);
		break;
	case lt_delete:
		result = CheckOperator_DELETE(source);
		break;
	case lt_try:
		result = CheckOperator_TRY(source);
		break;
	case lt_throw:
		result = CheckOperator_THROW(source);
		break;

	case lt_operator:
		result = CheckOperator_OPERATOR(source);
		break;

	default:
		throw "Fixme ASAP!";
		return (statement_t *)1;
	}

	return result;
}
