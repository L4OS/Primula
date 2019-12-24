#include "namespace.h"
#include "lexem_tree_root.h"

#include <stack>
#include <string>

#if ! MODERN_COMPILER
#include <stdlib.h>
#endif

extern int GetLexemPriority(lexem_type_t  lex);

class ExpressionParser
{
private:
	enum {
		default_state,
		wait_name_delimiter,
		dot_state,
		find_function_state
	} state;

	std::deque<shunting_yard_t>	operands;
	std::deque<shunting_yard_t> operators;
	bool prev_was_operand;

    namespace_t		    *	space_state;        // = nullptr;
    variable_base_t	    *	variable_state;     // = nullptr;
    function_parser		*	function_state;     // = nullptr;

	variable_base_t		*	FindVariable(shunting_yard_t * pYard);
	type_t *				CreateType(shunting_yard_t * yard);
	expression_node_t	*	CreateNode(shunting_yard_t * yard, shunting_yard_t * parent, expression_node_t * left);
	constant_node_t		*	CreateConstant(shunting_yard_t * yard);
	int						Parse(SourcePtr &source);
	void					FixExpression(shunting_yard_t shunt, int prio);
	void					PrepareArguments(call_t * call, expression_node_t * arg);

    expression_node_t   *   CheckWord(
        namespace_t             *   space,
        std::string                 name,
        bool                        no_parent_check,
        expression_node_t       *   rvalue,
        int                         line_num);
    expression_node_t	*	CreateWordMode(std::string word, shunting_yard_t * parent, expression_node_t * left, expression_node_t	*	rvalue, int line_num);
    void ParseArguments(SourcePtr * node, shunting_yard_t * parent);

public:
	lexem_type_t		ParseExpression(SourcePtr &source);
	shunting_yard_t *	PrepareExpression(void);
	expression_t	*	BuildExpression(void);
	void				ParseOperator_NEW(SourcePtr &source);
	void				ParseOperator_SIZEOF(SourcePtr &source);
	lexem_type_t		ParseIncrementDecrement(SourcePtr &node);
	void				ParseOpenBracket(SourcePtr &node);

	ExpressionParser(namespace_t * current_space)
	{
		state = default_state;
		space_state = current_space;
		prev_was_operand = false;
        variable_state = nullptr;
        function_state = nullptr;
	}
};

int ExpressionParser::Parse(SourcePtr &source)
{
	int prio = GetLexemPriority(source.lexem);

	shunting_yard_t	shunt(source);
    switch (shunt.lexem)
    {
    case lt_semicolon:
        fprintf(stderr, "Got semicolon in expression\n");
        break;

    case lt_openindex:
    {
        ExpressionParser	index_parser(space_state);
#if MODERN_COMPILER
        index_parser.ParseExpression(SourcePtr(source.sequence));
#else
        SourcePtr ptr(source.sequence);
        index_parser.ParseExpression(ptr);
        index_parser.PrepareExpression();
#endif
        FixExpression(shunt, prio);
        shunting_yard_t * index_exp = new shunting_yard_t(*index_parser.PrepareExpression());
        operands.push_back(*index_exp);
        prev_was_operand = true;
        break;
    }

    case lt_openbraket:
        if (prev_was_operand)
            throw "We got function() call; Expression state machine error";
        operators.push_back(shunt);
        break;

    case lt_call_method:
        operands.back().lexem = shunt.lexem;
        break;

    case lt_call:
        operands.back().lexem = shunt.lexem;
        break;

    case lt_mul:
        if (!prev_was_operand)
        {
            shunt.lexem = lt_indirect;
            shunt.unary = true;
            prio = 30;
        }
        FixExpression(shunt, prio);
        break;

    case lt_and:
        if (!prev_was_operand)
        {
            shunt.lexem = lt_addrres;
            shunt.unary = true;
            prio = 30;
        }
        FixExpression(shunt, prio);
        break;

    case lt_closebraket:
        throw "FSM error: lt_closebracker was reduced on lexem splitting stage";

    default:
        if (prio >= 1000)
        {
            operands.push_back(shunt);
            prev_was_operand = true;
        }
        else
        {
            FixExpression(shunt, prio);
        }
    }

	return 0;
}

void ExpressionParser::FixExpression(shunting_yard_t shunt, int prio)
{
	if (!prev_was_operand)
	{
		// пока так
		if (shunt.lexem == lt_inc || shunt.lexem == lt_dec)
			//
			shunt.unary = true;
	}
	if (operators.size() != 0)
	{
		shunting_yard_t	op = operators.back();
		if (op.lexem == lt_namescope)
		{
            if (operands.size() != 0)
            {
                shunting_yard_t  name = operands.back();
                operands.pop_back();
                op.right = new shunting_yard_t(name);
            }
            op.left = new shunting_yard_t(shunt);
            operators.pop_back();
			operands.push_back(op);
			//shunt = op;
		}
		else
			while (GetLexemPriority(op.lexem) <= prio)
			{
				operators.pop_back();
				if (operands.size() > 0)
				{
					op.right = new shunting_yard_t(operands.back());
					operands.pop_back();
					if (!op.unary)
					{
						op.left = new shunting_yard_t(operands.back());
						operands.pop_back();
					}
				}
				else
				{
					if (op.lexem == lt_namescope)
					{
						// Do notning
					}
                    else if (op.lexem == lt_typecasting)
                    {
                        // Got it on casting unary minused integer to enumerated type;
                        operators.push_back(op);
                        break;
                    }
					else
					{
						throw "Unary operation without precedence";
					}
				}
				operands.push_back(op);
				if (operators.size() == 0)
					break;
				op = operators.back();
			}
	}
	operators.push_back(shunt);
	prev_was_operand = false;
}

