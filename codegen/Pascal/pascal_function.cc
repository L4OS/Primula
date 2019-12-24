#include "pascal_generator.h"

void PascalGenerator::GenerateArgumentDefinition(variable_base_t * arg)
{
    type_t  * tptr = arg->type;
    if (tptr->prop == type_t::constant_type)
    {
        Write("const ");
        tptr = ((const_t*)tptr)->parent_type;
    }
    Write("%s: ", arg->name.c_str());
    GenerateTypeName(tptr);
}

void PascalGenerator::GenerateFunctionOverload(function_overload_t * overload, bool proto)
{
    if (overload->function->type->prop == type_t::void_type)
        IndentWrite("procedure ");
    else
        IndentWrite("function ");


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
    }

    if(overload->linkage.inlined)
        printf("inline ");

    if (!proto)
    {
        // Check class name. TODO Check namespaces
        if (overload->space != nullptr) // <-------------- This is hack. Must be eliminated
            if (overload->space->parent != nullptr &&
                (overload->space->parent->type == space_t::structure_space) &&
                (!overload->linkage.inlined))
            {
                Write("%s", overload->space->parent->name.c_str());
            }
    }
    Write("%s (", overload->function->name.c_str());

    arg_list_t::iterator    arg;
    for (
        arg = overload->arguments.begin();
        arg != overload->arguments.end();
        ++arg)
    {
        if (arg->type != nullptr)
        {
#if true
            GenerateArgumentDefinition(&(*arg));
#else
            Write("%s: ", arg->name.c_str());
            GenerateTypeName(arg->type);
#endif
            if (&(*arg) != &overload->arguments.back())
                Write("; ");
        }
        else
            Write("...");
    }

    switch (overload->function->method_type)
    {
    case function_parser::method:
        if (overload->function->type->prop == type_t::void_type)
        {
            Write(");");
            break;
        }
        Write("): ");
        GenerateTypeName(overload->function->type);
        Write(";");
        break;
    case function_parser::constructor:
        Write("// Constructor\n");
        break;
    case function_parser::destructor:
        Write("// Desctructor\n");
        break;
    }

    if (overload->space != nullptr && !proto)
    {
        Write("\n");
        if (overload->space->space_variables_list.size() > 0)
        {
            IndentWrite("var\n");
            MoveIndentRight();
            for (auto var : overload->space->space_variables_list)
            {
                IndentWrite("%s: ", var->name.c_str());
                GenerateTypeName(var->type);
                Write(";\n");
            }
            MoveIndentLeft();
        }
        if (overload->space->space_code.size() > 0)
        {
            PrintOpenBlock();
            ((PascalGenerator*)overload->space)->GenerateSpace();
            PrintCloseBlock();
        }
        Write("\n");
    }
    else if (overload->linkage.storage_class == linkage_t::sc_abstract)
        printf(" = 0;\n");
    else
        Write("\n");
}

