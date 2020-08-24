#include "restore_source_t.h"

bool     restore_source_t::opt_HideUnusedPrototypes;

static char INDENT[80];
static int	right = 0;

#pragma region Code Writer

static void MoveIndentRight()
{
    INDENT[right] = ' ';
    right += 4;
    if (right > sizeof(INDENT) - 1)
        throw "Block overflow on restoring source code";
    INDENT[right] = '\0';
}

static void MoveIndentLeft()
{
    INDENT[right] = ' ';
    right -= 4;
    if (right < 0)
        throw "Block underflow on restoring source code";
    INDENT[right] = '\0';
}

static void Write(const char * s, ...)
{
    va_list args;
    va_start(args, s);
    vprintf(s, args);
    va_end(args);
}

static void IndentWrite(const char *s, ...)
{
    printf( (const char*) INDENT);

    va_list args;
    va_start(args, s);
    vprintf(s, args);
    va_end(args);
}


static void PrintOpenBlock()
{
    IndentWrite("{\n");
    MoveIndentRight();
}

static void PrintCloseBlock(bool lineFeed)
{
    MoveIndentLeft();
    IndentWrite("}%s", lineFeed ? "\n" : " ");
}

#pragma endregion Code Writer

restore_source_t::restore_source_t()
{
    opt_HideUnusedPrototypes = true;

    INDENT[0] = '\0';
    int i;
    for (i = 1; i < sizeof(INDENT); ++i)
        INDENT[i] = ' ';
}

restore_source_t::~restore_source_t()
{
}

bool GenerateTypeName(type_t * type, const char * name);
void GenerateExpression(expression_t * exp);

void GenerateCall(call_t * call)
{
    if (call->code == nullptr)
    {
        Write("// Parser fault on function call :(\n");
        return;
    }
    if (call->code->function->method_type == function_parser::method)
        Write("%s", call->code->function->name.c_str());
    Write("(");
#if MODERN_COMPILER
    for (auto & arg : call->arguments)
    {
        GenerateExpression(arg);
        if (arg != call->arguments.back())
            printf(", ");
    }
#else
    std::list<expression_t *>::iterator arg;
    for (arg = call->arguments.begin(); arg != call->arguments.end(); ++arg)
    {
        GenerateExpression(*arg);
        if (&(*arg) != &call->arguments.back())
            Write(", ");
    }
#endif
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

void GenerateSpaceName(namespace_t * space)
{
    if (space->type == space_t::name_space)
    {
        if (space->parent && space->parent->type == space_t::name_space)
            GenerateSpaceName(space->parent);
        Write("%s::", space->name.c_str());
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
        str = "this";
        break;
    case lt_variable:
    {
        namespace_t * space = n->variable->space;
        if (space != nullptr)
        {
            if (space->type == space_t::name_space)
            {
                GenerateSpaceName(n->variable->space);
            }
            else if (space->type == space_t::global_space)
            {
                Write("::");
            }
        }

        Write("%s", n->variable->name.c_str());
        break;
    }
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
        Write("%d", n->value.integer_value);
        break;
    case lt_floatnumber:
        Write("%f", n->value.float_value);
        break;
    case lt_string:
        Write("\"%s\"", n->value.char_pointer);
        break;
    case lt_character:
        PrintCharacter(n->value.integer_value);
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
        Write(" [ ");
        GenerateItem(n->right);
        Write(" ] ");
        return;
    }
    break;

    case lt_typecasting:
    {
        Write("(");
        GenerateTypeName(n->type, nullptr);
        Write(") ");
        break;
    }

    case lt_new:
    {
        Write(" new ");
        type_t * type = n->type;
        switch (type->prop)
        {
        case type_t::pointer_type:
            type = ((pointer_t*)type)->parent_type;
            break;
        case type_t::dimension_type:
            type = ((array_t*)type)->child_type;
            break;
        default:
            throw "type must be pointer or array";
        }
        GenerateTypeName(type, nullptr);
        if (n->right != nullptr)
        {
            if (n->right->lexem == lt_argument)
            {
                Write("(");
                GenerateItem(n->right);
                Write(")");
                return;
            }
            Write("[");
            GenerateItem(n->right);
            Write("]");
        }
        return;
    }

    case lt_function_address:
    {
        function_parser * function = (function_parser*)n->type;
        Write("%s;", function->name.c_str());
        break;
    }

    case lt_operator_postinc:
    case lt_operator_postdec:
    {
        GenerateItem(n->right);
        Write(n->lexem == lt_operator_postinc ? "++ " : "-- ");
        return;
    }

    case lt_argument:
        if(n->right != nullptr)
            Write(", ");
        break;

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
                Write("* %s", name);
            else
                Write("* ");
            break;
        }
        case type_t::funct_ptr_type:
        {
            function_parser * func_ptr = (function_parser*)type;
            Write("%s", func_ptr->name.c_str());
            if (name)
                Write(" %s", name);
            else
                Write(" ");
            break;
        }
        case type_t::constant_type:
        {
            const_t * constant = (const_t*)type;
            if (constant->parent_type == nullptr)
            {
                throw "constant syntax error";
            }
            Write("const ");
            GenerateTypeName(constant->parent_type, name);
            break;
        }
        case type_t::dimension_type:
        {
            array_t * array = (array_t*)type;
            skip_name = GenerateTypeName(array->child_type, name);
            if (skip_name)
                Write("[%d] ", array->items_count);
            else
                Write("%s [%d] ", name, array->items_count);
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
                Write("%s %s %s ", compound_name, type->name.c_str(), name);
            else
                Write("%s %s ", compound_name, type->name.c_str());
            break;
        }

        case type_t::enumerated_type:
        {
            if (name)
                Write("enum %s %s ", type->name.c_str(), name);
            else
                Write("enum %s ", type->name.c_str());
            break;
        }

        default:
            if (name)
                Write("%s %s", type->name.c_str(), name);
            else
                Write("%s ", type->name.c_str());
        }
    }
    return true; // skip_name;
}