variable_base_t		*	ExpressionParser::FindVariable(shunting_yard_t * pYard)
{
	variable_base_t		*	var = nullptr;
	switch (pYard->lexem)
	{
    case lt_this:
        fprintf(stderr, "debug\n");
        break;
	case lt_word:
		var = this->space_state->FindVariable(pYard->text);
		break;
    case lt_openindex:
        var = this->space_state->FindVariable(pYard->left->text);
        break;
	default:
		throw "Parser stuck at ExpressionParser::FindVariable(shunting_yard_t * pYard)";
		break;
	}
	return var;
}

type_t * ExpressionParser::CreateType(shunting_yard_t * yard)
{
	if (yard->lexem == lt_pointer)
		return new pointer_t(CreateType(yard->left));

	type_t *	type = space_state->GetBuiltinType(yard->lexem);
	if (type != nullptr)
		return type;

	if (yard->lexem != lt_word)
		return nullptr;

	return this->space_state->FindType(yard->text);
}

constant_node_t * ExpressionParser::CreateConstant(shunting_yard_t * yard)
{
	constant_node_t * constant = nullptr;
	switch (yard->lexem)
	{
	case lt_character:
    {
        char ch = yard->text[0];
        if (ch == '\\')
        switch (yard->text[1])
        {
        case '0':
            ch = '\0';
            break;
        case '\\':
        case '\'':
        case '\"':
            ch = yard->text[1];
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
        default:
            throw "Character sequence not parsed";
        }
        constant = new constant_node_t(ch);
        break;
    }

	case lt_integer:
#if MODERN_COMPILER
        if (yard->text.rfind("0x", 0) == 0)
            constant = new constant_node_t(std::stoi(yard->text, nullptr, 16));
        else
            constant = new constant_node_t(std::stoi(yard->text));
#else
        if (yard->text.rfind("0x", 0) == 0)
            constant = new constant_node_t((int)strtol(yard->text.c_str(), nullptr, 16));
        else
            constant = new constant_node_t(atoi(yard->text.c_str()));
#endif
		break;

	case lt_string:
		constant = new constant_node_t(yard->text.c_str());
		break;

	case lt_floatnumber:
#if MODERN_COMPILER
        constant = new constant_node_t(std::stof(yard->text));
#else
        constant = new constant_node_t((float) atof(yard->text.c_str()));
#endif
		break;

	default:
		throw "Constant not parsed";
	}

	return constant;
}

void ExpressionParser::PrepareArguments(call_t * call, expression_node_t * arg)
{
	while (arg != nullptr)
	{
		expression_t	* arg_expr = new expression_t(arg->left);
		call->arguments.push_back(arg_expr);
		arg = arg->right;
	}
}

expression_node_t * ExpressionParser::CheckWord(
    namespace_t         *   space,
    std::string             name,
    bool                    no_parent_check,
    expression_node_t   *   rvalue,
    int                     line_num)
{
    expression_node_t * node = nullptr;
    do
    {
        function_parser * fptr = no_parent_check ? space->FindFunctionInSpace(name) : space->FindFunction(name);
        if (fptr != nullptr)
        {
            call_t * call = new call_t(space, name, nullptr);
            PrepareArguments(call, rvalue);
            function_overload_t * overload = fptr->FindOverload(call);

            node = new expression_node_t(lt_call_method);
            node->call = call;
            node->type = fptr->type;
            break;
        }

        variable_base_t * var = no_parent_check ? space->FindVariableInspace(name) : space->FindVariable(name);
        if (var != nullptr)
        {
            node = new expression_node_t(lt_variable);
            node->variable = var;
            node->type = var->type;
            break;
        }
        var = space->TryEnumeration(name, no_parent_check);
        if (var != nullptr)
        {
            node = new expression_node_t(lt_integer);
            node->type = space->GetBuiltinType(lt_type_int);
            node->is_constant = true;
            node->constant = new constant_node_t(333);
            break;
        }
        space->CreateError(line_num, -77716721, "unparsed statement '%s' ", name);
    }
    while (false);

    return node;
}

