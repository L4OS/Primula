#include "restore_source_t.h"

static char NewLine[80];
static int	right = 0;

static void PrintOpenBlock()
{
	printf("\n%s{\n", NewLine);
	NewLine[right] = ' ';
	right += 4;
	if (right > sizeof(NewLine) - 1)
		throw "Block overflow on restoring source code";
	NewLine[right] = '\0';
}

static void PrintCloseBlock()
{
	NewLine[right] = ' ';
	right -= 4;
	if (right < 0)
		throw "Block underflow on restoring source code";
	NewLine[right] = '\0';
	printf("%s}\n", NewLine);
}

restore_source_t::restore_source_t()
{
	NewLine[0] = '\0';
	int i;
	for (i = 1; i < sizeof(NewLine); ++i)
		NewLine[i] = ' ';
}

restore_source_t::~restore_source_t()
{
}

void GenerateType(type_t * type, bool inlined_only);
bool GenerateTypeName(type_t * type, const char * name);
void GenerateExpression(expression_t * exp);

void GenerateCall(call_t * call)
{
	if (call -> code == nullptr)
	{
		printf("// Parser fault on function call :(\n");
		return;
	}
	if (call->code->function->method_type == function_parser::method)
		printf("%s", call->code->function->name.c_str());
	printf("(");
#if MODERN_COMPILER
	for (auto & arg : call->arguments)
	{
		GenerateExpression(arg);
		if (&arg != &call->arguments.back())
			printf(", ");
	}
#else
    std::list<expression_t *>::iterator arg;
    for (arg = call->arguments.begin(); arg != call->arguments.end(); ++arg)
    {
        GenerateExpression(*arg);
        if (&(*arg) != &call->arguments.back())
            printf(", ");
    }
#endif
	printf(")");
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

void GenerateItem(expression_node_t * n, int parent_priority = 1000)
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
		printf("( ");

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
		str = "this";
		break;
	case lt_variable:
		printf("%s", n->variable->name.c_str());
		break;
	case lt_inc:
		str = "++";
		break;
	case lt_dec:
		str = "--";
		break;
	case lt_set:
		str = " = ";
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
	case lt_addrres:
		str = " & ";
		break;
	case lt_shift_left:
		str = " << ";
		break;
	case lt_shift_right:
		str = " >> ";
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
		printf("%d", n->constant->integer_value);
		break;
	case lt_string:
		printf("\"%s\"", n->constant->char_pointer);
		break;
	case lt_character:
        PrintCharacter(n->constant->integer_value);
		break;
	case lt_equally:
		str = " == ";
		break;
    case lt_not_eq:
        str = " != ";
        break;
	case lt_logical_not:
		str = " ! ";
		break;
	case lt_logical_and:
		str = " && ";
		break;
	case lt_logical_or:
		str = " || ";
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
		str = " %% ";
		break;
	case lt_rest_and_set:
		str = " %%= ";
		break;
	case lt_xor:
		str = " ^ ";
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
		printf("(");
		GenerateTypeName(n->type, nullptr);
		printf(") ");
		break;
	}

	case lt_new:
	{
		printf(" new ");
		GenerateTypeName(n->type, nullptr);
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
		printf("%s;%s", function->name.c_str(), NewLine);
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
        printf(str);

	if (n->right)
		GenerateItem(n->right, my_prio);

	if (use_brakets)
		printf(") ");
}

void GenerateExpression(expression_t * exp)
{
	if (exp->root != nullptr)
	{
		GenerateItem(exp->root);
	}
}