void GenerateStaticData(static_data_t * data, bool last, bool selfformat = false)
{
    switch (data->type)
    {
    case lt_openblock:
        //structure_t * structure = (structure_t *)data->nested;
        if (selfformat)
            Write("{");
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
            std::list<struct static_data*>::iterator compound = data->nested->begin();
            compound != data->nested->end();
            ++compound)
        {
            if (!selfformat)
                Write("%s", INDENT);
            if (*compound == nullptr)
            {
                Write("/* something goes wrong */ 0");
            }
            else
            {
                bool last = (*compound == data->nested->back());
                GenerateStaticData(*compound, last, true);
            }
        }
#endif
        if (selfformat)
            Write("}");
        else
            PrintCloseBlock(false);
        break;
    case lt_string:
        Write("\"%s\"", data->p_char);
        break;
    case lt_integer:
        Write("%d", data->s_int);
        break;
    case lt_openindex:
        if (data->nested->size() == 0)
            Write("\n /* Nested data size is not defined */\n");
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
        Write("}");
        break;
    default:
        throw "Static data type not parsed";
    }
    if (!last)
        Write(",\n");
    else
        Write("\n");
}

void restore_source_t::GenerateFunctionOverload(function_overload_t * overload, bool proto)
{
    Write(INDENT);

    switch (overload->linkage.storage_class)
    {
    case linkage_t::sc_default:
        break;
    case linkage_t::sc_extern:
        Write("extern ");
        break;
    case linkage_t::sc_static:
        Write("static ");
        break;
    case linkage_t::sc_abstract:
    case linkage_t::sc_virtual:
        Write("virtual ");
        break;
    default:
        throw "Wrong storage class";
    }
    if (overload->linkage.inlined)
    {
        Write("inline ");
    }
    switch (overload->function->method_type)
    {
    case function_parser::method:
        GenerateTypeName(overload->function->type, nullptr);
        break;
    case function_parser::constructor:
        Write("// Constructor\n%s", INDENT);
        break;
    case function_parser::destructor:
        Write("// Desctructor\n%s", INDENT);
        break;
    }
    if (!proto)
    {
        // Check class name. TODO Check namespaces
        if (overload->space != nullptr) // <-------------- This is hack. Must be eliminated
            if (overload->space->parent != nullptr &&
                (overload->space->parent->type == space_t::structure_space))
            {
                Write("%s", overload->space->parent->name.c_str());
            }
    }
    Write("%s(", overload->function->name.c_str());
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
        if ((*arg)->type != nullptr)
        {
            GenerateTypeName((*arg)->type, (char*)(*arg)->name.c_str());
            if (&(*arg) != &overload->arguments.back())
                Write(", ");
        }
        else
            Write("...");
    }
