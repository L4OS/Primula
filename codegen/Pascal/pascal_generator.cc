#include "pascal_generator.h"
#include <stdarg.h>

static char INDENT[80];
static int	right = 0;

#pragma region Static functions

void PascalGenerator::MoveIndentRight()
{
    INDENT[right] = ' ';
    right += 2;
    if (right > sizeof(INDENT) - 1)
        throw "Block overflow on restoring source code";
    INDENT[right] = '\0';
}

void PascalGenerator::MoveIndentLeft()
{
    INDENT[right] = ' ';
    right -= 2;
    if (right < 0)
        throw "Block underflow on restoring source code";
    INDENT[right] = '\0';
}

void PascalGenerator::Write(const char * s, ...)
{
    va_list args;
    va_start(args, s);
    vprintf(s, args);
    va_end(args);
}

void PascalGenerator::IndentWrite(const char *s, ...)
{
    printf(INDENT);

    va_list args;
    va_start(args, s);
    vprintf(s, args);
    va_end(args);
}

void PascalGenerator::PrintOpenBlock()
{
	IndentWrite("begin\n");
    MoveIndentRight();
}

void PascalGenerator::PrintCloseBlock()
{
    MoveIndentLeft();
    IndentWrite("end\n");
}

#pragma endregion Static functions

PascalGenerator::PascalGenerator()
{
	INDENT[0] = '\0';
	int i;
	for (i = 1; i < sizeof(INDENT); ++i)
		INDENT[i] = ' ';
}

PascalGenerator::~PascalGenerator()
{
}

void PascalGenerator::GenerateCall(call_t * call)
{
	if (call -> code == nullptr)
	{
		printf("// Parser fault on function call :(\n");
		return;
	}
	if (call->code->function->method_type == function_parser::method)
		Write("%s", call->code->function->name.c_str());
	Write("(");
    std::list<expression_t *>::iterator arg;
    for (arg = call->arguments.begin(); arg != call->arguments.end(); ++arg)
    {
        GenerateExpression(*arg);
        if (&(*arg) != &call->arguments.back())
            Write(", ");
    }
    Write(")");
}

extern int GetLexemPriority(lexem_type_t  lex);

void PrintCharacter(int ch)
{
    switch (ch)
    {
    case '\0':
        printf("'\\0'");
        break;
    case '\\':
        printf("'\\\\'");
        break;
    case '\'':
        printf("'\\''");
        break;
    case '\"':
        printf("'\\\"'");
        break;
    case '\r':
        printf("'\\r'");
        break;
    case '\n':
        printf("'\\n'");
        break;
    case '\t':
        printf("'\\t'");
        break;
    default:
        printf("'%c'", ch);
    }
}

