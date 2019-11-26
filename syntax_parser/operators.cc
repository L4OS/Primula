#include "namespace.h"

statement_t * namespace_t::CheckOperator_RETURN(SourcePtr &source)
{
	return_t * statement = nullptr;
	if (source.lexem == lt_semicolon)
	{
		if (owner_function->function->bitsize != 0)
		{
			CreateError(-7777795, "non void function must return value", source.line_number);
			return nullptr;
		}
		++source;
		if (!source.IsFinished())
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
			SourcePtr term_ptr(source, lt_semicolon, true);
			statement = new return_t;
			statement->return_value = ParseExpression(term_ptr);
			if (statement->return_value == nullptr)
			{
				delete statement;
				return nullptr;
			}
			source.index = ++term_ptr.index;
			if (source != false)
				source.lexem = source.index->lexem;
			//ExpressionParser(SourcePtr(source, lt_semicolon, true));

			compare_t compare = CompareTypes(owner_function->function->type, statement->return_value->type, false);
			switch (compare)
			{
			case same_types:	// types exactly match
				break;
			case cast_type:	// type can be casted to l-value type
				printf("Add casting to an expression\n");
				break;
			case no_cast:		// type cannpt be casted to l-value
				CreateError(-7777000, "function return argument type does not match to function type", source.line_number);
				source.Finish();
			}
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
		for (auto statement : *source.statements)
			block->block_space->ParseStatement(statement);
		result = block;
		source++;
		break;
	}
	case lt_openbraket:
	case lt_namescope:
	case lt_addrres:
	case lt_mul:
		result = ParseExpression(source);
		break;
	case lt_word:
		if (FindVariable(source.value) != nullptr || FindFunction(source.value) != nullptr)
		{
			result = ParseExpression(source);
			break;
		}
	default:
		result = CheckOperators(source);
		if (result == nullptr)
		{
			CreateError(-7777795, "lexem not parsed ", source.line_number);
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
		switch (source.lexem)
		{
		case lt_string:
		{
			result->exception_constant = new constant_node_t(source.value.c_str());
			break;
		}
		default:
			throw "Unparsed 'throw' state";
		}
	source++;
	if (source == false || source.lexem != lt_semicolon)
	{
		errors.Add(-7777286, "Expect semicolon after 'throw operand'", source.line_number);
	}
	return result;
}

statement_t * namespace_t::CheckOperator_TRY(SourcePtr &source)
{
	operator_TRY * result = new operator_TRY;
	do
	{
		result->body = CreateSpace(spacetype_t::codeblock_space, std::string(""));
		result->body->Parse(*source.statements);
		source++;
		if (source == false || source.lexem != lt_catch)
		{
			CreateError(-7777795, "'try' operator not followed with 'catch'", source.line_number);
			continue;
		}
		source++;
		if (source == false || source.lexem != lt_openbraket)
		{
			CreateError(-7777795, "'catch' operator must followed with '('", source.line_number);
			continue;
		}

		linkage_t	linkage;
		function_t  * handler = new function_t(GetBuiltinType(lt_void), "");
		function_overload_t * overload = new function_overload_t(handler, &linkage);
		result->handler = overload;
		result->handler->space = CreateSpace(spacetype_t::exception_handler_space, "operator");
		if (source.sequence->size() > 0)
		{
			result->handler->ParseArgunentDefinition(this, source.sequence);
		}
		if (result->handler->arguments.size() != 1)
		{
			CreateError(-7777795, "'catch' wrong argument count for exception handler", source.line_number);
			continue;
		}

		farg_t  arg = result->handler->arguments.front();
		result->type = arg.type;
		result->exception = arg.name;
		source++;
		if (source == false || source.lexem != lt_openblock)
		{
			CreateError(-7777795, "'catch' exception handler body expected", source.line_number);
			continue;
		}
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
				CreateError(-777782, "condition absent for operator if", source.line_number);
				source.Finish();
				return nullptr;
			}
			if (source.sequence == nullptr || source.sequence->size() == 0)
			{
				CreateError(-777782, "empty condition in operator if", source.line_number);
				source.Finish();
				return nullptr;
			}
			result->expression = ParseExpression(SourcePtr(source.sequence));
			source++;
			state = true_condition_state;
			continue;

		case true_condition_state:
			result->true_statement = CreateStatement(source, spacetype_t::codeblock_space);
			if (source != false && source.lexem == lt_semicolon)
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
			result->false_statement = CreateStatement(source, spacetype_t::codeblock_space);
			if (source.lexem == lt_semicolon)
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
	result->body = CreateStatement(source, spacetype_t::continue_space);

	if (source.lexem != lt_while)
	{
		CreateError(-777313, "expect 'while' expression", source.line_number);
		source.Finish();
		return nullptr;
	}
	source++;
	if (source.lexem != lt_openbraket)
	{
		CreateError(-777312, "while condition format error", source.line_number);
		source.Finish();
		return nullptr;
	}
	if (source.sequence == nullptr || source.sequence->size() == 0)
	{
		CreateError(-777782, "empty condition in operator if", source.line_number);
		source.Finish();
		return nullptr;
	}
	result->expression = ParseExpression(SourcePtr(source.sequence));
	source++;

	return result;
}

statement_t * namespace_t::CheckOperator_WHILE(SourcePtr &source)
{
	if (source.lexem != lt_openbraket)
	{
		CreateError(-778312, "while(){} condition format error", source.line_number);
		return nullptr;
	}

	operator_WHILE * result = new operator_WHILE;
	result->expression = this->ParseExpression(SourcePtr(source.sequence));
	source++;
	result->body = CreateStatement(source, spacetype_t::continue_space);
	return result;
}

statement_t * namespace_t::CheckOperator_FOR(SourcePtr &source)
{
	if (source.lexem != lt_openbraket)
	{
		CreateError(-777202, "empty condition in operator 'for'", source.line_number);
		source.Finish();
		return nullptr;
	}

	operator_FOR * result = new operator_FOR;
	result->body = CreateSpace(codeblock_space, std::string(""));

	//	__debugbreak();
	SourcePtr ptr(source.sequence);

	if (ptr != false && (ptr.lexem != lt_semicolon && ptr.lexem != lt_closebraket))
	{
		result->init = result->body->ParseExpressionExtended(ptr, &result->init_type, true);
	}
	if (ptr != false && ptr.lexem == lt_semicolon)
	{
		ptr++;
		result->condition = result->body->ParseExpression(ptr);
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
		for (auto statement : *source.statements)
		{
			result->body->ParseStatement(statement);
		}
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
				CreateError(-7777732, "switch open barcket expected", source.line_number);
				delete result;
				return nullptr;
			}
			result->expression = ParseExpression(SourcePtr(source.sequence));
			state = switch_expression_state;
			source++;
			continue;

		case switch_expression_state:
			if (source.lexem != lt_openblock)
			{
				CreateError(-7777733, "switch open block expected", source.line_number);
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
			CreateError(-7777733, "wrong terms after switch block", source.line_number);
			source.Finish();
			delete result;
			return nullptr;
			continue;
		}
	}
	return result;
}