#endif
    Write(")");
    if (overload->space != nullptr && !proto)
    {
        Write("\n");
        PrintOpenBlock();
        if (overload->space && overload->space->space_code.size() > 0)
            ((restore_source_t*)overload->space)->GenerateSpace();
        PrintCloseBlock(true);
    }
    else if (overload->linkage.storage_class == linkage_t::sc_abstract)
        Write(" = 0;\n");
    else
        Write(";\n");
}

void restore_source_t::GenerateFunction(function_parser * f, bool is_method, bool proto)
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
        if (type == space_t::structure_space ||
            !opt_HideUnusedPrototypes ||
            (*overload)->access_count > 0)
        {
            if ((*overload)->linkage.inlined ^ proto)
                GenerateFunctionOverload(*overload, proto);
        }
        //else
        //{
        //    printf("// ");
        //    GenerateFunctionOverload(*overload, proto);
        //}
    }
    if (!proto)
        Write("\n");
#endif
}

void restore_source_t::GenerateType(type_t * type, bool inlined_only)
{
    switch (type->prop)
    {
    case type_t::template_type:
    {
        Write("template<class %s>%s", type->name.c_str(), "FiXME");
        template_t * temp = (template_t *)type;
        ((restore_source_t*)temp->space)->GenerateSpace();
        printf(" ");
        break;
    }
    case type_t::pointer_type:
    {
        Write(" * ");
        pointer_t * op = (pointer_t*)type;
        GenerateType(op->parent_type, inlined_only);
        break;
    }
    case type_t::funct_ptr_type:
    {
        function_parser * func_ptr = (function_parser*)type;
        GenerateTypeName(func_ptr->type, nullptr);
        Write("%s(", func_ptr->name.c_str());
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
            ++func)
        {
            //			function_parser * func = (function_parser*)func_ptr;
            arg_list_t::iterator arg;
            for (
                arg = (*func)->arguments.begin();
                arg != (*func)->arguments.end();
                ++arg)
            {
                GenerateTypeName((*arg)->type, (*arg)->name.c_str());
                if (arg != (*func)->arguments.begin())
                    Write(", ");
            }
        }
#endif
        Write(")");
        break;
    }
    case type_t::compound_type:
    {
        structure_t * compound_type = (structure_t*)type;
        Write(INDENT);
        GenerateTypeName(type, nullptr);
        if (compound_type->space->inherited_from.size() > 0)
        {
            Write(": ");
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
                Write("%s", (*inherited_from)->name.c_str());
                Write(&(*inherited_from) != &compound_type->space->inherited_from.back() ? ", " : " ");
            }
#endif
        }
        PrintOpenBlock();
        ((restore_source_t*)compound_type->space)->GenerateSpace();
        if (inlined_only)
        {
            MoveIndentLeft();
            IndentWrite("} ");
        }
        else
        {
            PrintCloseBlock(false);
            Write(";\n\n");
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
                    if ((*overload)->space != nullptr && (*overload)->linkage.inlined == false)
                        GenerateFunctionOverload(*overload, false);
                }
            }
#endif
        }
        break;
    }
    case type_t::enumerated_type:
    {
        enumeration_t * en = (enumeration_t*)type;
        Write("enum %s ", type->name.c_str());
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
                IndentWrite("%s,\n", item->first.c_str());
            else
            {
                idx = item->second;
                IndentWrite("%s = %d,\n", item->first.c_str(), idx);
            }
            ++idx;
        }
#endif
        if (inlined_only)
        {
            MoveIndentLeft();
            IndentWrite("} ");
        }
        else
            PrintCloseBlock(false);
        break;
    }
    case type_t::dimension_type:
    {
        array_t * array = (array_t*)type;
        GenerateTypeName(array->child_type, nullptr);
        Write("[%d] ", array->items_count);
        break;
    }
    case type_t::typedef_type:
    {
        typedef_t * def = (typedef_t *)type;
        IndentWrite("typedef ");
        if (def->type->prop != type_t::enumerated_type)
        {
            GenerateType(def->type, true);
            IndentWrite("%s;\n", type->name.c_str());
        }
        else
        {
            if (def->type->name.size() > 0)
                GenerateTypeName(def->type, type->name.c_str());
            else
            {
                GenerateType(def->type, false);
                Write(" %s", type->name.c_str());
            }
            Write(";\n");
        }
        break;
    }
    case type_t::auto_type:
    {
        Write("/* AUTO */");
        break;
    }
    default:
    {
        throw "\n--------- TODO: Fix source generator ---------\n";
    }
    }
}