void PascalGenerator::GenerateItem(expression_node_t * n, int parent_priority)
{
	int my_prio = GetLexemPriority(n->lexem);
	bool use_brakets = my_prio > parent_priority &&  my_prio < 1000;
    const char * str = nullptr;

	//if (n->lexem != lt_namescope)
	//{
	//	if (my_prio > parent_priority &&  my_prio < 1000)
	//		use_brakets = true;
	//}

	if (use_brakets)
		Write("( ");

	if (n->left)
		GenerateItem(n->left, my_prio);

	switch (n->lexem)
	{
	case lt_pointer:
		str = " * ";
		break;
	case lt_indirect_pointer_to_member:
		str = " ->* ";
		break;
	case lt_this:
		str = "self";
		break;
	case lt_variable:
		Write("%s", n->variable->name.c_str());
		break;
	case lt_inc:
		str = "++";
		break;
	case lt_dec:
		str = "--";
		break;
	case lt_set:
		str = " := ";
		break;
	case lt_add:
		str = " + ";
		break;
	case lt_add_and_set:
		str = " += ";
		break;
    case lt_sub_and_set:
        str = " -= ";
        break;
	case lt_sub:
		str = " - ";
		break;
	case lt_mul:
		str = " * ";
		break;
	case lt_div:
		str = " / ";
		break;
	case lt_and:
        str = " & ";
        break;
    case lt_addrres:
		str = " @ ";
		break;
	case lt_shift_left:
		str = " SHL ";
		break;
	case lt_shift_right:
		str = " SHR ";
		break;
	case lt_or:
		str = " | ";
		break;
	case lt_dot:
		str = ".";
		break;
	case lt_point_to:
		str = "->";
		break;
	case lt_integer:
		Write("%d", n->constant->integer_value);
		break;
	case lt_string:
		Write("'%s'", n->constant->char_pointer);
		break;
	case lt_character:
        PrintCharacter(n->constant->integer_value);
		break;
	case lt_equally:
		str = " = ";
		break;
    case lt_not_eq:
        str = " <> ";
        break;
	case lt_logical_not:
		str = " NOT ";
		break;
	case lt_logical_and:
		str = " AND ";
		break;
	case lt_logical_or:
		str = " OR ";
		break;
	case lt_less:
		str = " < ";
		break;
	case lt_less_eq:
		str = " <= ";
		break;
	case lt_more:
		str = " > ";
		break;
	case lt_more_eq:
		str = " >= ";
		break;
	case lt_quest:
		str = " ? ";
		break;
	case lt_colon:
		str = " : ";
		break;
	case lt_rest:
		str = " MOD ";
		break;
	case lt_rest_and_set:
		str = " %%= ";
		break;
	case lt_xor:
		str = " XOR ";
		break;
	case lt_namescope:
		str = " :: ";
	case lt_indirect:
		str = " *";
		break;
	case lt_call:
		GenerateCall(n->call);
		break;
	case lt_call_method:
		GenerateCall(n->call);
		break;
	case lt_unary_plus:
		str = " +";
		break;
	case lt_unary_minus:
		str = " -";
		break;

	case lt_true:
		str = " true ";
		break;
	case lt_false:
		str = " false ";
		break;

	case lt_openindex:
	{
		printf(" [ ");
		GenerateItem(n->right);
		printf(" ] ");
		return;
	}
	break;

	case lt_typecasting:
	{
		GenerateTypeName(n->type);
        printf("(");
        GenerateItem(n->right);
        printf(") ");
		return;
	}

	case lt_new:
	{
		printf(" New ");
		GenerateTypeName(n->type);
		if (n->right != nullptr)
		{
			printf("[");
			GenerateItem(n->right);
			printf("]");
		}
		return;
	}

	case lt_function_address:
	{
		function_parser * function = (function_parser*)n->type;
		printf("%s;%s", function->name.c_str(), INDENT);
		break;
	}

    case lt_operator_postinc:
    case lt_operator_postdec:
    {
        GenerateItem(n->right);
        printf(n->lexem == lt_operator_postinc ? "++ " : "-- ");
        return;
    }

	default:
		throw "Cannot generate lexem";
		break;
	}

    if (str != nullptr)
        Write(str);

	if (n->right)
		GenerateItem(n->right, my_prio);

	if (use_brakets)
		Write(") ");
}

void PascalGenerator::GenerateExpression(expression_t * exp)
{
	if (exp->root != nullptr)
	{
        GenerateItem(exp->root);
	}
}

