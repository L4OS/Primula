#include "namespace.h"
#include "lexem_tree_root.h"

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

type_t * namespace_t::TryLexenForType(SourcePtr &source)
{
	type_t * type = nullptr;
	if (source.lexem == lt_const)
	{
		source++;
		if (source == false)
			return nullptr;
		type = TryLexenForType(source);
		if(type == nullptr)
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
		current_space = current_space->CreateSpace(namespace_t::spacetype_t::name_space, spacename);
		// call to parser namesoace's statements
		for (auto statement : *source.statements)
		{
			current_space->ParseStatement(statement);
		}
		fprintf(stderr, "TODO: Register namespace %s\n", spacename.c_str());
		current_space = current_space->Parent();
		source++;
		break;

	default:
		return -1;
	}

	return 0;
}

variable_base_t * namespace_t::FindVariable(std::string name)
{
	auto pair = space_variables_map.find(name);
	if (pair == space_variables_map.end())
	{
		if (this->type == function_space || this->type == exception_handler_space)
		{
			variable_base_t * arg = owner_function->FindArgument(name);
			if (arg != nullptr)
				return arg;
		}
		variable_base_t * enumeration = TryEnumeration(name, true);
		if (enumeration != nullptr)
			return enumeration;
		if (this->parent == nullptr)
			return nullptr;
		return parent->FindVariable(name);
	}
	return pair->second;
}

function_parser		* namespace_t::FindFunctionInSpace(std::string name)
{
	auto pair = function_map.find(name);
	if (pair != function_map.end())
		return pair->second;
	function_parser * f = nullptr;;
	for (auto inherited_from : this->inherited_from)
	{
		f = inherited_from->space->FindFunctionInSpace(name);
		if (f != nullptr)
			break;
	}
	return f;
}

function_parser		* namespace_t::FindFunction(std::string name)
{
	function_parser * f = FindFunctionInSpace(name);
	return f != nullptr ? f : this->parent == nullptr ? nullptr : parent->FindFunction(name);
}

function_parser		* namespace_t::FindTemplateFunction(std::string name)
{
	auto pair = template_function_map.find(name);
	if (pair != template_function_map.end())
		return pair->second;
	if (this->parent != nullptr)
		return parent->FindTemplateFunction(name);
	return nullptr;
}

variable_base_t * namespace_t::CreateVariable(type_t * type, std::string name, linkage_t * linkage)
{
	auto variable = new variable_base_t(this, type, name, linkage->storage_class);
	space_variables_map.insert(std::make_pair(name, variable));
	space_variables_list.push_back(variable);
	this->space_code.push_back(variable);
	return variable;
}