void restore_source_t::GenerateStatement(statement_t * code)
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
        IndentWrite("return ");
        if (value)
            GenerateExpression(value);
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
            Write("// !!! empty expression !!!\n");
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
        IndentWrite("do\n");
        bool block = (op->body->type == statement_t::_codeblock);
        if (!block)
            MoveIndentRight();
        GenerateStatement(op->body);
        if (!block)
            MoveIndentLeft();
        IndentWrite("while(");
        GenerateExpression(op->expression);
        Write(");\n");
        return;
    }

    case statement_t::_while:
    {
        operator_WHILE * op = (operator_WHILE*)code;
        IndentWrite("while( ");
        GenerateExpression(op->expression);
        Write(" )\n");
        bool blocked = op->body->type == statement_t::_codeblock;
        if (!blocked)
            MoveIndentRight();
        GenerateStatement(op->body);
        if (!blocked)
            MoveIndentLeft();
        return;
    }

    case statement_t::_if:
    {
        operator_IF * op = (operator_IF*)code;
        IndentWrite("if(");
        if (op->expression == nullptr)
            Write("/* error expression parsing */");
        else
            GenerateExpression(op->expression);
        Write(")\n");
        if (op->true_statement == nullptr)
        {
            Write("/* true 'if' statements lost */\n");
        }
        else
        {
            bool block = op->true_statement->type == statement_t::_codeblock;
            if (!block)
                MoveIndentRight();
            GenerateStatement(op->true_statement);
            if (!block)
                MoveIndentLeft();
        }

        if (op->false_statement != nullptr)
        {
            IndentWrite("else\n");
            bool block = op->false_statement->type == statement_t::_codeblock;
            if (!block)
                MoveIndentRight();
            GenerateStatement(op->false_statement);
            if (!block)
                MoveIndentLeft();
        }
        return;
    }

    case statement_t::_switch:
    {
        operator_SWITCH * op = (operator_SWITCH*)code;
        IndentWrite("switch(");
        GenerateExpression(op->expression);
        Write(")\n");
        PrintOpenBlock();
        ((restore_source_t*)op->body)->GenerateSpace();
        PrintCloseBlock(true);
        return;
    }

    case statement_t::_case:
    {
        operator_CASE * op = (operator_CASE *)code;
        IndentWrite("case %d:\n", op->case_value);
        return;
    }

    case statement_t::_continue:
        IndentWrite("continue");
        break;

    case statement_t::_break:
        IndentWrite("break");
        break;

    case statement_t::_default:
        IndentWrite("default:\n");
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
        Write(INDENT);
        switch (var->storage)
        {
        case linkage_t::sc_extern:
            Write("extern ");
            break;
        case linkage_t::sc_static:
            Write("static ");
            break;
        case linkage_t::sc_register:
            Write("register ");
            break;
        }
        if (var->type->name.size() > 0)
            GenerateTypeName(var->type, (char*)var->name.c_str());
        else
        {
            // Unnamed structure - draw full type
            GenerateType(var->type, true);
            Write(" %s", var->name.c_str());
        }
        if (var->declaration != nullptr)
        {
            switch (var->declaration->type)
            {
            case statement_t::_expression:
                Write("= ");
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
        IndentWrite("for( ");
        if (op->init_type)
            GenerateTypeName(op->init_type, nullptr);
        if (op->init != nullptr)
            GenerateExpression(op->init);
        Write("; ");
        if (op->condition != nullptr)
            GenerateExpression(op->condition);
        Write("; ");
        if (op->iterator != nullptr)
            GenerateExpression(op->iterator);
        Write(")\n");
        PrintOpenBlock();
        if (op->body != nullptr)
            ((restore_source_t*)op->body)->GenerateSpace();
        PrintCloseBlock(true);
        return;
    }
    case statement_t::_codeblock:
    {
        PrintOpenBlock();
        codeblock_t * op = (codeblock_t*)code;
        ((restore_source_t*)op->block_space)->GenerateSpace();
        PrintCloseBlock(true);
        return;
    }
    case statement_t::_trycatch:
    {
        operator_TRY * op = (operator_TRY*)code;
        IndentWrite("try ");
        PrintOpenBlock();
        if (op->body != nullptr)
            ((restore_source_t*)op->body)->GenerateSpace();
        PrintCloseBlock(true);
        IndentWrite("catch( ");
        GenerateTypeName(op->type, op->exception.c_str());
        //printf(op->exception.c_str());
        Write(")\n");
        PrintOpenBlock();
        if (op->handler->space != nullptr)
            ((restore_source_t*)op->handler->space)->GenerateSpace();
        PrintCloseBlock(true);
        return;
    }
    case statement_t::_throw:
    {
        operator_THROW * op = (operator_THROW *)code;
        IndentWrite("throw \"%s\"", op->exception_constant->char_pointer);
        break;
    }
    default:
        throw "Not parsed statement type";
    }
    Write(";\n");
}