void PascalGenerator::GenerateStatement(statement_t * code)
{
	if (code->type == statement_t::_variable)
	{
		variable_base_t * var = (variable_base_t *)code;
		if (var->hide)
			return;
	}

	switch (code->type)
	{
	case statement_t::_return:
	{
		return_t  * ret_code = (return_t*)code;
		expression_t * value = ret_code->return_value;

        if (value)
        {
            IndentWrite("Exit(");
            GenerateExpression(value);
            Write(")");
        }
        else
            IndentWrite("Exit");
		break;
	}

	case statement_t::_call:
	{
        Write(INDENT);
		GenerateCall((call_t*)code);
		break;
	}

	case statement_t::_expression:
	{
		expression_t * expr = (expression_t*)code;
		if (expr->root == nullptr)
			printf("%s// !!! empty expression !!!\n", INDENT);
		else
		{
            Write(INDENT);
			GenerateExpression(expr);
		}
		break;
	}

	case statement_t::_do:
	{
		operator_DO * op = (operator_DO*)code;
		IndentWrite("repeat\n");
        MoveIndentRight();
		GenerateStatement(op->body);
        MoveIndentLeft();
        IndentWrite("until ");
		GenerateExpression(op->expression);
		Write(";\n");
		return;
	}

	case statement_t::_while:
	{
		operator_WHILE * op = (operator_WHILE*)code;
        IndentWrite("while ");
		GenerateExpression(op->expression);
        Write(" do\n");
		GenerateStatement(op->body);
		return;
	}

	case statement_t::_if:
	{
        bool block;
		operator_IF * op = (operator_IF*)code;
		IndentWrite("if ");
        if (op->expression == nullptr)
            Write("/* error expression parsing */");
        else
		    GenerateExpression(op->expression);
		Write(" then\n");
        MoveIndentRight();
        if (op->true_statement == nullptr)
        {
            IndentWrite("/* true 'if' statements lost */\n");
        }
        else
        {
            block = (op->true_statement->type == statement_t::_codeblock);
            GenerateStatement(op->true_statement);
        }

		if (op->false_statement != nullptr)
		{
            MoveIndentLeft();
            IndentWrite("else\n");
            MoveIndentRight();
            GenerateStatement(op->false_statement);
		}
        MoveIndentLeft();
		return;
	}

	case statement_t::_switch:
	{
		operator_SWITCH * op = (operator_SWITCH*)code;
		IndentWrite("case ");
		GenerateExpression(op->expression);
		Write("of");
		PrintOpenBlock();

        ((PascalGenerator*)op->body)->GenerateSpace();

        PrintCloseBlock();
		return;
	}

	case statement_t::_case:
	{
		operator_CASE * op = (operator_CASE *)code;
		IndentWrite("%d : ", op->case_value);
		return;
	}

	case statement_t::_continue:
        IndentWrite("continue");
		break;

	case statement_t::_break:
        IndentWrite("break");
		break;

	case statement_t::_default:
        IndentWrite("else");
		return;

	case statement_t::_goto:
	{
		operator_GOTO * op = (operator_GOTO*)code;
        IndentWrite("goto %s", op->label.c_str());
		break;
	}
	case statement_t::_label:
	{
		operator_LABEL * op = (operator_LABEL*)code;
        IndentWrite("%s:\n", op->label.c_str());
		return;
	}
	case statement_t::_delete:
	{
		operator_DELETE * op = (operator_DELETE*)code;
        IndentWrite("delete %s %s", op->as_array ? "[]" : "", op->var->name.c_str());
		break;
	}
	case statement_t::_variable:
	{
		variable_base_t * var = (variable_base_t *)code;
        if (var->declaration == nullptr)
            return;
		{
            IndentWrite("%s ", var->name.c_str());
            switch (var->declaration->type)
			{
			case statement_t::_expression:
				Write(":= ");
				GenerateExpression((expression_t*)var->declaration);
				break;
			case statement_t::_call:
				GenerateCall((call_t*)var->declaration);
				break;
			default:
				throw "Unparsed type of varibale declaration";
			}
		}
		if (var->static_data != nullptr)
		{
			Write(" = ");
			GenerateStaticData(var->static_data, true);
		}
		break;
	}
	case statement_t::_for:
	{
		operator_FOR * op = (operator_FOR *)code;

        if (op->init_type)
            fprintf(stderr, "{ Don't forget declare variable! }\n");

        if (op->init != nullptr)
        {
            Write(INDENT);
            GenerateExpression(op->init);
            Write(";\n");
        }
        IndentWrite("while ");
		if (op->condition != nullptr)
			GenerateExpression(op->condition);
		Write(" do\n");
		PrintOpenBlock();
        if (op->body != nullptr)
        {
            ((PascalGenerator*)op->body)->GenerateSpace();
        }
        if (op->iterator != nullptr)
        {
            Write(INDENT);
            ((PascalGenerator*)op)->GenerateExpression(op->iterator);
            Write(";\n");
        }
        PrintCloseBlock();
		return;
	}
	case statement_t::_codeblock:
	{
		PrintOpenBlock();
		codeblock_t * op = (codeblock_t*)code;

        ((PascalGenerator*)op->block_space)->GenerateSpace();

        PrintCloseBlock();
		return;
	}
	case statement_t::_trycatch:
	{
		operator_TRY * op = (operator_TRY*)code;
		printf("try");

        ((PascalGenerator*)op->body)->GenerateSpace();

        IndentWrite("except on ");
        GenerateTypeName(op->type);
        Write("%s ", op->exception.c_str());
        Write(" do\n");

        ((PascalGenerator*)op->handler->space)->GenerateSpace();

        IndentWrite("end;\n");
        return;
	}
	case statement_t::_throw:
	{
		operator_THROW * op = (operator_THROW *)code;
		IndentWrite("rise Exception.Create('%s')", op->exception_constant->char_pointer);
		break;
	}
	default:
		throw "Not parsed statement type";
	}
	Write(";\n");
}