expression_node_t	*	ExpressionParser::CreateWordMode(std::string word, shunting_yard_t * parent, expression_node_t * left, expression_node_t	*	rvalue, int line_num)
{
    expression_node_t	*	node = nullptr;
    variable_base_t     *   var = nullptr;

    do 
    {
        // Check varible is a field of structured type
        if ((parent != nullptr))
        {
            if ((parent->lexem == lt_dot || parent->lexem == lt_point_to))
            {
                if (left == nullptr)
                {
                    if (parent->left == nullptr)
                        throw "Expression StateMachineError";
                    var = FindVariable(parent->left);
                    if (var == nullptr)
                    {
                        this->space_state->CreateError(line_num, -77716721, "variable '%s' not found", parent->left->text.c_str());
                        break;
                    }
                    type_t * vartyp = var->type;
                    if ((parent->lexem == lt_point_to) & (vartyp->prop == type_t::pointer_type))
                    {
                        vartyp = ((pointer_t*)vartyp)->parent_type;
                    }
                    while (vartyp->prop == type_t::typedef_type)
                    {
                        vartyp = ((typedef_t*)vartyp)->type;
                    }
                    if (vartyp->prop != type_t::compound_type)
                    {
                        this->space_state->CreateError(line_num, -77716721, "variable '%s' is not compound", var->name.c_str());
                        break;
                    }
                    node = new expression_node_t(lt_variable);
                    node->variable = var;
                    node->type = var->type;
                    break;
                }
                if (left->lexem == lt_this)
                {
                    namespace_t * self = this->space_state;
                    while (self && self->type != namespace_t::spacetype_t::structure_space)
                    {
                        self = self->parent;
                    }
                    if (self == nullptr)
                    {
                        this->space_state->CreateError(line_num, -77716623, "operator 'this' is allowed within  methods only", var->name.c_str());
                        break;
                    }

                    node = CheckWord(self, word, true, rvalue, line_num);
                    break;
                }

                if (left->lexem == lt_openindex)
                {
                    if (left->type->prop != type_t::compound_type)
                    {
                        space_state->CreateError(line_num, -77716721, "unparsed statement '%s' ", word.c_str());
                        break;
                    }
                    typedef structure_t * pstructure_t;
                    namespace_t * space = pstructure_t(left->type)->space;
                    node = CheckWord(space, word, true, rvalue, line_num);
                    break;
                }

                if (left->lexem == lt_variable)
                {
                    type_t * type = left->variable->type;
                    while (type->prop == type_t::typedef_type)
                        type = ((typedef_t*)type)->type;
                    if (parent->lexem == lt_point_to)
                    {
                        if (type->prop != type_t::pointer_type)
                        {
                            space_state->CreateError(line_num, -77716721, "non-pointer type '%s' ", left->variable->name.c_str());
                            break;
                        }
                        type = ((pointer_t*)type)->parent_type;
                    }
                    while (type->prop == type_t::typedef_type)
                        type = ((typedef_t*)type)->type;
                    if (type->prop != type_t::compound_type)
                    {
                        space_state->CreateError(line_num, -77716721, "unparsed statement '%s' ", word.c_str());
                        break;
                    }
                    typedef structure_t * pstructure_t;
                    namespace_t * space = pstructure_t(type)->space;
                    node = CheckWord(space, word, true, rvalue, line_num);
                    break;
                }
                if (left->lexem == lt_point_to || left->lexem == lt_dot)
                {
                    type_t * type = left->type;
                    if (type == nullptr)
                        throw "FSM error 123";
                    bool run = true;
                    int pointer_count = 0;
                    namespace_t * space = nullptr;
                    do
                    {
                        switch (type->prop)
                        {
                        case type_t::pointer_type:
                            type = ((pointer_t*)type)->parent_type;
                            ++pointer_count;
                            break;
                        case type_t::compound_type:
                            run = false;
                            space = ((structure_t*)type)->space;
                            continue;
                        default:
                            throw "type error";
                        }
                    } while (run);
                    if (space == nullptr)
                    {
                        this->space_state->CreateError(line_num, -77716721, "variable '%s' not found", parent->left->text.c_str());
                        break;
                    }
                    node = CheckWord(space, word, left, rvalue, line_num);
                    break;
                }
                else
                    throw "Parser FSM error: not parsed method call";
            }
        }

        node = CheckWord(space_state, word, false, rvalue, line_num);
        if (node == nullptr)
        {
            this->space_state->CreateError(line_num, -77716921, "Syntax error: Non-recognized statement '%s'", word.c_str());
            break;
        }
    }
    while (false);
    return node;
}