void restore_source_t::GenerateFunctionPrototypes(bool is_methods)
{
    std::list<class function_parser*>::iterator f;
    for (f = function_list.begin(); f != function_list.end(); ++f)
    {
        GenerateFunction(*f, is_methods, true);
    }
}

void restore_source_t::GenerateSpaceTypes()
{
    if (space_types_list.size() > 0)
    {
        std::list<type_t*>::iterator type;
        for (
            type = space_types_list.begin();
            type != space_types_list.end();
            ++type)
        {
            GenerateType(*type, false);
            Write("\n");
        }
    }
}

void restore_source_t::GenerateSpaceCode()
{
    /* Do variables declaration as statements */
    if (space_code.size() > 0)
    {
        //		printf("/* Code definitions */\n");
        std::list<statement_t *>::iterator code;
        for (
            code = space_code.begin();
            code != space_code.end();
            ++code)
        {
            if (*code != nullptr)
                GenerateStatement(*code);
            else
                printf("// code empty\n");
        }
    }
}

void restore_source_t::GenerateSpaceFunctions(bool definition)
{
    if (function_list.size() > 0)
    {
        std::list<class function_parser*>::iterator f;
        for (f = function_list.begin(); f != function_list.end(); ++f)
        {
            function_overload_list_t::iterator overload;
            for (
                overload = (*f)->overload_list.begin();
                overload != (*f)->overload_list.end();
                ++overload)
            {
                namespace_t * space = (*overload)->space;
                if (space != nullptr)
                {
                    linkage_t * linkage = &(*overload)->linkage;
                    if (definition)
                    {
                        GenerateFunctionOverload(*overload, !linkage->inlined && this->type == spacetype_t::structure_space);
                    }
                    else
                    {
                        GenerateFunctionOverload(*overload, linkage->inlined);
                    }
                }
                else 
                {
                    if (!definition)
                        GenerateFunctionOverload(*overload, true);
                    else
                    {
                        GenerateFunctionOverload(*overload, false);
                    }
                }
            }
        }
    }
}

void restore_source_t::GenerateEmbeddedSpaces()
{
    std::map<std::string, namespace_t *>::iterator  space;
    space = this->embedded_space_map.begin();
    while (space != this->embedded_space_map.end())
    {
        IndentWrite("namespace %s\n", space->second->name.c_str());
        PrintOpenBlock();
        ((restore_source_t*)space->second)->GenerateSpace();
        PrintCloseBlock(true);
        ++space;
    }
}

void restore_source_t::GenerateSpace()
{
    this->GenerateSpaceTypes();
    this->GenerateEmbeddedSpaces();
    bool is_methods = this->type == spacetype_t::structure_space;
    if (!is_methods)
    {
        this->GenerateFunctionPrototypes(is_methods);
        this->GenerateSpaceCode();
    }
    else
    {
        this->GenerateSpaceCode();
    }
    this->GenerateSpaceFunctions(true);
}

///
/// Main entry point to source restorer
///    It is important to have type of the space argument to be restore_source_t type.
///
void restore_source_t::GenerateCode(namespace_t * space)
{
    typedef restore_source_t    *   this_t;
    this_t(space)->GenerateSpace();
}