void PascalGenerator::GenerateFunction(function_parser * f, bool proto)
{
    function_overload_list_t::iterator  overload;
    for (
        overload = f->overload_list.begin();
        overload != f->overload_list.end();
        ++overload)
    {
        GenerateFunctionOverload(*overload, proto);
    }
}

void PascalGenerator::GenerateSpaceVariables()
{
    for (
        auto & var = space_variables_list.begin();
        var != space_variables_list.end();
        ++var)
    {
        IndentWrite("%s: ", (*var)->name.c_str());
        GenerateTypeName((*var)->type);
        Write(";\n");
    }
}

void PascalGenerator::GenerateSpaceFunctions()
{
    std::list<class function_parser*>::iterator f;
    for (
        f = function_list.begin();
        f != function_list.end();
        ++f)
    {
        function_overload_list_t::iterator  overload;
        for (
            overload = (*f)->overload_list.begin();
            overload != (*f)->overload_list.end();
            ++overload)
        {
            if ((*overload)->space != nullptr && !(*overload)->linkage.inlined)
                GenerateFunction(*f, false);
        }
    }
}

void PascalGenerator::GenerateSpaceCode()
{
    /* Do variables declaration as statements */
    if (this->space_code.size() > 0)
    {
        //		printf("/* Code definitions */\n");
        std::list<statement_t *>::iterator code;
        for (
            code = this->space_code.begin();
            code != this->space_code.end();
            ++code)
        {
            GenerateStatement(*code);
        }
    }
}

