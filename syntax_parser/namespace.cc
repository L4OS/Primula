#include "namespace.h"
#include "lexem_tree_root.h"
#if ! MODERN_COMPILER
#  include <stdlib.h>
#endif

type_t  namespace_t::builtin_types[18] =
{
    /*  0 */		type_t("auto", type_t::auto_type, 0),
    /*  1 */		type_t("int", type_t::signed_type, 32),
    /*  2 */		type_t("unsigned int", type_t::unsigned_type, 32),
    /*  3 */		type_t("char", type_t::signed_type, 8),
    /*  4 */		type_t("unsigned char", type_t::unsigned_type, 8),
    /*  5 */		type_t("short", type_t::signed_type, 16),
    /*  6 */		type_t("unsigned short", type_t::unsigned_type, 16),
    /*  7 */		type_t("long", type_t::signed_type, 32),
    /*  8 */		type_t("unsigned long", type_t::unsigned_type, 32),
    /*  9 */		type_t("long long", type_t::signed_type, 64),
    /* 10 */		type_t("unsigned long long", type_t::unsigned_type, 64),
    /* 11 */		type_t("void", type_t::void_type, 0),
    /* 12 */		type_t("bool", type_t::unsigned_type, 32),
    /* 13 */		type_t("float", type_t::float_type, 32),
    /* 14 */		type_t("double", type_t::float_type, 64),
    /* 15 */		type_t("long double", type_t::float_type, 64),
    /* 16 */		type_t("wchar_t", type_t::unsigned_type, 16),
    /* 17 */		type_t("__int64", type_t::signed_type, 64)
};

Errors                                   namespace_t::errors;

void namespace_t::RegisterBuiltinTypes()
{
    for (int i = 0; i < sizeof(builtin_types) / sizeof(builtin_types[0]); ++i)
    {
        this->CreateType( &builtin_types[i], builtin_types[i].name);
    }
}

type_t	*	namespace_t::GetBuiltinType(lexem_type_t type)
{
    switch (type)
    {
    case lt_auto:			return &builtin_types[0];
    case lt_type_int:		return &builtin_types[1];
    case lt_type_uint:		return &builtin_types[2];
    case lt_type_char:		return &builtin_types[3];
    case lt_type_uchar:		return &builtin_types[4];
    case lt_type_short:		return &builtin_types[5];
    case lt_type_ushort:	return &builtin_types[6];
    case lt_type_long:		return &builtin_types[7];
    case lt_type_ulong:		return &builtin_types[8];
    case lt_type_llong:		return &builtin_types[9];
    case lt_type_ullong:	return &builtin_types[10];
    case lt_void:			return &builtin_types[11];
    case lt_type_bool:		return &builtin_types[12];
    case lt_type_float:		return &builtin_types[13];
    case lt_type_double:	return &builtin_types[14];
    case lt_type_ldouble:	return &builtin_types[15];
    case lt_wchar_t:		return &builtin_types[16];
    case lt_type_int64:		return &builtin_types[17];
    default:
        break;
    }
    return 0;
}

void namespace_t::CreateError(int linenum, int error, std::string description, ...)
{

    char buffer[256];
    va_list args;
    va_start(args, description);
    buffer[255] = '\0';
    vsnprintf(buffer, 254, description.c_str(), args);
    va_end(args);

    fprintf(stderr, "%d: Error %d: %s\n", linenum, error, buffer);
    if (this != nullptr)
        errors.Add(error, buffer, linenum);
    else
        throw "Trying to log error into not declared namespace";
}

variable_segment_t namespace_t::FindSegmentType(linkage_t * linkage) 
{
    switch (linkage->storage_class)
    {
    case linkage_t::sc_default:
        switch (this->type)
        {
        case global_space:
            return vs_global;
        default:
            throw "namespace_t::FindSegmentType - space type not parsed";
        }
        break;
    case linkage_t::sc_extern:
        fprintf(stderr, "Interpreter: extern variable not supported. Redefine as global varibale\n");
        return vs_global;
        break;
    default:
        throw "namespace_t::FindSegmentType - storage class not parsed";
    }
}


type_t * namespace_t::TryLexenForType(SourcePtr &source)
{
    type_t * type = nullptr;
    if (source.lexem == lt_const)
    {
        source++;
        if (source == false)
            return nullptr;
        type = TryLexenForType(source);
        if (type == nullptr)
            return nullptr;
        return new const_t(type);
    }
    type = GetBuiltinType(source.lexem);
    if (type == nullptr)
        switch (source.lexem)
        {
        case lt_word:
        case lt_struct:
        case lt_class:
        case lt_union:
        case lt_enum:
            type = this->FindType(source.value);
        }
    return type;
}

//extern namespace_t			global_space;
namespace_t				*	current_space = nullptr; // &global_space;

void InitialNamespace(namespace_t * space)
{
    current_space = space;
}

// Interpeter allocates raw memory for local variables

