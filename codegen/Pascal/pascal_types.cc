#include "pascal_generator.h"

char * TranslateTypeName(type_t * type)
{
    int bitsize = type->bitsize;
    switch (type->prop)
    {
    case type_t::void_type:
        return "";
    case type_t::pointer_type:
        return "Pointer";
    case type_t::float_type:
        return bitsize == 32 ? "Real" : "Double";

    case type_t::signed_type:
        return bitsize == 8 ? "ShortInt" :
            bitsize == 16 ? "SmallInt" :
            bitsize == 32 ? "Integer" : "{ signed type not parsed} ";
    case type_t::unsigned_type:
        return bitsize == 8 ? "Char" :
            bitsize == 16 ? "Word" :
            bitsize == 32 ? "Longword" : "{ unsigned type not parsed} ";

    default:
        throw "Unknown type";
    }
}

void PascalGenerator::GenerateTypeName(type_t * type) //, const char * name)
{
    bool skip_name = false;
    if (type != nullptr)
    {
        //if (name)
        //    printf("%s: ", name);

        switch (type->prop)
        {
        case type_t::pointer_type:
        {
            pointer_t * addres = (pointer_t*)type;
            if (addres->parent_type == nullptr)
            {
                throw "pointer syntax error";
            }
            if (addres->parent_type->prop == type_t::void_type)
            {
                Write("Pointer");
            }
            else if (addres->parent_type->bitsize == 8)
            {
                Write("PChar");
            }
            else
                GenerateTypeName(addres->parent_type);
            break;
        }
        case type_t::funct_ptr_type:
        {
            function_parser * func_ptr = (function_parser*)type;
            printf("%s", func_ptr->name.c_str());
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
            GenerateTypeName(constant->parent_type);
            break;
        }
        case type_t::dimension_type:
        {
            array_t * array = (array_t*)type;
            GenerateTypeName(array->child_type);
            if (skip_name)
                printf("[%d] ", array->items_count);
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
                compound_name = "Record";
                break;
            case lt_class:
                compound_name = "class";
                break;
            case lt_union:
                compound_name = "Record case of";
                break;
            default:
                throw "Non-compound type marked as compound";
            }

            //if (name)
            //	printf("%s %s %s ", compound_name, type->name.c_str(), name);
            //else
            //	printf("%s %s ", compound_name, type->name.c_str());

            printf("%s ", compound_name, type->name.c_str());
            break;
        }

        case type_t::enumerated_type:
        {
            printf("Record case of %s ", type->name.c_str());
            break;
        }

        case type_t::typedef_type:
        {
            printf(" %s", type->name.c_str());
            break;
        }

        default:
            printf("%s ", TranslateTypeName(type));
        }
    }
}

void PascalGenerator::GenerateTypes()
{
    if (this->space_types_list.size() > 0)
    {
        std::list<type_t*>::iterator type;
        for (
            type = this->space_types_list.begin();
            type != this->space_types_list.end();
            ++type)
        {
            GenerateType(*type, false);
        }
    }
}