expression_node_t	*	ExpressionParser::CreateNode(shunting_yard_t * yard, shunting_yard_t * parent, expression_node_t * left)
{
    //if (yard->line_number == 664)
    //{
    //    fprintf(stderr, "debug\n");
    //}
	expression_node_t	*	node = nullptr;
	expression_node_t	*	lvalue = yard->left ? !yard->unary ? CreateNode(yard->left, yard, nullptr) : nullptr : nullptr;
	expression_node_t	*	rvalue = yard->right ? CreateNode(yard->right, yard, lvalue) : nullptr;
	variable_base_t		*	var = nullptr;
	type_t				*	cast = nullptr;
	function_parser	    *	fptr = nullptr;

	switch (yard->lexem)
	{
	case lt_character:
	{
		node = new expression_node_t(yard->lexem);
		node->type = this->space_state->GetBuiltinType(lt_type_char);
		node->is_constant = true;
		node->constant = CreateConstant(yard);
		break;
	}
	case lt_integer:
	{
		node = new expression_node_t(yard->lexem);
		node->type = this->space_state->GetBuiltinType(lt_type_int);
		node->is_constant = true;
		node->constant = CreateConstant(yard);
		break;
	}
	case lt_string:
	{
		node = new expression_node_t(yard->lexem);
		node->type = this->space_state->GetBuiltinType(lt_type_char);
		node->type = new const_t(node->type);
		node->type = new pointer_t(node->type);
		node->is_constant = true;
		node->constant = CreateConstant(yard);
		break;
	}
	case lt_floatnumber:
	{
		node = new expression_node_t(yard->lexem);
		node->type = this->space_state->GetBuiltinType(lt_type_float);
		node->is_constant = true;
		node->constant = CreateConstant(yard);
		break;
	}
	case lt_true:
	case lt_false:
	{
		node = new expression_node_t(yard->lexem);
		node->type = this->space_state->GetBuiltinType(lt_type_bool);
		node->is_constant = true;
		break;
	}
	case lt_dot:
	case lt_point_to:
	case lt_indirect_pointer_to_member:
	case lt_direct_pointer_to_member:
        if (rvalue == nullptr)
        {
            space_state->CreateError(yard->line_number, -77716122, "field or method '%s' not found", yard->right->text.c_str());
            return nullptr;
        }
        node = new expression_node_t(yard->lexem);
		node->left = lvalue;
		node->right = rvalue;
		node->type = rvalue->type;
		break;
	case lt_word:
	{
        node = CreateWordMode(yard->text, parent, left, rvalue, yard->line_number);
		break;
	}

	case lt_call:
	{
		function_parser * function = this->space_state->FindFunction(yard->text);
		if (function == nullptr)
		{
			// Try pointer to function or typedef
			variable_base_t * var = this->space_state->FindVariable(yard->text);
			if (var == nullptr)
			{
				this->space_state->CreateError(yard->line_number, -77716721, "function '%s' not declared", yard->text.c_str());
				return nullptr;
			}
			switch (var->type->prop)
			{
			case type_t::funct_ptr_type:
				function = ((function_parser*)var->type);
				break;
			case type_t::typedef_type:
			{
				typedef_t * def = (typedef_t*)var->type;
				if (def->type->prop == type_t::funct_ptr_type)
				{
					function = ((function_parser*)def->type);
					break;
				}
				throw "Something wrong at function call";
			}
			case type_t::pointer_type:
			{
				pointer_t * ptr = (pointer_t*)var->type;
				break;
			}
			case type_t::unsigned_type:
				break;

			default:
				throw "Unparsed type's property";
			}
		}

		call_t * call = new call_t(this->space_state, function->name, nullptr);
		PrepareArguments(call, rvalue);
		function_overload_t * overload = function->FindOverload(call);
		yard->type = function->type;

		node = new expression_node_t(yard->lexem);
		node->call = call;
		node->type = function->type;
		break;
	}
	case lt_call_method:
	{
        type_t * type;
        if (left == nullptr)
        {
            if (parent->lexem == lt_point_to || parent->lexem == lt_dot)
            {
                left = CreateNode(parent->left, nullptr, nullptr);
            }
            else 
                throw "State machine error on call nethod";
        }
        else
            printf("Left is not nullptr. Is this ok?");
        type = left->type;
		while (type->prop == type_t::pointer_type)
			type = ((pointer_t*)left->type)->parent_type;
		if (type->prop != type_t::compound_type)
		{
			this->space_state->CreateError(yard->line_number, -77716721, "method cannot call for non compound type '%s'", type->name.c_str());
			break;  
		}
		structure_t * structure = (structure_t *)type;
		function_parser * function = structure->space->FindFunctionInSpace(yard->text);
		if (function == nullptr)
		{
			this->space_state->CreateError(yard->line_number, -77716721, "method '%s' not found in compound type", yard->text.c_str());

			// This is inline method of template class
			function = this->space_state->parent->FindFunction(yard->text);

			if (function == nullptr)
			{
				variable_base_t * var = this->space_state->FindVariable(yard->text);
				if (var == nullptr)
				{
					fprintf(stderr, "Last chance to found function is lost\n");
					break;
				}
				type_t * t = var->type;
				while (t->prop == type_t::pointer_type)
				{
					t = ((pointer_t*)t)->parent_type;
				}

				fprintf(stderr, "Did we found function ptr?\n");
				if (t->prop != type_t::funct_ptr_type)
					break;
				function = (function_parser*)t;
			}

			if (function == nullptr)
				break;
		}

		call_t * call = new call_t(this->space_state, function->name, nullptr);
		PrepareArguments(call, rvalue);
		function_overload_t * overload = function->FindOverload(call);
		yard->type = function->type;

		node = new expression_node_t(yard->lexem);
		node->call = call;
		node->type = function->type;
		break;
	}

	case lt_set:
	case lt_add_and_set:
	case lt_sub_and_set:
	case lt_mul_and_set:
	case lt_div_and_set:
	case lt_rest_and_set:
		if (rvalue == nullptr)
		{
			this->space_state->CreateError(yard->line_number, -77771432, "rvalue error ");
			break;
		}
		if (lvalue == nullptr)
		{
			this->space_state->CreateError(yard->line_number, -77771432, "lvalue error");
			break;
		}
		node = new expression_node_t(yard->lexem);
		node->left = lvalue;
		node->right = rvalue;
		node->type = lvalue->type;
		break;

	case lt_typecasting:
	{
		node = new expression_node_t(lt_typecasting);
		node->type = yard->type;
		node->right = rvalue;
	}
	break;

	case lt_this:
		node = new expression_node_t(yard->lexem);
		node->type = this->space_state->GetBuiltinType(yard->lexem);
		break;

	case lt_mul:
		if (lvalue != nullptr)
		{
			node = new expression_node_t(lt_mul);
			node->left = lvalue;
			node->type = rvalue->type;
			node->right = rvalue;
		}
		else
		{
			// разыменовывние указателя
			node = new expression_node_t(lt_pointer);
			node->type = rvalue->type;
			node->right = rvalue;
		}
		break;
	case lt_inc:
	case lt_dec:
	case lt_logical_not:
	case lt_not:
	case lt_unary_plus:
	case lt_unary_minus:
        if (rvalue == nullptr)
        {
            space_state->CreateError(0, -77770707, "expression error. Possibly duplicated error.");
            return nullptr;
        }
		node = new expression_node_t(yard->lexem);
		node->right = rvalue;
		node->type = rvalue->type;
		break;
    case lt_indirect:
        if (rvalue == nullptr)
            throw "expression state machine error";
        if (rvalue->type->prop != type_t::pointer_type)
        {
            this->space_state->CreateError(yard->line_number, -7777366, "inderection on non-pointer type value" );
            return nullptr;
        }
        node = new expression_node_t(yard->lexem);
        node->right = rvalue;
        node->type = ((pointer_t*)rvalue->type)->parent_type;
        break;
    case lt_addrres:
        node = new expression_node_t(yard->lexem);
        node->right = rvalue;
        node->type = new pointer_t(rvalue->type);
        break;
	case lt_add:
	case lt_sub:
	case lt_div:
	case lt_and:
	case lt_or:
	case lt_xor:
	case lt_rest:
	case lt_shift_left:
	case lt_shift_right:
		node = new expression_node_t(yard->lexem);
		node->left = lvalue;
		node->right = rvalue;
		node->type = lvalue->type;
		break;

    case lt_quest:
        node = new expression_node_t(yard->lexem);
        switch (lvalue->type->prop)
        {
        case type_t::pointer_type:
            node->left = new expression_node_t(lt_not_eq);
            node->left->type = space_state->GetBuiltinType(lt_type_bool);
            node->left->left = lvalue;
            node->left->right = new expression_node_t(lt_integer);
            node->left->right->is_constant = true;
            node->left->right->constant = new constant_node_t(0);
            break;

        default:
            node->left = lvalue;
            break;
        }
        node->right = rvalue;
        node->type = rvalue->type;
        break;

    case lt_colon:
    {
        compare_t   comp = CompareTypes(lvalue->type, rvalue->type, false, false);
        if (comp == no_cast && lvalue->IsConstZero() == true)
        {
            comp = CompareTypes(rvalue->type, lvalue->type, false, true);
            if (comp != no_cast)
                lvalue->type = rvalue->type;
        }
        if (comp == no_cast && rvalue->IsConstZero() == true)
        {
            comp = CompareTypes(lvalue->type, rvalue->type, false, true);
            if (comp != no_cast)
                rvalue->type = lvalue->type;
        }
        if (comp == no_cast)
        {
            this->space_state->CreateError(yard->line_number, -777790123, "ternary operation types not matched");
            break;
        }
        node = new expression_node_t(yard->lexem);
        node->left = lvalue;
        node->right = rvalue;
        node->type = rvalue->type;
        break;
    }

	case lt_namescope:
		node = new expression_node_t(yard->lexem);
		node->left = lvalue;
		node->right = rvalue;
		if (lvalue != nullptr)
			node->type = lvalue->type;
		else
			fprintf(stderr, "Global namespace?\n");
		break;

	case lt_openindex:
		node = new expression_node_t(yard->lexem);
		node->left = lvalue;
		node->right = rvalue;
		node->type = ((pointer_t*)lvalue->type)->parent_type;
		break;

	case lt_logical_and:
	case lt_logical_or:
	case lt_less:
	case lt_equally:
	case lt_not_eq:
	case lt_more:
	case lt_less_eq:
	case lt_more_eq:
		node = new expression_node_t(yard->lexem);
		node->left = lvalue;
		node->right = rvalue;
		node->type = this->space_state->GetBuiltinType(lt_type_bool);
		break;

	case lt_new:
		node = new expression_node_t(yard->lexem);
		if (rvalue != nullptr)
		{
			node->right = rvalue;
			node->is_constant = rvalue->is_constant;
		}
		node->type = new pointer_t(yard->type);
		break;

	case lt_argument:
		node = new expression_node_t(yard->lexem);
		node->left = lvalue;
		node->right = rvalue;
		node->type = lvalue->type;
		break;

    case lt_operator_postinc:
    case lt_operator_postdec:
    {
        if (rvalue->is_constant)
        {
            this->space_state->CreateError(yard->line_number, -777790123, "cannot increment or decrement constant variable");
            break;
        }
        // TODO: check that operator++ exist for this type
        node = new expression_node_t(yard->lexem);
        node->right = rvalue;
        node->type = rvalue->type;
        break;
    }
	default:
		throw "Fix expression_node_t * CreateNode(ExpressionParser::shunting_yard_t yard)";
		break;
	}
    if (node == nullptr)
        fprintf(stderr, "Something going wrong in ExpressionParser::CreateNode. Fix me ASAP.\n");

    return node;
}