/*
void namespace_t::AllocateLocalVariablePool(std::list<common_value_t> * calculated_arguments)
{
    int argument_frame_size = calculated_arguments ? calculated_arguments->size() << 2 : 0; // 32bits
    int stack_size = non_static_variable_pool_size + argument_frame_size;
    if (stack_size == 0)
    {
        local_argument_space = nullptr;
        local_variable_space = nullptr;
        return;
    }

    local_argument_space = new char[stack_size];

    if (argument_frame_size != 0)
    {
        fprintf(stderr, "Allocated %d bytes for arguments of '%s' : 0x%p\n",
            argument_frame_size,
            this->name.c_str(),
            local_argument_space);

        int * argument_frame = (int*)local_argument_space;
        for (auto var : *calculated_arguments)
        {
                fprintf(stderr, "Set argument on 0x%p to %d\n", argument_frame, var.i32);
                *argument_frame++ = var.i32;
        }
    }

    if (non_static_variable_pool_size != 0)
    {
//        local_variable_space = new char[non_static_variable_pool_size];

        local_variable_space = &local_argument_space[argument_frame_size];

        fprintf(stderr, "Allocated %d bytes for local variables of '%s' : 0x%p\n",
            non_static_variable_pool_size,
            this->name.c_str(),
            local_variable_space);

        for (auto var : this->space_variables_list)
        {
            if (var->value.char_pointer != nullptr)
            {
                fprintf(stderr, "Found constant data. TODO: check static variable\n");
                char * offset = local_variable_space + var->value.offset;

                // Byte by byte copy static data to local variable
                for (int i = 0; i < non_static_variable_pool_size; ++i)
                {
                    offset[i] = var->value.char_pointer[i];
                }
            }
        }
    }
    else
        local_variable_space = nullptr;

}

void namespace_t::ReleaseLocalVariablePool()
{
    if (local_argument_space != nullptr)
    {
        fprintf(stderr, "Deleted %d bytes on 0x%p of arguments and local variables of '%s'\n",
            non_static_variable_pool_size,
            local_argument_space,
            this->name.c_str());
        delete local_argument_space;
        local_argument_space = nullptr;
        local_variable_space = nullptr;
    }
}
*/


int namespace_t::CheckNamespace(SourcePtr &source)
{
    std::string		spacename;
    switch (source.lexem)
    {
    case lt_word:
        spacename += source.value;
        source++;
        if (source.lexem != lt_openblock)
            return -1;
    case lt_openblock:
    {
        current_space = current_space->CreateSpace(name_space, spacename);
        // call to parser namesoace's statements
#if MODERN_COMPILER
        for (auto statement : *source.statements)
        {
            current_space->ParseStatement(statement);
        }
#else
        Code::statement_list_t::iterator statement;
        for (statement = source.statements->begin(); statement != source.statements->end(); ++statement)
        {
            current_space->ParseStatement(*statement);
        }
#endif
        this->embedded_space_map.insert(std::make_pair(spacename, current_space));
        current_space = current_space->Parent();
        source++;
        break;
    }
    default:
        return -1;
    }

    return 0;
}

namespace_t * namespace_t::FindNamespaceInSpace(std::string name)
{
    std::map<std::string, namespace_t*>::iterator pair;
    pair = this->embedded_space_map.find(name);
    if (pair != embedded_space_map.end())
        return pair->second;
    
    parent_spaces_t::iterator  inherited = this->inherited_from.begin();
    while (inherited != this->inherited_from.end())
    {
        namespace_t *   space = (*inherited)->space->FindNamespaceInSpace(name);
        if (space != nullptr)
            return space;
        ++inherited;
    }

    return  nullptr;
}

namespace_t * namespace_t::FindNamespace(std::string name)
{
    namespace_t * space = FindNamespaceInSpace(name);
    if (space)
        return space;

    return (parent == nullptr) ? nullptr : parent->FindNamespace(name);
}

variable_base_t * namespace_t::FindVariable(std::string name)
{
    variable_base_t * var = nullptr;

    do
    {
        if (this->type == function_space || this->type == exception_handler_space)
        {
            var = owner_function->FindArgument(name);
            if (var != nullptr)
                break;
        }
        var = FindVariableInspace(name);
        if (var != nullptr)
            break;
        var = TryEnumeration(name, true);
        if (var != nullptr)
            break;
        if (this->parent != nullptr)
            var = parent->FindVariable(name);
    } while (false);

    if (var != nullptr)
        ++var->access_count;
    return var;
}


variable_base_t     *   namespace_t::FindVariableInspace(std::string name)
{
    std::map<std::string, variable_base_t*>::iterator pair;
    pair = this->space_variables_map.find(name);
    if (pair != space_variables_map.end())
    {
        ++pair->second->access_count;
        return pair->second;
    }

    std::list<namespace_t *>::iterator used_space = this->using_space_list.begin();
    while (used_space != this->using_space_list.end())
    {
        variable_base_t * var = (*used_space)->FindVariableInspace(name);
        if (var != nullptr)
            return var;
        ++used_space;
    }

    parent_spaces_t::iterator  inherited = this->inherited_from.begin();
    while (inherited != this->inherited_from.end())
    {
        variable_base_t * var = (*inherited)->space->FindVariableInspace(name);
        if (var != nullptr)
            return var;
        ++inherited;
    }
    return nullptr;
}


function_parser		* namespace_t::FindFunctionInSpace(std::string name)
{
#if MODERN_COMPILER
    auto pair = function_map.find(name);
    if (pair != function_map.end())
    {
        ++pair->second->access_count;
        return pair->second;
    }
    function_parser * f = nullptr;;
    for (auto inherited_from : this->inherited_from)
    {
        f = inherited_from->space->FindFunctionInSpace(name);
        if (f != nullptr)
            break;
    }
#else
    std::map<std::string, class function_parser*>::iterator pair;
    pair = function_map.find(name);
    if (pair != function_map.end())
    {
        return pair->second;
    }
    function_parser * f = nullptr;;

    for
        (
            parent_spaces_t::iterator   inherited_from = this->inherited_from.begin();
            inherited_from != this->inherited_from.end();
            ++inherited_from
            )
    {
        f = (*inherited_from)->space->FindFunctionInSpace(name);
        if (f != nullptr)
            break;
    }
#endif
    return f;
}

function_parser		* namespace_t::FindFunction(std::string name)
{
    function_parser * f = FindFunctionInSpace(name);
    return f != nullptr ? f : this->parent == nullptr ? nullptr : parent->FindFunction(name);
}