bool GenerateTypeName(type_t * type, const char * name)
{
	bool skip_name = false;
	if (type != nullptr)
	{
		switch (type->prop)
		{
		case type_t::pointer_type:
		{
			address_t * addres = (address_t*)type;
			if (addres->parent_type == nullptr)
			{
				throw "pointer syntax error";
			}
			GenerateTypeName(addres->parent_type, nullptr);
			if (name)
				printf("* %s", name);
			else
				printf("* ");
			break;
		}
		case type_t::funct_ptr_type:
		{
			function_parser * func_ptr = (function_parser*)type;
			printf("%s", func_ptr->name.c_str());
			if (name)
				printf(" %s", name);
			else
				printf(" ");
			break;
		}
		case type_t::constant_type:
		{
			const_t * constant = (const_t*)type;
			if (constant->parent_type == nullptr)
			{
				throw "constant syntax error";
			}
			printf("const ");
			GenerateTypeName(constant->parent_type, name);
			break;
		}
		case type_t::dimension_type:
		{
			array_t * array = (array_t*)type;
			skip_name = GenerateTypeName(array->child_type, name);
			if (skip_name)
				printf("[%d] ", array->items_count);
			else
				printf("%s [%d] ", name, array->items_count);
			skip_name = true;;
			break;
		}
		case type_t::compound_type:
		{
			structure_t * compound_type = (structure_t*)type;
			const char * compound_name;
			switch (compound_type->kind)
			{
			case lt_struct:
				compound_name = "struct";
				break;
			case lt_class:
				compound_name = "class";
				break;
			case lt_union:
				compound_name = "union";
				break;
			default:
				throw "Non-compound type marked as compound";
			}
			if (name)
				printf("%s %s %s ", compound_name, type->name.c_str(), name);
			else
				printf("%s %s ", compound_name, type->name.c_str());
			break;
		}

		case type_t::enumerated_type:
		{
			if (name)
				printf("enum %s %s ", type->name.c_str(), name);
			else
				printf("enum %s ", type->name.c_str());
			break;
		}

		default:
			if (name)
				printf("%s %s ", type->name.c_str(), name);
			else
				printf("%s ", type->name.c_str());
		}
	}
	return true; // skip_name;
}

void GenerateSpace(namespace_t * space);

void GenerateStaticData(static_data_t * data, bool last, bool selfformat = false)
{
	switch (data->type)
	{
	case lt_openblock:
		//structure_t * structure = (structure_t *)data->nested;
		if (selfformat)
			printf("{");
		else
			PrintOpenBlock();
#if MODERN_COMPILER
        for (auto & compound : *data->nested)
		{
			if (!selfformat)
				printf("\n%s", NewLine);
            if (compound == nullptr)
            {
                printf("/* something goes wrong */0, ");
            }
            else
            {
                bool last = (compound == data->nested->back());
                GenerateStaticData(compound, last, true);
            }
		}
#else
        for (
            std::list<struct static_data*>::iterator compound  = data->nested->begin();
            compound != data->nested->end();
            ++compound)
        {
            if (!selfformat)
                printf("\n%s", NewLine);
            if (*compound == nullptr)
            {
                printf("/* something goes wrong */0, ");
            }
            else
            {
                bool last = (*compound == data->nested->back());
                GenerateStaticData(*compound, last, true);
            }
        }
#endif
        if(selfformat)
			printf("}");
		else
			PrintCloseBlock();
		break;
	case lt_string:
		printf("\"%s\"", data->p_char);
		break;
	case lt_integer:
		printf("%d", data->s_int);
		break;
	case lt_openindex:
		if (data->nested->size() == 0)
			printf("\n /* Nested data size is not defined */%s", NewLine);
		printf("{ ");
#if MODERN_COMPILER
        for (auto & term : *data->nested)
		{
			bool last = (term == data->nested->back());
			GenerateStaticData(term, last, false);
		}
#else
        {
            std::list<struct static_data*>::iterator term;
            for (
                term = data->nested->begin();
                term != data->nested->end();
                ++term)
            {
                bool last = (*term == data->nested->back());
                GenerateStaticData(*term, last, false);
            }
        }
#endif
        printf(" }");
		break;
	default:
		throw "Static data type not parsed";
	}
	if (!last)
		printf(", ");
}