shunting_yard_t * ExpressionParser::PrepareExpression()
{
	while (operators.size() > 0)
	{
		shunting_yard_t	op = operators.back();
		operators.pop_back();
		if (operands.size() > 0)
		{
			shunting_yard op1 = operands.back();
			operands.pop_back();
			if (op.unary)
			{
				op.right = new shunting_yard_t(op1);
				operands.push_back(op);
			}
			else
			{
				if (operands.size() > 0)
				{
					shunting_yard op2 = operands.back();
					operands.pop_back();
					op.right = new shunting_yard_t(op1);
					op.left = new shunting_yard_t(op2);
					operands.push_back(op);
				}
				else
					throw "not enought arguments in expression";

			}
		}
		else
		{
			if (op.lexem == lt_namescope || op.lexem == lt_new)
			{
				operands.push_back(op);
			}
			else
				throw "not enought arguments in unary expression";
		}
	}
	return &this->operands.back();
}

expression_t	*	ExpressionParser::BuildExpression(void)
{
	PrepareExpression();

	expression_node_t * root = CreateNode(&this->operands.back(), nullptr, nullptr);
	if (root == nullptr)
		return nullptr;

	operands.pop_back();

	expression_t	*	expression = new expression_t(root);

    return expression;
}

void ExpressionParser::ParseOperator_NEW(SourcePtr &source)
{
	shunting_yard_t	* yard = new shunting_yard_t(source);
	do
	{
		source++;
		if (source == false)
			continue;
		yard->type = space_state->GetBuiltinType(source.lexem);
		if (yard->type == nullptr && source.lexem == lt_word)
			yard->type = space_state->FindType(source.value);
		if (yard->type == nullptr)
			continue;
		source++;

		int priority = 20;
		switch (source.lexem)
		{
        case lt_semicolon:
            break;
        case lt_openbraket:
        {
            ParseArguments(&source, yard);
            break;
        }
        case lt_openindex:
        {
            ExpressionParser	index_parser(space_state);
#if MODERN_COMPILER
            index_parser.ParseExpression(SourcePtr(source.sequence));
#else
            SourcePtr ptr(source.sequence);
            index_parser.ParseExpression(ptr);
#endif
            expression_t * expr = index_parser.BuildExpression();
            if (expr->is_constant == false)
            {
                this->space_state->CreateError(source.line_number, -77791004, "Non-constant index expression not supported");
                continue;
            }
            //			printf("Complete index parsing in operator 'new'\n");
            yard->type = new array_t(yard->type, 5);

            shunting_yard_t * rval = new shunting_yard_t(index_parser.operands.back());
            this->operands.push_back(*rval);
            //			FixExpression(*yard, priority);
            source++;
            break;
        }
        default:
            this->space_state->CreateError(source.line_number, -77771234, "Expression broken in operator new");
            source.Finish();
            continue;
        }

        FixExpression(*yard, priority);
		return;

	} while (false);

	this->space_state->CreateError(source.line_number, -77771234, "Expression broken in operator new");
	source.Finish();
}