function_parser		* namespace_t::FindTemplateFunction(std::string name)
{
#if MODERN_COMPILER
    auto pair = template_function_map.find(name);
#else
    std::map<std::string, function_parser*>::iterator   pair;
    pair = template_function_map.find(name);
#endif
    if (pair != template_function_map.end())
        return pair->second;
    if (this->parent != nullptr)
        return parent->FindTemplateFunction(name);
    return nullptr;
}

variable_base_t::variable_base_t(
    namespace_t		*	space,
    type_t			*	type,
    std::string			name,
    variable_segment_t storage)
    : statement_t(statement_t::_variable)
{
    access_count = 0;
    this->space = space;
    this->type = type;
    this->name = name;
    this->storage = storage;
    declaration = nullptr;
#if COMPILER
    static_data = nullptr;
#else
#endif
    hide = false; // This is useful for reconstruction to source code of "for" loop
//        printf("Create variable: %s %p\n", name.c_str(), this);
}

variable_base_t * namespace_t::CreateVariable(type_t * type, std::string name, variable_segment_t  segment)
{
    variable_base_t * variable = new variable_base_t(this, type, name, segment);
    space_variables_map.insert(std::make_pair(name, variable));
    space_variables_list.push_back(variable);
    this->space_code.push_back(variable);
#if ! COMPILER
    {
        variable->value.offset = non_static_variable_pool_size;
        non_static_variable_pool_size += (variable->type->bitsize >> 3);
    }
#endif
    return variable;
}

void namespace_t::RegisterFunction(std::string name, function_parser * function, bool back)
{
    function_map.insert(std::make_pair(name, function));
    if (back)
        function_list.push_back(function);
    else
        function_list.push_front(function);

}

type_t * namespace_t::CreateType(type_t * type, std::string name)
{
    space_types_map.insert(std::make_pair(name, type));
    space_types_list.push_back(type);
    return type;
}

type_t * namespace_t::FindType(std::string name)
{
#if MODERN_COMPILER
    auto pair = space_types_map.find(name);
#else
    std::map<std::string, type_t*>::iterator pair;
    pair = space_types_map.find(name);
#endif
    if (pair != space_types_map.end())
        return pair->second;
    if (this->parent != nullptr)
        return parent->FindType(name);
    return nullptr;
}

int namespace_t::Parse(Code::statement_list_t source)
{
    try
    {
#if MODERN_COMPILER
        for (Code::lexem_list_t statement : source)
        {
            SourcePtr source(&statement);
            ParseStatement(source);
        }
#else
        for
            (
                Code::statement_list_t::iterator statement = source.begin();
                statement != source.end();
                ++statement
                )
        {
            SourcePtr source(&(*statement));
            ParseStatement(source);
        }
#endif
    }
    catch (char * text)
    {
        ::fprintf(stderr, "namespace exception: %s\n", text);
        return -777780;
    }
    return 0;
}


void namespace_t::CheckStorageClass(SourcePtr & source, linkage_t * linkage)
{
    if (source != false)
        if (linkage->storage_class != linkage_t::sc_default)
        {
            CreateError(source.line_number, -77777, "storage duplication");
        }
        else
            switch (source.lexem)
            {
            case lt_extern:
                linkage->storage_class = linkage_t::sc_extern;
                source++;
                if (source != false && source.lexem == lt_string)
                {
                    linkage->linkage_def = source.value;
                    source++;
                }
                return;
            case lt_static:
                linkage->storage_class = linkage_t::sc_static;
                break;
            case lt_register:
                linkage->storage_class = linkage_t::sc_register;
                break;
            case lt_mutable:
                linkage->storage_class = linkage_t::sc_mutable;
                break;
            case lt_virtual:
                linkage->storage_class = linkage_t::sc_virtual;
                break;
            case lt_inline:
                linkage->inlined = true;
                break;
            default:
                return;
            }
    source++;
    return;
}

int namespace_t::ParseStatement(Code::lexem_list_t statement)
{
    SourcePtr source(&statement);
    return ParseStatement(source);
}