void GenerateStatement(statement_t * code)
{
	if (code->type == statement_t::_variable)
	{
		variable_base_t * var = (variable_base_t *)code;
		if (var->hide)
			return;
	}

	printf(NewLine);
	switch (code->type)
	{
	case statement_t::_return:
	{
		return_t  * ret_code = (return_t*)code;
		expression_t * value = ret_code->return_value;
		printf("return ");
		if (value)
			GenerateExpression(value);
		break;
	}

	case statement_t::_call:
	{
		GenerateCall((call_t*)code);
		break;
	}

	case statement_t::_expression:
	{
		expression_t * expr = (expression_t*)code;
		if (expr->root == nullptr)
			printf("%s// !!! empty expression !!!\n", NewLine);
		else
		{
			GenerateExpression(expr);
		}
		break;
	}

	case statement_t::_do:
	{
		operator_DO * op = (operator_DO*)code;
		printf("do");
		GenerateStatement(op->body);
		printf("while(");
		GenerateExpression(op->expression);
		printf(");\n");
		return;
	}

	case statement_t::_while:
	{
		operator_WHILE * op = (operator_WHILE*)code;
		printf("while( ");
		GenerateExpression(op->expression);
		printf(" )");
		GenerateStatement(op->body);
		return;
	}

	case statement_t::_if:
	{
		operator_IF * op = (operator_IF*)code;
		printf("if(");
        if (op->expression == nullptr)
            printf("/* error expression parsing */");
        else
		    GenerateExpression(op->expression);
		printf(")");
        if (op->true_statement == nullptr)
        {
            printf("/* true 'if' statements lost */\n");
        }
        else
        {
            bool block = (op->true_statement->type == statement_t::_codeblock);
            if (!block)
                printf("\n    ");
            GenerateStatement(op->true_statement);
        }

		if (op->false_statement != nullptr)
		{
			printf("%selse\n", NewLine);
			GenerateStatement(op->false_statement);
		}
		return;
	}

	case statement_t::_switch:
	{
		operator_SWITCH * op = (operator_SWITCH*)code;
		printf("switch(");
		GenerateExpression(op->expression);
		printf(")");
		PrintOpenBlock();
		GenerateSpace(op->body);
		PrintCloseBlock();
		return;
	}

	case statement_t::_case:
	{
		operator_CASE * op = (operator_CASE *)code;
		printf("case %d:\n", op->case_value);
		return;
	}

	case statement_t::_continue:
		printf("continue");
		break;

	case statement_t::_break:
		printf("break");
		break;

	case statement_t::_default:
		printf("default:\n");
		return;

	case statement_t::_goto:
	{
		operator_GOTO * op = (operator_GOTO*)code;
		printf("goto %s", op->label.c_str());
		break;
	}
	case statement_t::_label:
	{
		operator_LABEL * op = (operator_LABEL*)code;
		printf("%s:\n", op->label.c_str());
		return;
	}
	case statement_t::_delete:
	{
		operator_DELETE * op = (operator_DELETE*)code;
		printf("delete %s %s", op->as_array ? "[]" : "", op->var->name.c_str());
		break;
	}
	case statement_t::_variable:
	{
		variable_base_t * var = (variable_base_t *)code;
		switch (var->storage)
		{
		case linkage_t::sc_extern:
			printf("extern ");
			break;
		case linkage_t::sc_static:
			printf("static ");
			break;
		case linkage_t::sc_register:
			printf("register ");
			break;
		}
		if (var->type->name.size() > 0)
			GenerateTypeName(var->type, (char*)var->name.c_str());
		else
		{
			// Unnamed structure - draw full type
			GenerateType(var->type, true);
			printf(" %s", var->name.c_str());
		}
		if (var->declaration != nullptr)
		{
			switch (var->declaration->type)
			{
			case statement_t::_expression:
				printf("= ");
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
			printf(" = ");
			GenerateStaticData(var->static_data, true);
		}
		break;
	}
	case statement_t::_for:
	{
		operator_FOR * op = (operator_FOR *)code;
		printf("for( ");
		if (op->init_type)
			GenerateTypeName(op->init_type, nullptr);
		if (op->init != nullptr)
			GenerateExpression(op->init);
		printf("; ");
		if (op->condition != nullptr)
			GenerateExpression(op->condition);
		printf("; ");
		if (op->iterator != nullptr)
			GenerateExpression(op->iterator);
		printf(")");
		PrintOpenBlock();
		GenerateSpace(op->body);
		PrintCloseBlock();
		return;
	}
	case statement_t::_codeblock:
	{
		PrintOpenBlock();
		codeblock_t * op = (codeblock_t*)code;
		GenerateSpace(op->block_space);
		PrintCloseBlock();
		return;
	}
	case statement_t::_trycatch:
	{
		operator_TRY * op = (operator_TRY*)code;
		printf("try ");
		PrintOpenBlock();
		GenerateSpace(op->body);
		PrintCloseBlock();
		printf("%scatch( ", NewLine);
		GenerateTypeName(op->type, op->exception.c_str());
		//printf(op->exception.c_str());
		printf(") ");
		PrintOpenBlock();
		GenerateSpace(op->handler->space);
		PrintCloseBlock();
		return;
	}
	case statement_t::_throw:
	{
		operator_THROW * op = (operator_THROW *)code;
		printf("throw \"%s\"", op->exception_constant->char_pointer);
		break;
	}
	default:
		throw "Not parsed statement type";
	}
	printf(";\n");

}

void GenerateFunctionOverload(function_overload_t * overload, bool proto)
{
	printf(NewLine);

	switch (overload->linkage.storage_class)
	{
	case linkage_t::sc_extern:
		printf("extern ");
		break;
	case linkage_t::sc_static:
		printf("static ");
		break;
	case linkage_t::sc_abstract:
	case linkage_t::sc_virtual:
		printf("virtual ");
		break;
	case linkage_t::sc_inline:
		printf("inline ");
		break;
	}
	switch (overload->function->method_type)
	{
	case function_parser::method:
		GenerateTypeName(overload->function->type, nullptr);
		break;
	case function_parser::constructor:
		printf("// Constructor\n");
		printf(NewLine);
		break;
	case function_parser::destructor:
		printf("// Desctructor\n");
		printf(NewLine);
		break;
	}
	if (!proto)
	{
		// Check class name. TODO Check namespaces
		if (overload->space != nullptr) // <-------------- This is hack. Must be eliminated
			if (overload->space->parent != nullptr &&
				(overload->space->parent->type == space_t::structure_space) &&
				(overload->linkage.storage_class != linkage_t::sc_inline))
			{
				printf("%s", overload->space->parent->name.c_str());
			}
	}
	printf("%s (", overload->function->name.c_str());
#if MODERN_COMPILER
    for (farg_t & arg : overload->arguments)
	{
		if (arg.type != nullptr)
		{
			GenerateTypeName(arg.type, (char*)arg.name.c_str());
			if (&arg != &overload->arguments.back())
				printf(", ");
		}
		else
			printf("...");
	}
#else
    arg_list_t::iterator    arg;
    for (
        arg = overload->arguments.begin();
        arg != overload->arguments.end();
        ++arg)
    {
        if (arg->type != nullptr)
        {
            GenerateTypeName(arg->type, (char*)arg->name.c_str());
            if (&(*arg) != &overload->arguments.back())
                printf(", ");
        }
        else
            printf("...");
    }
#endif
	printf(")");
	if (overload->space != nullptr && !proto)
	{
		printf("%s", NewLine);
		PrintOpenBlock();
		if (overload->space->space_code.size() > 0)
			GenerateSpace(overload->space);
		PrintCloseBlock();
	}
	else if (overload->linkage.storage_class == linkage_t::sc_abstract)
		printf(" = 0;\n");
	else
		printf(";\n");
}

void GenerateFunction(function_parser * f, bool proto)
{
#if MODERN_COMPILER
    for (function_overload_t * overload : f->overload_list)
	{
		GenerateFunctionOverload(overload, proto);
	}
#else
    function_overload_list_t::iterator  overload;
    for (
        overload = f->overload_list.begin();
        overload != f->overload_list.end();
        ++overload)
    {
        GenerateFunctionOverload(*overload, proto);
    }
#endif
}

void GenerateType(type_t * type, bool inlined_only)
{
	if (type->prop == type_t::template_type)
	{
		printf("%stemplate<class %s>%s", NewLine, type->name.c_str(), "FiXME");
		template_t * temp = (template_t *)type;
		GenerateSpace(temp->space);
		printf(" ");
	}
	else if (type->prop == type_t::pointer_type)
	{
		printf(" * ");
		pointer_t * op = (pointer_t*)type;
		GenerateType(op->parent_type, inlined_only);
	}
	else if (type->prop == type_t::funct_ptr_type)
	{
		function_parser * func_ptr = (function_parser*)type;
		GenerateTypeName(func_ptr->type, nullptr);
		printf("%s(", func_ptr->name.c_str());
#if MODERN_COMPILER
		for (auto func : func_ptr->overload_list)
		{
			//			function_parser * func = (function_parser*)func_ptr;
			for (auto & arg : func->arguments)
			{
				GenerateTypeName(arg.type, arg.name.c_str());
				if (&arg != &func->arguments.back())
					printf(", ");
			}
		}
#else
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
                GenerateTypeName(arg->type, arg->name.c_str());
                if (&arg != &(*func)->arguments.begin())
                    printf(", ");
            }
        }
#endif
		printf(")");
	}
	else if (type->prop == type_t::compound_type)
	{
		structure_t * compound_type = (structure_t*)type;
		GenerateTypeName(type, nullptr);
		if (compound_type->space->inherited_from.size() > 0)
		{
			printf(": ");
#if MODERN_COMPILER
            for (auto & inherited_from : compound_type->space->inherited_from)
			{
				printf("%s", inherited_from->name.c_str());
				printf(&inherited_from != &compound_type->space->inherited_from.back() ? ", " : " ");
			}
#else
            std::list<structure_t*>::iterator inherited_from;
            for (
                inherited_from = compound_type->space->inherited_from.begin();
                inherited_from != compound_type->space->inherited_from.end();
                ++inherited_from)
            {
                printf("%s", (*inherited_from)->name.c_str());
                printf(&(*inherited_from) != &compound_type->space->inherited_from.back() ? ", " : " ");
            }
#endif
        }
		PrintOpenBlock();
		GenerateSpace(compound_type->space);
		if (inlined_only)
			printf("} ");
		else
		{
			PrintCloseBlock();
			// Definition Outside class declaration
#if MODERN_COMPILER
            for (auto f : compound_type->space->function_list)
				for (auto overload : f->overload_list)
					if (overload->space != nullptr && overload->linkage.storage_class != linkage_t::sc_inline)
						GenerateFunction(f, false);
#else
            std::list<class function_parser*>::iterator f;
            for (
                f = compound_type->space->function_list.begin();
                f != compound_type->space->function_list.end();
                ++f)
            {
                function_overload_list_t::iterator  overload;
                for (
                    overload = (*f)->overload_list.begin();
                    overload != (*f)->overload_list.end();
                    ++overload)
                {
                    if ((*overload)->space != nullptr && (*overload)->linkage.storage_class != linkage_t::sc_inline)
                        GenerateFunction(*f, false);
                }
            }
#endif
		}
	}
	else if (type->prop == type_t::enumerated_type)
	{
		enumeration_t * en = (enumeration_t*)type;
		printf("enum %s ", type->name.c_str());
		PrintOpenBlock();
		int idx = 0;
#if MODERN_COMPILER
        for (auto item : en->enumeration)
		{
			if (item.second == idx)
				printf("%s%s,\n", NewLine, item.first.c_str());
			else
			{
				idx = item.second;
				printf("%s%s = %d,\n", NewLine, item.first.c_str(), idx);
			}
			++idx;
		}
#else
        std::map<std::string, int>::iterator item;
        for (
            item = en->enumeration.begin(); 
            item != en->enumeration.end();
            ++item)
        {
            if (item->second == idx)
                printf("%s%s,\n", NewLine, item->first.c_str());
            else
            {
                idx = item->second;
                printf("%s%s = %d,\n", NewLine, item->first.c_str(), idx);
            }
            ++idx;
        }
#endif
		if (inlined_only)
			printf("} ");
		else
			PrintCloseBlock();
	}
	else if (type->prop == type_t::dimension_type)
	{
		array_t * array = (array_t*)type;
		GenerateTypeName(array->child_type, nullptr);
		printf("[%d] ", array->items_count);
	}
	else if (type->prop == type_t::typedef_type)
	{
		typedef_t * def = (typedef_t *)type;
		printf("%stypedef ", NewLine);
		if (def->type->prop != type_t::enumerated_type)
		{
			GenerateType(def->type, false);
			printf("%s%s;\n", NewLine, type->name.c_str());
		}
		else
		{
			if (def->type->name.size() > 0)
				GenerateTypeName(def->type, type->name.c_str());
			else
			{
				GenerateType(def->type, true);
				printf(" %s", type->name.c_str());
			}
			printf(";\n");
		}
	}
	else if (type->prop == type_t::auto_type)
	{
		printf("/* AUTO */");
	}
	else
	{
		throw "\n--------- TODO: Fix source generator ---------\n";
	}
}