void ExpressionParser::ParseOperator_SIZEOF(SourcePtr &source)
{
	const type_t * type = nullptr;
	variable_base_t * var = nullptr;
	std::string size;
	do
	{
		source++;
		if (source == false && source.lexem != lt_openbraket)
		{
			this->space_state->CreateError(source.line_number, -77771236, "sizeof format error" );
			continue;
		}
        ExpressionParser	sizeof_parser(space_state);
#if MODERN_COMPILER
        arg_parser.ParseExpression(SourcePtr(source.sequence));
#else
        SourcePtr ptr(source.sequence);
        sizeof_parser.ParseExpression(ptr);
#endif
        expression_t	* sizeof_exp = nullptr;
        type = nullptr;
        if (sizeof_parser.operands.size() == 1 && sizeof_parser.operators.size() == 0)
        {
            // Check type
            shunting_yard_t yard = sizeof_parser.operands.back();
            if (yard.lexem == lt_word)
                type = space_state->FindType(yard.text);
            else
                type = space_state->GetBuiltinType(yard.lexem);
        }
        if (!type)
        {
            sizeof_exp = sizeof_parser.BuildExpression();
            if (sizeof_exp == nullptr)
            {
                this->space_state->CreateError(source.line_number, -77771237, "cannot parse sizeof argument");
                source.Finish();
                continue;
            }
            type = sizeof_exp->type;
        }
type_found:
        shunting_yard_t		yard(source);
        int sz = type->bitsize >> 3;
#if MODERN_COMPILER
        yard.text = std::to_string(sz);
#else
        char fix[16];
        snprintf(fix, 16, "%d", sz);
        yard.text = fix;
#endif
		yard.lexem = lt_integer;
		this->operands.push_back(yard);
		continue;
	} while (false);
}