void namespace_t::SelectStatement(type_t * type, linkage_t * linkage, std::string name, SourcePtr & source)
{
    variable_base_t					*	variable = nullptr;
    function_overload_t				*	function = nullptr;

    switch (source.lexem)
    {
    case lt_namescope:
    {
        source++;
        // Loocking for compound type
        type_t	* compound = FindType(name);
        if (compound != nullptr)
        {
            if (compound->prop == type_t::compound_type)
            {
                if (source.lexem != lt_word)
                    throw "Select statement FSM error: lexeme is not word";
                structure_t	* structure = (structure_t*)compound;
                name = source.value;
                source++;
                structure->space->SelectStatement(type, linkage, source.value, source);
            }
            else
            {
                CreateError(source.line_number, -7777726, "type '%s' have no space", name.c_str());
                source.Finish();
                return;
            }
        }
        else
            throw "Select statement FSM error: maybe name is namespace?";
        return;
    }
    case lt_comma:
    case lt_semicolon:
    case lt_set:
        variable = FindVariable(name);
        if (variable != nullptr && variable->space == this)
        {
            CreateError(source.line_number, -77777, "Variable '%s' already defined", name.c_str());
            source.Finish();
            return;
        }
        variable = CreateVariable(type, name, FindSegmentType(linkage));
        if (source.lexem == lt_semicolon)
            return;
        if (source.lexem == lt_comma)
            return;
        if (source.lexem == lt_set)
        {
            source++;
            if (source.lexem == lt_openblock)
            {
#if COMPILER
                variable->static_data = BraceEncodedInitialization(type, source);
#else
                fprintf(stderr, "%d bytes constant area allocation for variable '%s'\n", type->bitsize >> 3, variable->name.c_str());
                variable->value.char_pointer = new char[type->bitsize>>3];
                char  * eop =  (char *) PackDefinitionToMemory(type, source, (unsigned int *) variable->value.char_pointer);
                if (variable->value.char_pointer + (type->bitsize >> 3) != eop)
                {
                    fprintf(stderr, "Data not aligned to structure boundary\n");
                }
#endif
                return;
            }
            expression_t * assign = ParseExpression(source);
            if (assign != nullptr)
            {
                bool is_zero = assign->is_constant ?
                    assign->type->prop == type_t::signed_type ?
                    (assign->root->value.integer_value == 0) : false : false;

                //if (source.line_number == 701)
                //    printf("debug\n");

                if (CompareTypes(variable->type, assign->type, false, is_zero) == no_cast)
                {
                    CreateError(source.line_number, -777776, "Cannot cast %s to %s",
                        assign->type->name.c_str(), variable->type->name.c_str());
                    return;
                }
                variable->declaration = assign;
            }

            if (source.lexem == lt_comma)
                return;

            if (source.lexem == lt_semicolon)
                return;
        }
        // Assign variable parse
        source++;
        if (source == false)
        {
            CreateError(source.line_number, -777776, "Broken assignment code");
            return;
        }
        if (source.lexem == lt_openblock)
        {
            switch (variable->type->prop)
            {
            case type_t::compound_type:
            case type_t::pointer_type:
                throw "todo: parse struct and array initialization";
            default:
                CreateError(source.line_number, -777776, "Assignment block format error");
                source.Finish();
                return;
            }
        }
        else
        {
            ParseExpression(source);
            //			__debugbreak();
            source++;
            return;
        }
        return;

    case lt_openbraket:
    {
        Code::lexem_list_t				*	sequence = source.sequence;
        if (sequence->size() != 0)
        {
            SourcePtr	args(sequence);
            if (args.lexem == lt_const)
                args++;
            type_t * is_type = this->TryLexenForType(args);
            if (is_type == nullptr)
            {
                if (type->name == name)
                {
                    fprintf(stderr, "This is an instance of %s %s(%s)\n", type->name.c_str(), name.c_str(), args.value.c_str());
                    // TODO: Check that constructor is matched to arguemts

                    variable_base_t * instance = CreateVariable(type, name, FindSegmentType(linkage));
                    if (type->prop != type_t::compound_type)
                    {
                        throw "Default constructors for non-compound types is not implemented yet";
                    }
                    structure_t * structure = (structure_t*)type;
                    function_parser * constructor = structure->space->FindFunction(type->name);
                    if (constructor != nullptr)
                    {
                        call_t	* call = constructor->TryCallFunction(this, args);
                    }
                    else
                    {
                        throw "Cannot find constructor on conpound variable declaration";
                    }
                    source++;
                    return;
                }
                switch (args.lexem)
                {
                case lt_integer:
                case lt_string:
                case lt_class:
                case lt_enum:
                case lt_word:
                    variable = CreateVariable(type, name, FindSegmentType(linkage));
                    if (variable->type->prop == type_t::compound_type)
                    {
                        structure_t *  st = (structure_t*)variable->type;
                        function_parser * constructor = st->space->FindFunctionInSpace(st->name);
                        if (constructor == nullptr)
                        {
                            CreateError(source.line_number, -7711114, "Constructor for %s not defined", name);
                            source.Finish();
                            return;
                        }
                        SourcePtr args(source.sequence);
                        variable->declaration = constructor->TryCallFunction(this, args);
                        source++;
                        return;
                    }
                    else
                    {
                        expression_t  *  code = ParseExpression(source);
                        if (CompareTypes(variable->type, code->type, true, 0) == no_cast)
                        {
                            CreateError(source.line_number, -7711114, "Unable translate type");
                            source.Finish();
                            return;
                        }
                        // TODO: enable only if constant or namespace within function 
                        variable->declaration = code;
                        return;
                    }
                    break;
                default:
                    CreateError(source.line_number, -7777774, "non-terminated function definition");
                    source.Finish();
                    return;
                }
            }
        }
        source++;
        if (source == false)
        {
            CreateError(source.line_number, -7777774, "non-terminated function definition");
            return;
        }

        function = CreateFunction(type, name, sequence, linkage);
        if (source.lexem == lt_const)
        {
            function->read_only_members = true;
            source++;
            if (source == false)
            {
                CreateError(source.line_number, -7777774, "non-terminated function definition");
                return;
            }
        }
        if (source.lexem == lt_openblock)
        {
            if (this->type == function_space)
            {
                CreateError(source.line_number, -7777774, "function inside function not allowed");
                source.Finish();
                return;
            }
            if (function->space != nullptr)
            {
                CreateError(source.line_number, -77777, "Function '%s' already defined in current namespace", name.c_str());
                source.Finish();
                return;
            }
            function->Parse(source.line_number, this, source.statements);
            if (this->no_parse_methods_body)
                function->linkage.inlined = true;
            //else
            //    printf("/* Check here! */\n");
            source++;
            return;
        }
        if (source.lexem == lt_semicolon)
        {
            // fprintf(stderr, "Line %d: debug: semicolon on select_state in namespace_t::ParseStatement", source.line_number);
            return;
        }
        if (source.lexem == lt_set)
        {
            source++;
            if (source != false && source.lexem == lt_integer)
            {
                if (source.value.size() == 1 && source.value[0] == '0')
                {
                    if (function->linkage.storage_class == linkage_t::sc_virtual)
                    {
                        function->linkage.storage_class = linkage_t::sc_abstract;
                        source++;
                        break;
                    }
                    else
                    {
                        CreateError(source.line_number, -7777414, "Non virtual method '%s' marked as abstract", function->function->name.c_str());
                        source.Finish();
                        break;
                    }
                }
            }
        }
    }
    CreateError(source.line_number, -7777401, "Unaprsed lexem on function definition|declaration");
    source.Finish();
    break;

    case lt_openindex:
#if true // Is this check unneccessary?
        variable = FindVariable(name);
        if (variable != nullptr && variable->space == this)
        {
            CreateError(source.line_number, -7777400, "Variable '%s' already defined", name.c_str());
            source.Finish();
            return;
        }
#endif
        do {
            if (source.sequence->size() == 0)
            {	// empty index array may be updated later
                type = new array_t(type);
            }
            else
            {	//got index expression
#if MODERN_COMPILER
                expression_t * expr = ParseExpression(SourcePtr(source.sequence));
#else
                SourcePtr ptr(source.sequence);
                expression_t * expr = ParseExpression(ptr);
#endif
                if (expr == nullptr)
                {
                    CreateError(source.line_number, -777743, "broken expression in array definition");
                    source.Finish();
                    return;
                }
                if (!expr->is_constant)
                {
                    CreateError(source.line_number, -777745, "dimension size is not constant");
                    source.Finish();
                    return;
                }
                // Check size of dimension
                int dimsize = expr->root->value.integer_value;
                type = new array_t(type, dimsize);
            }
            source++;
        } while (source == true && source.lexem == lt_openindex);
        variable = CreateVariable(type, name, FindSegmentType(linkage));
        return;

    case lt_colon:
        if (++source == false || source.lexem != lt_integer)
        {
            CreateError(source.line_number, -7777914, "Expected field size in bits");
            source.Finish();
            return;
        }
        else
        {
#if MODERN_COMPILER
            int field_size = std::stoi(source.value);
#else
            int field_size = atoi(source.value.c_str());
#endif
            type->bitsize = field_size;
            if (++source == false || source.lexem != lt_semicolon)
            {
                CreateError(source.line_number, -7777914, "Bad termination lexem (%d) of class|struct field definition", source.lexem);
                source.Finish();
                return;
            }
            variable = CreateVariable(type, name, FindSegmentType(linkage));
        }
        return;

    default:
        CreateError(source.line_number, -7777774, "Unparsed lexem (%d) on selection state in statement parser", source.lexem);
        source.Finish();
    }
}