void namespace_t::RegisterFunction(std::string name, function_parser * function, bool back)
{
	function_map.insert(std::make_pair(name, function));
	if(back)
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
	auto pair = space_types_map.find(name);
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
		for (Code::lexem_list_t statement : source)
		{
			SourcePtr source(&statement);
			ParseStatement(source);
		}
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
			CreateError(-77777, "storage duplication", source.line_number);
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
				linkage->storage_class = linkage_t::sc_inline;
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
		if(compound != nullptr)
		{ 
			if (compound->prop == type_t::compound_type)
			{
				if (source.lexem != lt_word)
					throw "Select statement FSM error: lexem is not word";
				structure_t	* structure = (structure_t*)compound;
				name = source.value;
				source++;
				structure->space->SelectStatement(type, linkage, source.value, source);
			}
			else
			{
				CreateError(-7777726, "simple type has no namespace", source.line_number);
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
			CreateError(-77777, "Variable already defined", source.line_number);
			source.Finish();
			return;
		}
		variable = CreateVariable(type, name, linkage);
		if (source.lexem == lt_semicolon)
			return;
		if (source.lexem == lt_comma)
			return;
		if (source.lexem == lt_set)
		{
			source++;
			if (source.lexem == lt_openblock)
			{
				variable->static_data = BraceEncodedInitialization(type, source);
				return;
			}
			expression_t * assign = ParseExpression(source);
			{
                
                bool is_zero = assign->is_constant ? 
                    assign->type->prop == type_t::signed_type ?
                    (assign->root->constant->integer_value == 0) : false : false;

                if (CompareTypes(variable->type, assign->type, false, is_zero) == no_cast)
                {
                    CreateError(-777776, "Cannot cast %s to %s", source.line_number, 
                        assign->type->name, variable->type->name);
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
			CreateError(-777776, "Broken assignment code", source.line_number);
			return;
		}
		if (source.lexem == lt_openblock)
		{
			switch (variable->type->prop)
			{
			case type_t::compound_type:
			case type_t::property_t::pointer_type:
				throw "todo: parse struct and array initialization";
			default:
				CreateError(-777776, "Assignment bloack format error", source.line_number);
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
				fprintf(stderr, "This is an instance of %s %s(%s)\n", type->name.c_str(), name.c_str(), args.value.c_str());
				// TODO: Check that constructor is matched to arguemts

				variable_base_t * instance = CreateVariable(type, name, linkage);
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
		}
		source++;
		if (source == false)
		{
			CreateError(-7777774, "non-terminated function definition", source.line_number);
			return;
		}

		function = CreateFunction(type, name, sequence, linkage);
		if (source.lexem == lt_const)
		{
			function->read_only_members = true;
			source++;
			if (source == false)
			{
				CreateError(-7777774, "non-terminated function definition", source.line_number);
				return;
			}
		}
		if (source.lexem == lt_openblock)
		{
			if (this->type == spacetype_t::function_space)
			{
				CreateError(-7777774, "function inside function not allowed", source.line_number);
				source.Finish();
				return;
			}
			if (function->space != nullptr)
			{
				CreateError(-77777, "Function already defined in current namespace", source.line_number);
				source.Finish();
				return;
			}
			function->Parse(this, source.statements);
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
						CreateError(-7777414, "Non virtual method marked as abstract", source.line_number);
						source.Finish();
						break;
					}
				}
			}
		}
	}
	CreateError(-7777401, "Unaprsed lexem on function definition|declaration", source.line_number);
	source.Finish();
	break;

	case lt_openindex:
#if true // Is this check unneccessary?
		variable = FindVariable(name);
		if (variable != nullptr && variable->space == this)
		{
			CreateError(-7777400, "Variable already defined", source.line_number);
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
				expression_t * expr = ParseExpression(SourcePtr(source.sequence));
				if (expr == nullptr)
				{
					CreateError(-777743, "broken expression in array definition", source.line_number);
					source.Finish();
					return;
				}
				if (!expr->is_constant)
				{
					CreateError(-777745, "dimension size is not constant", source.line_number);
					source.Finish();
					return;
				}
				// Check size of dimension
				int dimsize = expr->root->constant->integer_value;
				type = new array_t(type, dimsize);
			}
			source++;
		} while (source== true && source.lexem == lt_openindex);
		variable = CreateVariable(type, name, linkage);
		return;

	case lt_colon:
		if (++source == false || source.lexem != lt_integer)
		{
			CreateError(-7777914, "Expected field bitsize", source.line_number, source.lexem);
			source.Finish();
			return;
		}
		else
		{
			int field_size = std::stoi(source.value);
			type->bitsize = field_size;
			if (++source == false || source.lexem != lt_semicolon)
			{
				CreateError(-7777914, "Bad termination of class|struct field definition", source.line_number, source.lexem);
				source.Finish();
				return;
			}
			variable = CreateVariable(type, name, linkage);
		}
		return;

	default:
		CreateError(-7777774, "Unparsed lexem '%d' on selection state in statement parser", source.line_number, source.lexem);
		source.Finish();
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

	type_t                          *	type; // = nullptr;
	variable_base_t                 *	variable; // = nullptr;
	function_parser                 *	function; // = nullptr;
	statement_t						*	code; // = nullptr;

	static int debug_line = 0;
	while (source != false)
	{
		if (debug_line == source.line_number)
		{
			CreateError(0, "Statement Breakpoint", source.line_number);
		}
		switch (state)
		{
		case startup_state:
			type = nullptr;
			variable = nullptr;
			function = nullptr;
			code = nullptr;
			CheckStorageClass(source, &linkage);
			state = parsing_state;
			continue;

		case finish_state:
			CreateError(-777778, "Lexem below statement terminator", source.line_number);
			source.Finish();
			continue;

		case type_state:
			switch (source.lexem)
			{
			case lt_namescope:
				if (type->prop == type_t::property_t::compound_type)
				{
					structure_t * structure = (structure_t *)type;
					structure->space->ParseStatement(++source);
					continue;
				}
				CreateError(-777778, "Type is not structrue. But maybe not error", source.line_number);
				source.Finish();
				continue;

			case lt_semicolon:
				if (type->prop != type_t::property_t::compound_type)
				{
					CreateError(-7777774, "stament not define anything", source.line_number);
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
				if (this->type != spacetype_t::structure_space)
				{
					CreateError(-7777774, "function inside function not allowed", source.line_number);
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
							CreateError(-7777074, "wrong declaration of constructor", source.line_number);
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
					v = new variable_base_t(this, vartype, name, linkage.storage_class);
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
							fprintf(stderr, "Catch typetype\n");
							parent->parent_type = f->function->type;
						}
					}
					v = new variable_base_t(this, vartype, name, linkage.storage_class);
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
					CreateError(-7777074, "wrong declaration of constructor", source.line_number);
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
					CreateError(-7771393, "inheritance allowed only for classes and structures", source.line_number);
					source.Finish();
					continue;
				}
				lexem_type_t	keep_type = source.previous_lexem;
				source++;
				if (source == false || source.lexem != lt_word)
				{
					CreateError(-7771393, "syntax error on type inheritance", source.line_number);
					source.Finish();
					continue;
				}
				do 
				{
					structure_t * parent_type = (structure_t*) this->FindType(source.value);
					if (parent_type == nullptr)
					{
						CreateError(-7771393, "parent type not found", source.line_number);
						source.Finish();
						continue;
					}
					if (parent_type->prop != parent_type->compound_type)
					{
						CreateError(-7771393, "parent type is not compound", source.line_number);
						source.Finish();
						continue;
					}
					fprintf(stderr, "TODO: Assing parent type '%s' to %s\n", parent_type->name.c_str(), name.c_str());
					inherited_from.push_back(parent_type);
					source++;
					if (source != true)
					{
						CreateError(-7771393, "non-termiated inheritance", source.line_number);
						continue;
					}
					if (source.lexem == lt_openblock)
					{
						source.previous_lexem = keep_type;
						state = check_compound_state;
						break;
					}
					if(source.lexem != lt_comma)
					{
						CreateError(-7771393, "wrong delimiter", source.line_number);
						source.Finish();
						continue;
					}
					source++;
				} 
				while (source == true);

			}
				continue;

			default:
				throw "Wrong lexeme at type_state in namespace_t::ParseStatement";
			}
			break;

		case select_state:
			SelectStatement(type, &linkage, name, source);
			if (source != false)
				switch (source.lexem)
			{
			case lt_semicolon:
				state = startup_state;
				source++;
				continue;
			case lt_comma:
				state = type_state;
				source++;
				continue;
			case lt_set:
				source++;
				if (source == false)
				{
					CreateError(-7775312, "no data found for declaration", source.line_number);
					continue;
				}
				variable = FindVariable(name);
				if (variable == nullptr)
				{
					CreateError(-7777777, "internal syntax parser error", source.line_number);
					source.Finish();
					continue;
				}
				if (source.lexem != lt_openblock)
				{
					throw "Internal syntax parser error in namespace_t::ParseStatement";
				}
				variable->static_data = BraceEncodedInitialization(variable->type, source);
				if(variable->static_data->type != lt_openblock)
					throw "Second Internal syntax parser error in namespace_t::ParseStatement";
				if (variable->type->prop == type_t::dimension_type)
				{
					array_t * array = (array_t*)variable->type;
					int real_count = variable->static_data->nested->size();
					int type_count = array->items_count;
					if (type_count == 0 && array->bitsize == 32)
					{
						array->items_count = real_count;
						array->bitsize = real_count * array->child_type->bitsize;
					}
					else if (type_count < real_count)
					{
						CreateError(-7779007, "too many initializer for array", source.line_number);
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
			else
				fprintf(stderr, "TODO: Check this on select_state\n");
			break; // end of select state

		case parsing_state:
			type = GetBuiltinType(source.lexem);
			if (type != nullptr)
			{
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
				state = type_state;
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
					CreateError(-777745, "destructor on non-compound type not implemented", source.line_number);
					source.Finish();
					continue;
				}
				state = type_state;
				source++;
				continue;

			case lt_private:
				this->current_visibility = visibility_t::vs_private;
				source++;
				state = visibility_colon;
				break;
			case lt_protected:
				this->current_visibility = visibility_t::vs_protected;
				source++;
				state = visibility_colon;
				break;
			case lt_public:
				this->current_visibility = visibility_t::vs_public;
				source++;
				state = visibility_colon;
				break;

			case lt_tilde:
				if (this->type != structure_space)
				{
					CreateError(-777745, "destructor on non-compound type not implemented", source.line_number);
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
			{
				expression_t  *  code = ParseExpression(source);
				//
				// add expressiob to list
				//
				this->space_code.push_back(code);
				if (source != false)
				{
					fprintf(stderr, "Skip something\n");
					source.Finish();
				}
				continue;
			}

			case lt_semicolon:
				fprintf(stderr, "Igmore semicolon in namespace_t::CheckOperators\n");
				source++;
				break;

            case lt_openblock:
                current_space = this->CreateSpace(namespace_t::spacetype_t::name_space, "just_block");
                // call to parser namesoace's statements
                for (auto statement : *source.statements)
                {
                    current_space->ParseStatement(statement);
                }
                current_space = current_space->Parent();
                fprintf(stderr, "Don't forget insert this block to code!\n");
                source++;
                break;

			case lt_word:
				type = TryLexenForType(source);
				if (type != nullptr)
				{
					state = type_state;
					source++;
					continue;
				}

				if (linkage.storage_class != linkage_t::storage_class_t::sc_default)
				{
					CreateError(-777745, "type expected for variable declaration", source.line_number);
#if true  // true for C-ctyle, false for C++ style
					type = GetBuiltinType(lt_type_int);
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
					if(source != nullptr)
					{
						state = wait_delimiter_state;
//						printf("Skip something\n");
					}
					continue;
				}
				function = FindTemplateFunction(source.value);
				if (function != nullptr)
				{
					source++;
					if(source == false)
					{
						CreateError(-7775132, "Non terminated statement", source.line_number);
						continue;
					}
					if (source.lexem == lt_openbraket)
					{
						function = FindFunction(function->name);
						if (function == nullptr)
						{
							CreateError(-7775132, "Not found instance of templated function", source.line_number);
							source.Finish();
							continue;
						}
						call_t * call = function->TryCallFunction(this, SourcePtr(source.sequence));
						this->space_code.push_back(call);
						source++;
						continue;
					}
					if (source.lexem == lt_less)
					{
						function_parser * instance = CreateFunctionFromTemplate(function, ++source);
						if (source.lexem == lt_openbraket)
						{
							call_t	* call = instance->TryCallFunction(this, SourcePtr(source.sequence));
							this->space_code.push_back(call);
							source++;
							continue;
						}
					}
					CreateError(-7775132, "Unparsed lexeme in namespace_t::ParseStatement(SourcePtr &source)", source.line_number);
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
							call_t * call = function->TryCallFunction(this, SourcePtr(source.sequence));
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
							CreateError(-7775132, "Unparsed word in namespace_t::ParseStatement(SourcePtr &source)", source.line_number);
							source.Finish();
						}
					}
					source.Finish();
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
				CreateError(-7776732, "identifier '%s' not found", source.line_number, source.value.c_str());
//				__debugbreak();
				break;

			default:
				code = CheckOperators(source);
				if (code != nullptr)
				{
					if (this->type == spacetype_t::function_space || 
						this->type == spacetype_t::switch_space ||
						this->type == spacetype_t::codeblock_space ||
						this->type == spacetype_t::continue_space ||
						this->type == namespace_t::exception_handler_space)
					{
						space_code.push_back(code);
					}
					else
					{
						CreateError(-7777732, "operators allowed only within functions", source.line_number);
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
			state = type_state; // wait_delimiter_state;
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
			state = type_state;
			continue;

		case wait_delimiter_state:
			if (source.lexem != lt_semicolon)
			{
				CreateError(-777715, "wrong delimiter", source.line_number);
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
				CreateError(-777715, "wrong delimiter", source.line_number);
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