lexem_type_t ExpressionParser::ParseIncrementDecrement(SourcePtr & node)
{
	if (!prev_was_operand)
	{
		//		printf("Prefix inc|dec\n");
		return node.lexem;
	}

    if (operands.size() <= 0)
		throw "postfix state machime error";

    shunting_yard_t	* val = PrepareExpression();
    if (val == nullptr)
    {
        fprintf(stderr, "Check that error already generated in PrepareExpression()\n");
    }
    else
    {
        shunting_yard_t	* op = new shunting_yard_t(node);
        switch (op->lexem)
        {
        case lt_inc:
            op->lexem = lt_operator_postinc;
            break;
        case lt_dec:
            op->lexem = lt_operator_postdec;
            break;
        default:
            throw "How did you get here?";
        }

        op->right = new shunting_yard_t(*val);
        operands.pop_back();
        operands.push_back(*op);
    }

	//	printf("Postfix inc|dec\n");
	return lt_empty;
}

void ExpressionParser::ParseArguments(SourcePtr * node, shunting_yard_t * parent)
{
    if (node->sequence->size() > 0)
    {
        SourcePtr	seq(node->sequence);
        while (seq == true)
        {
            ExpressionParser	arg_parser(space_state);
            shunting_yard_t		arg(seq);
            arg_parser.ParseExpression(seq);
            arg.lexem = lt_argument;
            arg.left = arg_parser.PrepareExpression();
            arg.left = new shunting_yard_t(*arg.left);
            parent->right = new shunting_yard_t(arg);

            if (seq == false)
                break;

            if (seq.lexem != lt_comma)
            {
                this->space_state->CreateError(node->line_number, -77749810, "wrong input on waiting arguments delimiter");
                seq.Finish();
                return;
            }
            seq++;
            if (seq != true)
            {
                this->space_state->CreateError(node->line_number, -77749810, "expext arguments after delimiter");
                seq.Finish();
                return;
            }
            parent = parent->right;
        }
    }
}

void ExpressionParser::ParseOpenBracket(SourcePtr &node)
{
	if (prev_was_operand)
	{
		if (operands.size() == 0)
		{
			throw "expression state machine error";
		}
        shunting_yard_t * parent = nullptr;
        if (operators.size() > 0)
        {
            parent = &operators.back();
            int prev_prio = GetLexemPriority(parent->lexem);
            int my_prio = 20;
            if (prev_prio <= my_prio)
                this->PrepareExpression();
        }
		parent = &operands.back();

		switch (parent->lexem)
		{
		case lt_dot:
		case lt_point_to:
		case lt_direct_pointer_to_member:
		case lt_indirect_pointer_to_member:
			if (this->operators.size() == 0)
				parent = &operands.back();
			else
				parent = &operators.back();
			node.lexem = lt_call_method;
			parent = parent->right;
			break;
		default:
			node.lexem = lt_call;
		}

		parent->lexem = node.lexem;
//		fprintf(stderr, "Debug: Call method %s\n", node.value.c_str());
#if true
        ParseArguments(&node, parent);
#else
        SourcePtr	seq(node.sequence);
        if (node.sequence->size() > 0)
		{
			while (seq == true)
			{
                ExpressionParser	arg_parser(space_state);
                shunting_yard_t		arg(seq);
				arg_parser.ParseExpression(seq);
				arg.lexem = lt_argument;
				arg.left = arg_parser.PrepareExpression();
				arg.left = new shunting_yard_t(*arg.left);
				parent->right = new shunting_yard_t(arg);

				if (seq == false)
					break;

				if (seq.lexem != lt_comma)
				{
					this->space_state->CreateError(node.line_number, -77749810, "wrong input on waiting arguments delimiter");
					seq.Finish();
					return;
				}
				seq++;
				if (seq != true)
				{
					this->space_state->CreateError(node.line_number, -77749810, "expext arguments after delimiter");
					seq.Finish();
					return;
				}
				parent = parent->right;
			}
		}
#endif
		return;
	}
	else
	{
		SourcePtr		seq(node.sequence);
		type_t		*	cast = this->space_state->TryLexenForType(seq);
		if (cast != nullptr)
		{
			shunting_yard_t * yard = new shunting_yard_t(node);
			yard->lexem = lt_typecasting;
			yard->unary = true;
			yard->type = cast;
			while (++seq != false)
			{
				switch (seq.lexem)
				{
				case lt_mul:
					yard->type = new pointer_t(yard->type);
					break;
				default:
					throw "Need additional parsing on casting to type";
				}
			}
			operators.push_back(*yard);
			prev_was_operand = false;
			return;
		}
		ExpressionParser	parenthesis(space_state);

		parenthesis.ParseExpression(seq);
		shunting_yard_t * shunt = parenthesis.PrepareExpression();
		operands.push_back(*shunt);
		prev_was_operand = true;
		return;
	}
}

