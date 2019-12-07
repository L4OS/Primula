#include "namespace.h"

#pragma region Static

static static_data_t *	root = nullptr;

static static_data_t * String(type_t * type, Code::lexem_node_t * node)
{
    static_data_t * data = nullptr;
    int ptr_count = 0;
    array_t * array = nullptr;
    while (true)
    {
        switch (type->prop)
        {
        case type_t::pointer_type:
            ptr_count++;
            type = ((pointer_t*)type)->parent_type;
            continue;
        case type_t::constant_type:
            type = ((const_t*)type)->parent_type;
            continue;
        case type_t::typedef_type:
            type = ((typedef_t*)type)->type;
            continue;
        case type_t::dimension_type:
            array = (array_t*)type;
            if (array->child_type->bitsize != 8)
                throw "utf16/utf32 strings not supported in this version";
            if (array->items_count < std::string(node->value).length() + 1)
                throw "Array size too small to keep constant string";
            type = array->child_type;
        }
        break;
    }

    if (ptr_count > 0 && array != nullptr)
        throw "unable cast const char[n] to char *[n] ";
    if (ptr_count == 0 && array == nullptr)
        throw "unable cast const char[n] to char ";

    if (type->bitsize == 8 && (type->prop == type->signed_type || type->prop == type->unsigned_type))
    {
        data = new static_data(node->value);
    }
    return data;
}

static static_data_t * Integer(type_t * type, Code::lexem_node_t * node)
{
    static_data_t	*	data = nullptr;
    int					base;

    std::string		num(node->value);
    if (num[0] != '0' || num.length() == 1)
        base = 10;
    else if (num.length() > 2)
    {
        switch (num[1])
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            base = 8;
            break;
        case 'X':
        case 'x':
            base = 16;
            break;
        case 'b':
            base = 2;
            break;
        default:
            throw "Integer format error";
        }
    }

    switch (type->prop)
    {
    case type->signed_type:
        data = new static_data_t(std::stoi(num, nullptr, base));
        break;
    case type->unsigned_type:
        data = new static_data_t((unsigned int)std::stoi(num, nullptr, base));
        break;
    }
    return data;
}

#pragma endregion Static

//static_data_t * BraceEncodedStructureInitialization(structure_t * structure, Code::statement_list_t * encoded_data);

#if false

static_data_t * BraceEncodedInitialization(type_t * type, SourcePtr source)
{
    int dim_size = 0;
    int real_size = 0;

    while (type->prop == type->constant_type)
        type = ((const_t*)type)->parent_type;

    switch (type->prop)
    {
    case type_t::dimension_type:
        dim_size = ((array_t*)type)->items_count;
        type = ((array_t*)type)->child_type;
        break;
    case type_t::pointer_type:
        type = ((pointer_t*)type)->parent_type;
        break;
    default:
        throw "TODO: BraceEncodedInitialization";
    }

    while (type->prop == type->constant_type)
        type = ((const_t*)type)->parent_type;

    static_data_t * ok = new static_data_t(lt_openblock);

    for (auto statement : *source.statements)
    {
        for (auto sequence : statement)
        {
            switch (sequence.lexem)
            {
            case lt_comma:
                continue;
            default:
                static_data_t * res = ÑheckLexemData(type, &sequence);
                (*ok->nested).push_back(res);
                ++real_size;
                continue;
            }
        }
    }
    if (dim_size != 0 && dim_size != real_size)
    {
        throw "TODO: We need generate user error here";
    }
    return ok;
}

#endif

static_data_t * namespace_t::TryEnumsAndConstants(type_t * type, Code::lexem_node_t * node)
{
    switch (type->prop)
    {
    case type->typedef_type:
    {
        typedef_t * def = (typedef_t*)type;
        return TryEnumsAndConstants(def->type, node);
    }
    case type->unsigned_type:
    case type->signed_type:
    {
        expression_node_t * expr = nullptr;
        switch (node->lexem)
        {
        case lt_word:
            expr = this->TryEnumeration(node->value);
            if (expr == nullptr)
            {
                this->CreateError(node->line_number, -77778327, "enumeration '%s' not defined", node->value);
                return nullptr;
            }
            if (!expr->is_constant)
            {
                this->CreateError(node->line_number, -77778327, "enumeration '%s' must be constant", node->value);
                return nullptr;
            }
            return new static_data_t( expr->constant->integer_value );
        default:
            throw "FSM error in TryEnumsAndConstants()";
        }
    }
    case type->enumerated_type:
    {
        enumeration_t * enu = (enumeration_t*)type;
        if (enu->enumeration.count(node->value) > 0)
        {
            return new static_data_t(enu->enumeration.at(node->value));
        }
    }
    }
    // TODO: Check constants
    return nullptr;
}