void GenerateSpace(namespace_t * space)
{
#if MODERN_COMPILER
    for (auto f : space->function_list)
		//		if (f->space == nullptr)
		GenerateFunction(f, true);
    if (space->space_types_list.size() > 0)
    {
        //		printf("/* Space type definitions '%s' */\n", space->name.c_str());
        for (auto type : space->space_types_list)
            GenerateType(type, false);
    }

    /* Do variables declaration as statements */
    if (space->space_code.size() > 0)
    {
        //		printf("/* Code definitions */\n");
        for (auto code : space->space_code)
            GenerateStatement(code);
    }

    if (space->function_list.size() > 0)
    {
        //		printf("/* Method definitions */\n");
        for (auto f : space->function_list)
            for (auto overload : f->overload_list)
            {
                if (overload->linkage.storage_class == linkage_t::sc_inline || space->type != space_t::spacetype_t::structure_space)
                {
                    if (overload->space != nullptr)
                    {
                        bool proto = false;
                        if (overload->space->parent->type == space_t::spacetype_t::structure_space)
                            proto = overload->linkage.storage_class != linkage_t::sc_inline;
                        GenerateFunctionOverload(overload, proto);
                    }
                }
            }
    }
#else
    std::list<class function_parser*>::iterator f;
    for (
        f = space->function_list.begin();
        f != space->function_list.end();
        ++f)
        //		if (f->space == nullptr)
        GenerateFunction(*f, true);
    if (space->space_types_list.size() > 0)
    {
        //		printf("/* Space type definitions '%s' */\n", space->name.c_str());
        std::list<type_t*>::iterator type;
        for (
            type = space->space_types_list.begin();
            type != space->space_types_list.end();
            ++type)
        {
            GenerateType(*type, false);
        }
    }

    /* Do variables declaration as statements */
    if (space->space_code.size() > 0)
    {
        //		printf("/* Code definitions */\n");
        std::list<statement_t *>::iterator code;
        for (
            code = space->space_code.begin();
            code != space->space_code.end();
            ++code)
        {
            GenerateStatement(*code);
        }
    }

    if (space->function_list.size() > 0)
    {
        //		printf("/* Method definitions */\n");
        std::list<class function_parser*>::iterator f;
        for (
            f = space->function_list.begin();
            f != space->function_list.end();
            ++f)
        {
            function_overload_list_t::iterator overload;
            for (
                overload = (*f)->overload_list.begin();
                overload != (*f)->overload_list.end();
                ++overload)
            {
                if ((*overload)->linkage.storage_class == linkage_t::sc_inline || space->type != space_t::structure_space)
                {
                    if ((*overload)->space != nullptr)
                    {
                        bool proto = false;
                        if ((*overload)->space->parent->type == space_t::structure_space)
                            proto = (*overload)->linkage.storage_class != linkage_t::sc_inline;
                        GenerateFunctionOverload(*overload, proto);
                    }
                }
            }
        }
    }
#endif

}

void restore_source_t::GenerateCode(namespace_t * space)
{
	::GenerateSpace(space);
}