lexem_type_t ExpressionParser::ParseExpression(SourcePtr &source)
{
	lexem_type_t	prefix = lt_empty;
	SourcePtr	node(source);
	for (node = source; node != false; node != false ? node++ : node)
	{
		switch (node.lexem)
        {
        case lt_comma:
        {
        source = node;
        //			fprintf(stderr, "Comma separated expression\n");
        return node.lexem;
        }
        case lt_semicolon:
        {
            source = node;
            //			fprintf(stderr, "Semicolon separated expression\n");
            return node.lexem;
        }
        case lt_namescope:
        {
            shunting_yard_t yard(source);
            if (source.previous_lexem == lt_word)
            {
                if (operands.size() == 0)
                    throw "mismatched scope";
                yard.left = new shunting_yard_t(operands.back());
                operands.pop_back();
            }
            operators.push_back(yard);
            continue;
        }
        case lt_new:
        {
            ParseOperator_NEW(node);
            source = node;
            continue;
        }
        case lt_sizeof:
        {
            ParseOperator_SIZEOF(node);
            prev_was_operand = true;
            continue;
        }
        case lt_inc:
        case lt_dec:
        {
            prefix = ParseIncrementDecrement(node);
            if (prefix == lt_error)
                return prefix;
            if (prefix == lt_empty)
                continue;
            break;
        }
        case lt_add:
        {
            if (!prev_was_operand)
                node.lexem = lt_unary_plus;
            break;
        }
        case lt_sub:
        {
            if ( !prev_was_operand)
                node.lexem = lt_unary_minus;
            break;
        }
        case lt_openbraket:
        {
            ParseOpenBracket(node);
            continue;
        }
        case lt_quest:
        {
            shunting_yard_t yard(node);
            prev_was_operand = false;
            FixExpression(yard, 140); // Need 150 and rearrange priorities5
            continue;
        }
        case lt_colon:
        {
            shunting_yard_t yard(node);
            operators.push_back(yard);
            prev_was_operand = false;
            continue;
        }

        }
		Parse(node);
	}
	source = node;
	return source.lexem;
}

expression_node_t * FixConstants(expression_node_t * node)
{
	constant_node_t * lc = nullptr, *rc = nullptr;
	if (node->left)
	{
		node->left = FixConstants(node->left);
		lc = node->left->constant;
	}
	if (node->right)
	{
		node->right = FixConstants(node->right);
		rc = node->right->constant;
	}

	int val;

	if (lc != nullptr && rc != nullptr)
	{

		switch (node->lexem)
		{
		case lt_add:
			val = node->left->constant->integer_value + node->right->constant->integer_value;
			break;
		case lt_sub:
			val = node->left->constant->integer_value - node->right->constant->integer_value;
			break;
		case lt_mul:
			val = node->left->constant->integer_value * node->right->constant->integer_value;
			break;
		case lt_div:
			val = node->left->constant->integer_value / node->right->constant->integer_value;
			break;
        case lt_rest:
            val = node->left->constant->integer_value % node->right->constant->integer_value;
            break;
        case lt_shift_left:
			val = node->left->constant->integer_value <<= node->right->constant->integer_value;
			break;
		case lt_shift_right:
			val = node->left->constant->integer_value >>= node->right->constant->integer_value;
			break;

		default:
			throw "Check operation";
		}

        node->constant = new constant_node_t(val);
		node->lexem = lt_integer;
        node->is_constant = true;
    }

	if (node->constant != nullptr)
	{
		if (node->left)
		{
			delete node->left;
			node->left = 0;
		}
		if (node->right)
		{
			delete node->right;
			node->right = 0;
		}
	}
	return node;
}

expression_t  * namespace_t::ParseExpression(SourcePtr &source)
{
	::ExpressionParser	ep(this);

	lexem_type_t res = ep.ParseExpression(source);
	if (res == lt_error)
		return 0;
	if (source == true && source.lexem != lt_semicolon && source.lexem != lt_comma)
	{
		CreateError(source.line_number, -77761432, "Semicolon or comma expected");
	}
	expression_t  * expression = ep.BuildExpression();
    if (expression != nullptr)
    {
        expression->root = FixConstants(expression->root);
        expression->type = expression->root->type;
        expression->is_constant = expression->root->is_constant;
    }
	return expression;
}

expression_t  * namespace_t::ParseExpressionExtended(SourcePtr &source, type_t ** p_type, bool hide)
{
	do
	{
		*p_type = this->TryLexenForType(source);
		if (*p_type == nullptr)
			continue;

		source++;
        if (source == false)
            continue;
        if (source.lexem == lt_mul)
        {
            *p_type = new pointer_t(*p_type);
            source++;
        }
        if (source == false)
            continue;
        SourcePtr	name_ptr(source);
		if (name_ptr != false && name_ptr.lexem == lt_word)
		{
			linkage_t	linkage;
			variable_base_t * instance = CreateVariable(*p_type, name_ptr.value, &linkage);
			instance->hide = hide;
			continue;
		}

	} while (false);

    if(source == false)
    {
        CreateError(source.line_number, -77761432, "Unable parse typed declaration");
        source.Finish();
        return nullptr;
    }

	return ParseExpression(source);
}

expression_node_t::expression_node_t(lexem_type_t	lexem)
{
	this->lexem = lexem;
    this->is_constant = false;
	switch (lexem)
	{
	case lt_integer:
		//	case lt_hexdecimal:
		type = namespace_t::GetBuiltinType(lt_type_int);
		break;
	case lt_floatnumber:
		type = namespace_t::GetBuiltinType(lt_type_float);
		break;
	case lt_string:
		type = namespace_t::GetBuiltinType(lt_string);
		type = new pointer_t(type);
		break;
	default:
		type = nullptr;
	}
	variable = nullptr;
	constant = nullptr;
	left = right = nullptr;
	call = nullptr;
}