static_data_t * namespace_t::CheckLexemeData(type_t * type, Code::lexem_node_t * node)
{
    static_data_t * ok = nullptr;
    switch (node->lexem)
    {
    case lt_string:
        ok = String(type, node);
        break;
    case lt_integer:
        ok = Integer(type, node);
        break;
    case lt_word:
        ok = TryEnumsAndConstants(type, node);
        break;

    case lt_openblock:
    {
        switch (type->prop)
        {
        case type_t::property_t::compound_type:
        {
            structure_t * structure = new structure_t(*(structure_t *)type);
            ok = BraceEncodedStructureInitialization(structure, node->statements);
            break;
        }
        case type_t::property_t::dimension_type:
        {
            ok = new static_data_t(lt_openindex);
            array_t * array = (array_t*)type;
            if (array->items_count != node->statements->size())
            {
                throw "Arguemnt count not matches to definition";
            }
            for (auto statement : *node->statements)
            {
                for (auto lexem : statement)
                {
                    if (lexem.lexem == lt_comma)
                        continue;
                    static_data_t * res = CheckLexemeData(array->child_type, new Code::lexem_node_t(lexem));
                    (*ok->nested).push_back(res);
                }
            }
            break;
        }
        case type_t::property_t::typedef_type:
        {
            typedef_t * def = (typedef_t*)type;
            ok = CheckLexemeData(def->type, node);
            break;
        }

        default:
            throw "Not parsed type";
        }
        break;
    }
    default:
        throw "Unknown type";
    }
    return ok;
}

static_data_t * namespace_t::BraceEncodedStructureInitialization(structure_t * structure, Code::statement_list_t * encoded_data)
{
    static_data_t * ok = new static_data_t(lt_openblock);
    for (auto statement : *encoded_data)
    {
        std::list<variable_base_t*>::iterator var = structure->space->space_variables_list.begin();
        for (auto sequence : statement)
        {
            switch (sequence.lexem)
            {
            case lt_comma:
                var++;
                continue;
            default:
                static_data_t * res = CheckLexemeData((*var)->type, &sequence);
                (*ok->nested).push_back(res);
                continue;
            }
        }
    }
    return ok;
}

static_data_t *  namespace_t::BraceEncodedInitialization(type_t * type, SourcePtr & source)
{
    int type_is_pointer = 0;

    while (true)
    {
        switch (type->prop)
        {
        case type_t::dimension_type:
        {
            if (source.lexem == lt_openblock)
            {
                static_data_t * ok = new static_data_t(lt_openblock);
                type_is_pointer++;
                type = ((array_t*)type)->child_type;
                for (auto statement : *source.statements)
                {
                    SourcePtr  data_ptr(&statement);
                    while (data_ptr == true)
                    {
                        if (data_ptr.lexem == lt_comma)
                        {
                            ++data_ptr;
                            continue;
                        }
                        static_data_t * res = BraceEncodedInitialization(type, data_ptr);
                        (*ok->nested).push_back(res);
                    }
                }
                return ok;
            }
            throw "Fix ASAP - BraceEncodedInitialization";
        }
        case type_t::constant_type:
        {
            type = ((const_t*)type)->parent_type;
            continue;
        }
        case type_t::typedef_type:
        {
            type = ((typedef_t*)type)->type;
            continue;
        }
        case type_t::pointer_type:
        {
#if true
            Code::lexem_node_t  node;
            node.lexem = source.lexem;
            node.line_number = source.line_number;
            node.value = source.value.c_str();
            static_data_t * res = CheckLexemeData(type, &node);
            source++;
            return res;
#else
            type = ((pointer_t*)type)->parent_type;
            if (source.lexem == lt_string)
            {
                while (type->prop == type_t::constant_type)
                    type = ((const_t*)type)->parent_type;
                if (type->prop == type_t::typedef_type && type->bitsize == 8)
                {
                }
            }
            else
                throw "Fix namespace_t::BraceEncodedInitialization";
#endif
            continue;
        }
        case type_t::compound_type:
        {
            structure_t * structure = (structure_t*)type;
            if (structure->kind == lt_class)
            {
                CreateError(source.line_number , -7777716, "Brace-enclosed initialization (!allowed|!implemented) for classes");
                source.Finish();
                return nullptr;
            }

            Code::statement_list_t * encoded_data = source.statements;
            root = BraceEncodedStructureInitialization(structure, encoded_data);
            ++source;
            return root;
        }
        default:
            throw "namespace_t::BraceEncodedInitialization - FIX PROPERTY NOW!";
        }
    }
#if false
    if (type->prop == type->pointer_type)
    {
        type_is_pointer++;
        type = ((pointer_t*)type)->parent_type;
        continue;
    }
    if (type->prop == type->constant_type)
    {
        type = ((const_t*)type)->parent_type;
        continue;
    }
    break;
}

if (type->prop != type->compound_type)
{
    root = ::BraceEncodedInitialization(type, source);
    return root;
}
structure_t * structure = (structure_t*)type;
if (structure->kind == lt_class)
{
    CreateError(-7777716, "Brace-enclosed initialization (!allowed|!implemented) for classes", source.line_number);
    source.Finish();
    return nullptr;
}

Code::statement_list_t * encoded_data = source.statements;
root = ::BraceEncodedStructureInitialization(structure, encoded_data);
return root;
#endif
}