void PascalGenerator::GenerateType(type_t * type, bool inlined_only)
{
	switch (type->prop)
	{
    case type_t::template_type:
    {
        IndentWrite("template<class %s>%s", type->name.c_str(), "FiXME");
        template_t * temp = (template_t *)type;

        ((PascalGenerator*)temp->space)->GenerateSpace();

        printf(" ");
        break;
    }
    case type_t::pointer_type:
    {
        pointer_t * op = (pointer_t*)type;
        if (op->parent_type->bitsize == 8)
        {
            printf(" PChar ");
        }
        else
        {
            printf(" ^ ");
            GenerateType(op->parent_type, inlined_only);
        }
    }
    case type_t::funct_ptr_type:
    {
        function_parser * func_ptr = (function_parser*)type;
        GenerateTypeName(func_ptr->type);
        printf("%s(", func_ptr->name.c_str());

        function_overload_list_t::iterator func;
        for (
            func = func_ptr->overload_list.begin();
            func != func_ptr->overload_list.end();
            )
        {
            //			function_parser * func = (function_parser*)func_ptr;
            arg_list_t::iterator arg;
            for (
                arg = (*func)->arguments.begin();
                arg != (*func)->arguments.begin();
                ++arg)
            {
                printf("%s: ", arg->name.c_str());
                GenerateTypeName(arg->type);
                if (&arg != &(*func)->arguments.begin())
                    printf(", ");
            }
        }
        printf(")");
    }
    case type_t::compound_type:
    {
        structure_t * compound_type = (structure_t*)type;
        IndentWrite("%s = record\n", compound_type->name.c_str());
        MoveIndentRight();
        //		GenerateTypeName(type);
        if (compound_type->space->inherited_from.size() > 0)
        {
            Write(": ");

            std::list<structure_t*>::iterator inherited_from;
            for (
                inherited_from = compound_type->space->inherited_from.begin();
                inherited_from != compound_type->space->inherited_from.end();
                ++inherited_from)
            {
                Write("%s", (*inherited_from)->name.c_str());
                printf(&(*inherited_from) != &compound_type->space->inherited_from.back() ? ", " : " ");
            }
        }

        ((PascalGenerator*)compound_type->space)->GenerateSpaceVariables();
        ((PascalGenerator*)compound_type->space)->GenerateSpaceFunctions();
        MoveIndentLeft();
        IndentWrite("end;\n\n");
        break;
    }
    case type_t::enumerated_type:
    {
        enumeration_t * en = (enumeration_t*)type;
        Write("( ");
        int idx = 0;

        if (en->enumeration.size() > 4)
        {
            MoveIndentRight();
            Write("\n%s", INDENT);
        }
        int counter = 0;
        std::map<std::string, int>::iterator item;
        for (
            item = en->enumeration.begin();
            item != en->enumeration.end();)
            // ++item)
        {
            if (counter == 4)
            {
                Write("\n%s", INDENT);
                counter = 0;
            }
            if (item->second == idx)
                printf(item->first.c_str());
            else
            {
                idx = item->second;
                Write("%s = %d", item->first.c_str(), idx);
            }
            ++item;
            ++idx;
            ++counter;
            Write(item != en->enumeration.end() ? ", " : ");\n");
        }

        if (en->enumeration.size() > 4)
            MoveIndentLeft();
        break;
    }
    case type_t::dimension_type:
    {
        array_t * array = (array_t*)type;
        GenerateTypeName(array->child_type);
        Write("[%d] ", array->items_count);
        break;
    }
    case type_t::typedef_type:
    {
        typedef_t * def = (typedef_t *)type;

        //        printf("%stypedef ", INDENT);

        if (def->type->prop != type_t::enumerated_type)
        {
            GenerateType(def->type, false);
            Write("\n");
        }
        else
        {
            IndentWrite("%s = ", type->name.c_str());
            if (def->type->name.size() > 0)
            {
//                GenerateTypeName(def->type);
                GenerateType(def->type, true);
            }
            else
            {
                GenerateType(def->type, true);
            }
            Write("\n");
        }
        break;
    }
    case type_t::auto_type:
    {
        printf("/* AUTO */");
        break;
    }
    default:
        throw "\n--------- TODO: Fix source generator ---------\n";
    }
}

void PascalGenerator::GeneratePrototypes()
{
    std::list<class function_parser*>::iterator f;
    for (f = function_list.begin(); f != function_list.end(); ++f)
    {
        if ((*f)->access_count != 0)
            GenerateFunction(*f, true);

    }
}

void PascalGenerator::GenerateSpace()
{
    GeneratePrototypes();
    GenerateSpaceCode();
    GenerateSpaceFunctions();
}

void PascalGenerator::GenerateCode(namespace_t * space)
{
    Write("{ This file was auto generated by Primula parsers }\n");
    if (space->space_types_list.size() > 0)
    {
        Write("type\n");
        MoveIndentRight();
        ((PascalGenerator*)space)->GenerateTypes();
        MoveIndentLeft();
    }

    if (space->space_variables_list.size() > 0)
    {
        Write("\n { Variables }\nvar\n");
        MoveIndentRight();
        ((PascalGenerator*)space)->GenerateSpaceVariables();
        MoveIndentLeft();
    }


    Write("\n { Function prototypes }\n");
    MoveIndentRight();
    ((PascalGenerator*)space)->GeneratePrototypes();
    MoveIndentLeft();

    Write("\n { Function implementation }\n");
    ((PascalGenerator*)space)->GenerateSpaceFunctions();

    if (space->space_code.size() > 0)
    {
        Write("\ninitialization\n");
        MoveIndentRight();
        ((PascalGenerator*)space)->GenerateSpaceCode();
        MoveIndentLeft();
    }
}