void namespace_t::ParseUsing(SourcePtr &source)
{
    ++source;
    if (source == false)
    {
        CreateError(source.line_number, -7770101, "unterminated statement 'using'");
        return;
    }
    if (source.lexem != lt_namespace)
    {
        CreateError(source.line_number, -7770101, "FSM error on parsing 'using' statememt");
        return;
    }
    ++source;
    if (source.lexem != lt_word)
    {
        CreateError(source.line_number, -7770102, "expected name of using space");
        source.Finish();
        return;
    }
    namespace_t * space = FindNamespace(source.value);
    if (space == nullptr)
    {
        CreateError(source.line_number, -7770102, "namespace '%s' not found", source.value.c_str());
        return;
    }

    // https://en.cppreference.com/w/cpp/language/namespace#Using-directives
    if(space->parent == this)
        using_space_list.push_back(space);
    else
    {
        namespace_t * global = this;
        while (global->parent)
            global = global->parent;
        global->using_space_list.push_back(space);
    }
}

int namespace_t::ParseStatement(SourcePtr &source)
{
    enum {
        startup_state,
        parsing_state,
        type_state,
        select_state,
        check_function_definition_state,
        check_compound_state,
        check_enumeration_state,
        wait_delimiter_state,
        visibility_colon,
        finish_state
    } state = startup_state;

    linkage_t           linkage;
    std::string         name;
    parent_spaces_t     inherited_from;
    bool                destructor = false;

    type_t                          *	type = nullptr;
    type_t                          *   comma_type = nullptr;
    variable_base_t                 *	variable = nullptr;
    function_parser                 *	function = nullptr;
    statement_t						*	code = nullptr;
    namespace_t                     *   space = nullptr;

    static int debug_line = 0;
    while (source != false)
    {
        if (debug_line == source.line_number)
        {
            CreateError(source.line_number, 0, "Statement Breakpoint");
        }
        switch (state)
        {
        case startup_state:
            //type = nullptr;
            //variable = nullptr;
            //function = nullptr;
            //code = nullptr;
            CheckStorageClass(source, &linkage);
            state = parsing_state;
            continue;

        case finish_state:
            CreateError(source.line_number, -777778, "Lexeme ('%d') after statement terminator", source.lexem);
            source.Finish();
            continue;

        case type_state:
            switch (source.lexem)
            {
            case lt_namescope:
                if (type->prop == type_t::compound_type)
                {
                    structure_t * structure = (structure_t *)type;
                    structure->space->ParseStatement(++source);
                    continue;
                }
                CreateError(source.line_number, -777778, "Type '%s' is not structrued. But maybe not error", type->name.c_str());
                source.Finish();
                continue;

            case lt_semicolon:
                if (type->prop != type_t::compound_type)
                {
                    CreateError(source.line_number, -7777774, "statement not define anything");
                    source.Finish();
                    continue;
                }
                source++;
                state = startup_state;
                continue;
            case lt_mul:
                type = new pointer_t(type);
                source++;
                continue;
            case lt_const:
                type = new const_t(type);
                source++;
                continue;
            case lt_word:
                name = source.value;
                source++;
                state = select_state;
                continue;
            case lt_and:
                type = new address_t(type);
                source++;
                continue;
            case lt_operator:
                CheckOverloadOperator(&linkage, type, ++source);
                state = finish_state;
                break;

            case lt_openbraket:
            {
                if (this->type != structure_space)
                {
                    CreateError(source.line_number, -7777774, "function inside function not allowed");
                    source.Finish();
                    continue;
                }
                if (type->prop == type_t::compound_type)
                {
                    structure_t * structure = (structure_t *)type;
                    // -------------   This is constructor or destructor --------------------
                    if (structure->space == this)
                    {
                        if (!destructor)
                        {
                            function_overload_t * overload = CheckCostructorDestructor(&linkage, type, type->name, source);
                            overload->function->method_type = function_parser::constructor;
                        }
                        else
                        {
                            function_overload_t * overload = CheckCostructorDestructor(&linkage, type, "~" + type->name, source);
                            overload->function->method_type = function_parser::destructor;
                        }
                        if (source != false)
                        {
                            CreateError(source.line_number, -7777074, "wrong declaration of constructor");
                            source.Finish();
                        }
                        continue;
                    }
                }
                // This is variable definition (also may be pointer to a function)
                type_t * vartype = TypeDefOpenBraket(source, name, GetBuiltinType(lt_auto));
                if (source == false)
                {
                    throw "expect semicolon or open parenthesis";
                }
                linkage_t linkage;
                variable_base_t * v = nullptr;
                function_overload_t * f = nullptr;
                switch (source.lexem)
                {
                case lt_semicolon:
                    v = new variable_base_t(this, vartype, name, FindSegmentType(&linkage));
                    throw "FSM error: Lost variable";
                    break;
                case lt_openbraket:
                    f = CreateFunction(type, name, source.sequence, &linkage);

                    if (vartype != nullptr)
                    {
                        type_t * t = vartype;
                        if (vartype->prop == type_t::auto_type)
                        {
                            throw "Fix me just now";
                        }
                        while (t->prop == type_t::pointer_type)
                        {
                            if (((pointer_t*)t)->parent_type->prop == type_t::auto_type)
                                break;
                            else
                                t = ((pointer_t*)t)->parent_type;
                        }
                        if (t->prop == type_t::pointer_type)
                        {
                            pointer_t	*	parent = (pointer_t*)t;
                            //fprintf(stderr, "Catch typetype\n");
                            parent->parent_type = f->function->type;
                        }
                    }
                    v = new variable_base_t(this, vartype, name, FindSegmentType(&linkage));
                    space_variables_map.insert(std::make_pair(v->name, v));
                    space_variables_list.push_back(v);
                    source++;
                    if (source != false)
                    {
                        if (source.lexem == lt_const)
                        {
                            f->read_only_members = true;
                            source++;
                        }
                    }
                    state = wait_delimiter_state;
                    continue;

                default:
                    CreateError(source.line_number, -7777074, "wrong declaration of constructor");
                    source.Finish();
                    continue;
                }
                state = startup_state;
                continue;

            }

            case lt_less:
            {
                variable_base_t * instance = CreateObjectFromTemplate(type, &linkage, ++source);
                space_variables_map.insert(std::make_pair(instance->name, instance));
                space_variables_list.push_back(instance);
                // Don't forget add constructor call
                this->space_code.push_back(instance);
            }
            continue;

            case lt_colon:
            {
                if (source.previous_lexem != lt_struct && source.previous_lexem != lt_class)
                {
                    CreateError(source.line_number, -7771393, "inheritance allowed only for classes and structures");
                    source.Finish();
                    continue;
                }
                lexem_type_t	keep_type = source.previous_lexem;
                source++;
                if (source == false || source.lexem != lt_word)
                {
                    CreateError(source.line_number, -7771393, "syntax error on type inheritance");
                    source.Finish();
                    continue;
                }
                do
                {
                    structure_t * parent_type = (structure_t*)this->FindType(source.value);
                    if (parent_type == nullptr)
                    {
                        CreateError(source.line_number, -7771393, "parent type not found");
                        source.Finish();
                        continue;
                    }
                    if (parent_type->prop != parent_type->compound_type)
                    {
                        CreateError(source.line_number, -7771393, "parent type '%s' is not compound", parent_type->name.c_str());
                        source.Finish();
                        continue;
                    }
                    fprintf(stderr, "TODO: Assing parent type '%s' to %s\n", parent_type->name.c_str(), name.c_str());
                    inherited_from.push_back(parent_type);
                    source++;
                    if (source != true)
                    {
                        CreateError(source.line_number, -7771393, "non-termiated inheritance");
                        continue;
                    }
                    if (source.lexem == lt_openblock)
                    {
                        source.previous_lexem = keep_type;
                        state = check_compound_state;
                        break;
                    }
                    if (source.lexem != lt_comma)
                    {
                        CreateError(source.line_number, -7771393, "wrong delimiter (%d)", source.lexem);
                        source.Finish();
                        continue;
                    }
                    source++;
                } while (source == true);

            }
            continue;

            default:
                throw "Wrong lexeme at type_state in namespace_t::ParseStatement";
            }
            break;

        case select_state:
            SelectStatement(type, &linkage, name, source);
            if (source == false)
                continue;
            switch (source.lexem)
            {
            case lt_semicolon:
                state = startup_state;
                source++;
                continue;
            case lt_comma:
                type = comma_type;
                state = type_state;
                source++;
                continue;
            case lt_set:
                source++;
                if (source == false)
                {
                    CreateError(source.line_number, -7775312, "no data found for assignment|declaration");
                    continue;
                }
                variable = FindVariable(name);
                if (variable == nullptr)
                {
                    CreateError(source.line_number, -7777777, "internal syntax parser error");
                    source.Finish();
                    continue;
                }
                if (source.lexem != lt_openblock)
                {
                    throw "Internal syntax parser error in namespace_t::ParseStatement";
                }
#if COMPILER
                variable->static_data = BraceEncodedInitialization(variable->type, source);
                if (variable->static_data->type != lt_openblock)
                    throw "Second Internal syntax parser error in namespace_t::ParseStatement";
#else
                {
                    variable->value.char_pointer = new char[variable->type->bitsize >> 3];
                    char * eod = (char *)PackDefinitionToMemory(variable->type, source, (unsigned int *)variable->value.char_pointer);
                    if (eod - variable->value.char_pointer != variable->type->bitsize >> 3)
                    {
                        fprintf(stderr, "Data synch lost at variavle initialization\n");
                    }
               }
#endif
                if (variable->type->prop == type_t::dimension_type)
                {
                    array_t * array = (array_t*)variable->type;
#if COMPILER
                    int real_count = variable->static_data->nested->size();
#else
                    int real_count = array->items_count;;
                    fprintf(stderr, "Check real elemnts count of array\n");
#endif
                    int type_count = array->items_count;
                    if (type_count == 0 && array->bitsize == 32)
                    {
                        array->items_count = real_count;
                        array->bitsize = real_count * array->child_type->bitsize;
                    }
                    else if (type_count < real_count)
                    {
                        CreateError(source.line_number, -7779007, "too many initializer [%d] for array[%d]", real_count, type_count);
                        source.Finish();
                        continue;
                    }
                    source++;
                    continue;
                }
                throw "Third Internal syntax parser error in namespace_t::ParseStatement";

            default:
                throw "Fix me just NOW!!!!!";
            }
            break; // end of select state

        case parsing_state:
            type = GetBuiltinType(source.lexem);
            if (type != nullptr)
            {
                comma_type = type;
                state = type_state;
                source++;
                continue;
            }

            switch (source.lexem)
            {
            case lt_namespace:
                CheckNamespace(++source);
                source.Finish();
                continue;

            case lt_typedef:
                type = ParseTypeDefinition(source);
                comma_type = type;
                state = type_state; // Are you sure?
                continue;

            case lt_struct:
                name = source.value;
                source++;
                state = check_compound_state;
                continue;
            case lt_class:
                name = source.value;
                source++;
                state = check_compound_state;
                continue;
            case lt_union:
                name = source.value;
                source++;
                state = check_compound_state;
                continue;

            case lt_enum:
                name = source.value;
                state = check_enumeration_state;
                source++;
                continue;

            case lt_template:
                ParseTemplate(++source);
                state = startup_state;
                break;

            case lt_operator:
                CheckOverloadOperator(&linkage, type, ++source);
                state = finish_state;
                break;

            case lt_const:
                type = TryLexenForType(source);
                if (type == nullptr)
                {
                    CreateError(source.line_number, -777745, "destructor on non-compound type '%s' not implemented", type->name.c_str());
                    source.Finish();
                    continue;
                }
                comma_type = type;
                state = type_state;
                source++;
                continue;

            case lt_private:
#if MODERN_COMPILER
                this->current_visibility = visibility_t::vs_private;
#else
                this->current_visibility = vs_private;
#endif
                source++;
                state = visibility_colon;
                break;
            case lt_protected:
#if MODERN_COMPILER
                this->current_visibility = visibility_t::vs_protected;
#else
                this->current_visibility = vs_protected;
#endif
                source++;
                state = visibility_colon;
                break;
            case lt_public:
#if MODERN_COMPILER
                this->current_visibility = visibility_t::vs_public;
#else
                this->current_visibility = vs_public;
#endif
                source++;
                state = visibility_colon;
                break;

            case lt_tilde:
                if (this->type != structure_space)
                {
                    CreateError(source.line_number, -777745, "destructor on non-compound type not implemented");
                    source.Finish();
                    continue;
                }
                destructor = true;
                source++;
                continue;

            case lt_openbraket:
            case lt_mul:
            case lt_this:
            case lt_inc:
            case lt_dec:
            case lt_namescope:
            {
                expression_t  *  code = ParseExpression(source);
                //
                // add expressiob to list
                //
                this->space_code.push_back(code);
                if (source != false && source.lexem != lt_semicolon)
                {
                    fprintf(stderr, "Skip something\n");
                    source.Finish();
                }
                source++;
                continue;
            }

            case lt_semicolon:
                fprintf(stderr, "Igmore semicolon in namespace_t::CheckOperators\n");
                source++;
                break;

            case lt_openblock:
            {
                current_space = this->CreateSpace(codeblock_space, "just_block");
#if MODERN_COMPILER
                // call to parser namesoace's statements
                for (auto statement : *source.statements)
                {
                    current_space->ParseStatement(statement);
                }
#else
                Code::statement_list_t::iterator    statement;
                for (
                    statement = source.statements->begin();
                    statement != source.statements->end();
                    ++statement)
                {
                    current_space->ParseStatement(*statement);
                }
#endif
                codeblock_t * codeblock = new codeblock_t;
                codeblock->block_space = current_space;
                current_space = current_space->Parent();
                this->space_code.push_back(codeblock);
            }
            source++;
            break;

            case lt_using:
                ParseUsing(source);
                continue;

            case lt_word:
                type = TryLexenForType(source);
                if (type != nullptr)
                {
                    comma_type = type;
                    state = type_state;
                    source++;
                    continue;
                }

                if (linkage.storage_class != linkage_t::sc_default)
                {
                    CreateError(source.line_number, -777745, "type expected for variable declaration (%s)", source.value.c_str());
#if true  // true for C-ctyle, false for C++ style
                    type = GetBuiltinType(lt_type_int);
                    comma_type = type;  // Will not used for such case?
                    state = type_state;
#else
                    source.Finish();
#endif
                    continue;
                }
                variable = FindVariable(source.value);
                if (variable != nullptr)
                {
                    expression_t  *  code = ParseExpression(source);
                    if (code == nullptr)
                    {
                        source.Finish();
                        continue;
                    }

                    //
                    // add expressiob to list
                    //
                    this->space_code.push_back(code);
                    if (source != nullptr)
                    {
                        state = wait_delimiter_state;
                        //						printf("Skip something\n");
                    }
                    continue;
                }
                function = FindTemplateFunction(source.value);
                if (function != nullptr)
                {
                    name = function->name;
                    source++;
                    if (source == false)
                    {
                        CreateError(source.line_number, -7775132, "Non terminated statement");
                        continue;
                    }
                    if (source.lexem == lt_openbraket)
                    {
                        function = FindFunction(function->name);
                        if (function == nullptr)
                        {
                            CreateError(source.line_number, -7775132, "Not found instance of templated function '%s'", name.c_str());
                            source.Finish();
                            continue;
                        }
#if MODERN_COMPILER
                        call_t * call = function->TryCallFunction(this, SourcePtr(source.sequence));
#else
                        SourcePtr ptr(source.sequence);
                        call_t * call = function->TryCallFunction(this, ptr);
#endif
                        this->space_code.push_back(call);
                        source++;
                        continue;
                    }
                    if (source.lexem == lt_less)
                    {
                        function_parser * instance = CreateFunctionFromTemplate(function, ++source);
                        if (source.lexem == lt_openbraket)
                        {
#if MODERN_COMPILER
                            call_t	* call = instance->TryCallFunction(this, SourcePtr(source.sequence));
#else
                            SourcePtr ptr(source.sequence);
                            call_t	* call = instance->TryCallFunction(this, ptr);
#endif
                            this->space_code.push_back(call);
                            source++;
                            continue;
                        }
                    }
                    CreateError(source.line_number, -7775132, "Unparsed lexeme (%d) in namespace_t::ParseStatement(SourcePtr &source)", source.lexem);
                    source.Finish();
                    continue;
                }
                function = FindFunction(source.value);
                if (function != nullptr)
                {
                    // This code is logically almost same as in expression parser. But they are different
                    source++;
                    if (source == true)
                    {
                        if (source.lexem == lt_openbraket)
                        {
#if MODERN_COMPILER
                            call_t * call = function->TryCallFunction(this, SourcePtr(source.sequence));
#else
                            SourcePtr ptr(source.sequence);
                            call_t * call = function->TryCallFunction(this, ptr);
#endif
                            this->space_code.push_back(call);
                        }
                        else if (source.lexem == lt_less)
                        {
#if true
                            throw "FSM error on lt_word state in namespace_t parsing_state state";
#else
                            function_parser * instance = CreateFunctionFromTemplate(function, ++source);
                            if (source.lexem == lt_openbraket)
                            {
                                call_t	* call = function->TryCallFunction(this, SourcePtr(source.sequence));
                                this->space_code.push_back(call);
                            }
                            continue;
#endif
                        }
                        else
                        {
                            CreateError(source.line_number, -7775132, "Unparsed word in namespace_t::ParseStatement(SourcePtr &source)");
                            source.Finish();
                        }
                    }
                    source.Finish();
                    continue;
                }
                space = FindNamespace(source.value);
                if (space != nullptr)
                {
                    expression_t  *  code = ParseExpression(source);
                    this->space_code.push_back(code);
                    state = wait_delimiter_state;
                    continue;
                }

                if (source == true)
                {
                    std::string str = source.value;
                    source++;
                    if (source.lexem == lt_colon)
                    {
                        code = new operator_LABEL(str);
                        space_code.push_back(code);
                        state = parsing_state;
                        source++;
                        continue;
                    }
                }
                CreateError(source.line_number, -7776732, "identifier '%s' not found", source.value.c_str());
                //				__debugbreak();
                break;

            default:
                code = CheckOperators(source);
                if (code != nullptr)
                {
                    space_t * space = this;
                    while (space)
                    {
                        if (space->type == function_space)
                            break;
                        space = space->parent;
                    }
                    if (space != nullptr &&
                        (this->type != structure_space &&
                            this->type != name_space &&
                            this->type != template_space))
                    {
                        space_code.push_back(code);
                    }
                    else
                    {
                        CreateError(source.line_number, -7777732, "operators allowed only within functions");
                        source.Finish();
                        continue;
                    }
                    if (source == false)
                    {
                        //						printf("operator parsed\n");
                        continue;
                    }
                    if (source.lexem != lt_semicolon)
                    {
                        //                      fprintf(stderr, "Expected semicolon\n");
                    }
                    source++;
                    continue;
                }
                throw "Unparsed lexem in namespace_t::ParseStatement";
                break;
            }
            break;

        case check_compound_state:
            if (source.statements != nullptr)
            {
                type = ParseCompoundDefinition(name, source.previous_lexem, source.statements);
                ((structure_t*)type)->space->inherited_from = inherited_from;
                source++;
            }
            else
                type = FindType(name);
            comma_type = type;
            state = type_state;
            continue;

        case check_enumeration_state:
            if (source.statements != nullptr)
            {
                type = ParseEnumeration(name, source.statements);
                if (type != nullptr && !name.empty())
                    CreateType(type, name);
                source++;
            }
            else
                type = FindType(name);
            comma_type = type;
            state = type_state;
            continue;

        case wait_delimiter_state:
            if (source.lexem != lt_semicolon)
            {
                CreateError(source.line_number, -777715, "wrong delimiter (%d)", source.lexem);
                source.Finish();
                continue;
            }
            // on variable assignment in definition
            state = finish_state;
            source++;
            continue;

        case visibility_colon:
            if (source.lexem != lt_colon)
            {
                CreateError(source.line_number, -777715, "wrong delimiter (%d)", source.lexem);
                source.Finish();
                continue;
            }
            state = startup_state;
            source++;
            continue;

        default:
            throw "Unknown state in namespace_t::ParseStatement";
        }
    }

    return 0;
}