namespace_t * namespace_t::findBreakableSpace()
{
	namespace_t *	space = this;
	while (space)
	{
		if (space->type == continue_space || space->type == switch_space)
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
		CreateError(-7777737, "wroing input after break operator", source.line_number);
		return nullptr;
	}

	namespace_t	*	breakeable = findBreakableSpace();
	if (breakeable == nullptr)
	{
		CreateError(-7777737, "operator break allowe only witin loops and switches", source.line_number);
		return nullptr;
	}
	source++;
	opeator_BREAK	*	statement = new opeator_BREAK;
	statement->break_space = breakeable;
	return statement;
}

statement_t * namespace_t::CheckOperator_CASE(SourcePtr &source)
{
	operator_CASE	*	result = nullptr;
	int case_value = 0;

	do {
		if (source.lexem != lt_integer)
		{
			CreateError(-7777938, "non integer case label", source.line_number);
			source.Finish();
			continue;
		}
		case_value = std::stoi(source.value);
		source++;
		if (source.lexem != lt_colon)
		{
			CreateError(-7777734, "case colon expected", source.line_number);
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
		CreateError(-7777737, "wroing input after 'default' operator", source.line_number);
		return nullptr;
	}
	opeator_DEFAULT * result = new opeator_DEFAULT;
	return result;
}

statement_t * namespace_t::CheckOperator_CONTINUE(SourcePtr &source)
{
	if (source.lexem != lt_semicolon)
	{
		CreateError(-7777737, "wroing input after 'continue' operator", source.line_number);
		return nullptr;
	}

	namespace_t	*	space = findBreakableSpace();
	if (space == nullptr)
	{
		CreateError(-7777737, "operator 'continue' allowed only witin loops and switches", source.line_number);
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
		CreateError(-7777737, "operator 'goto' expected a label", source.line_number);
		return nullptr;
	}
	operator_GOTO	*	statement = new operator_GOTO;
	statement->label = source.value;
	source++;
	if (source.index == source.eof || source.lexem != lt_semicolon)
	{
		CreateError(-7777737, "operator 'goto' label format error", source.line_number);
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

	CreateError(-7777737, "wrong form of operator delete", source.line_number);
	return nullptr;
}

statement_t * namespace_t::CheckOperator_OPERATOR(SourcePtr &source)
{
	if (source.index == source.eof)
	{
		CreateError(-7777737, "operator 'continue' allowed only witin loops and switches", source.line_number);
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
				function_t		* function = nullptr;
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
				CreateError(-7771232, "Function not found", source.line_number);
				return nullptr;
			}
		}
		break;

	default:
		CreateError(-7773737, "operator 'operator' not defined", source.line_number);
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
		this->CreateError(-777780, "overload operator format error", overload.line_number);
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
			CreateError(-7771732, "syntax error (not a type)", overload.line_number);
			return;
		}
		overload++;
		if (overload.lexem != lt_openbraket)
		{
			CreateError(-7771733, "expected open parentese on ", overload.line_number);
			return;
		}
		operator_body = CreateOperator(type, "operator_conversion_type", overload.sequence, linkage);
		break;
	}

	default:
		this->CreateError(-177750, "Unable overlooad operator", overload.line_number);
	}
	overload++;
	overload++;
	if (overload.IsFinished() || overload.lexem != lt_openblock)
	{
		CreateError(-7777704, "non-terminated operator definition", overload.line_number);
		overload.Finish();
		return;
	}
	if (this->type == spacetype_t::function_space)
	{
		CreateError(-7777705, "function inside function not allowed", overload.line_number);
		overload.Finish();
		return;
	}
	operator_body->Parse(this, overload.statements);
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
