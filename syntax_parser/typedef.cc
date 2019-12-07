#include "namespace.h"

type_t * namespace_t::TypeDefOpenBraket(SourcePtr & source, std::string & name, type_t * type)
{
	switch (source.lexem)
	{
	case lt_namescope:
	{
		type_t * parent = FindType(name);
		if (parent != nullptr && parent->prop == type_t::compound_type)
		{
			structure_t * structure = (structure_t*)parent;
			name.clear();
		}
		break;
	}
	case lt_mul:
		type = new pointer_t(type);
		break;
	case lt_word:
		if (!name.empty())
		{
			CreateError(source.line_number, -7777739, "Structure error in typedef");
			source.Finish();
			return nullptr;
		}
		name = source.value;
		break;
	case lt_openbraket:
	{
		SourcePtr	item(source.sequence);
		while (item != false)
		{
			type = TypeDefOpenBraket(item, name, type);
		}
		break;
	}
	case lt_openindex:
		type = ParseIndex(source, type);
		break;

	default:
		throw "namespace_t::ParseTypeDefinition: FSM unparsed lexem";
	}
	if (source == false)
		fprintf(stderr, "Debug and remove");
	source++;
	return type;
}

type_t * namespace_t::ParseIndex(SourcePtr & source, type_t * type)
{
	if (source.sequence == nullptr || source.sequence->size() == 0)
		type = new pointer_t(type);
	else
	{
		expression_t  * size_expr = ParseExpression(SourcePtr(source.sequence));
		if (size_expr != nullptr && size_expr->is_constant)
		{
			constant_node_t	* constant = (constant_node_t *)size_expr->root->constant;
			type = new array_t(type, constant->integer_value);
		}
		else
		{
			CreateError(source.line_number, -777414, "typedef: array size must be constant value");
			source.Finish();
			return nullptr;
		}
	}
	return type;
}

type_t * namespace_t::ParseTypeDefinition(SourcePtr &source)
{
	enum {
		check_typedef_state,
		typedef_struct_state,
		typedef_name_state,
		finish_typedef_state,
		typedef_enum_state,
		typedef_function_state,
		wait_delimiter_state,
		finish_state
	} state = check_typedef_state;

	type_t			*	type = nullptr;
	type_t			*	typetype = nullptr;
	type_t			*	funct_type = nullptr;
	lexem_type_t		compound_mode = lt_empty;
	std::string			def_name;

	if (source != false)
		source++;

	while (source != false)
	{
		switch (state)
		{
		case check_typedef_state:
			switch (source.lexem)
			{
			case lt_class:
			case lt_union:
			case lt_struct:
				def_name = source.value;
				state = typedef_struct_state;
				compound_mode = source.lexem;
				source++;
				continue;

			case lt_enum:
				def_name = source.value;
				state = typedef_enum_state;
				source++;
				continue;

			case lt_word:
				type = TryLexenForType(source);
				if (type == nullptr)
				{
					CreateError(source.line_number, -7777738, "Unparsed typedef construction (%s)", source.value);
					source.Finish();
					continue;
				}
				source++;
				state = typedef_name_state;
				continue;

			default:
				type = GetBuiltinType(source.lexem);
				if (type == nullptr)
				{
					CreateError(source.line_number, -7777738, "Unparsed lexem in parsing typedef " );
					source.Finish();
					continue;
				}
				source++;
				state = typedef_name_state;
				continue;
			}

		case typedef_struct_state:
			if (source.lexem != lt_openblock)
			{
				CreateError(source.line_number, -7777739, "structure definition error in typedef" );
				source.Finish();
				continue;
			}
			type = ParseCompoundDefinition(def_name, compound_mode, source.statements);
			if (type == nullptr)
			{
				CreateError(source.line_number, -7777434, "structure defintion error in typedef" );
				source.Finish();
				continue;
			}
			source++;
			state = typedef_name_state;
			continue;

		case typedef_name_state:
			switch (source.lexem)
			{
			case lt_mul:
			{
				std::string parentname = type->name;
				type = new pointer_t(type);
				type->name = parentname;
				break;
			}
			case lt_openbraket:
			{
				if (funct_type != nullptr)
					throw "namespace_t::ParseTypeDefinitionL FSM error on next bracket";
				funct_type = type;

				//if(name.size()> 0)
				//	throw "namespace_t::ParseTypeDefinitionL FSM error on name";
				name.clear();
				typetype = TypeDefOpenBraket(source, name, GetBuiltinType(lt_auto));
				state = finish_typedef_state;
				continue;
			}
			case lt_word:
				name = source.value;
				state = finish_typedef_state;
				break;
			default:
				CreateError(source.line_number, -777714, "typedef: typename expected");
				source.Finish();
				continue;
			}
			source++;
			continue;

		case typedef_enum_state:
		{
			if (source.lexem == lt_word)
			{
				def_name = source.value;
				type = FindType(def_name);
				source++;
			}
			if (source.lexem == lt_openblock)
			{
				if (type != nullptr)
				{
					CreateError(source.line_number, -777414, "typedef: type '%s' already defined", def_name.c_str());
					source.Finish();
					continue;
				}
				type = ParseEnumeration(def_name, source.statements);
				if (!def_name.empty())
				{
					CreateType(type, def_name);
				}
				source++;
			}
			if (type == nullptr)
			{
				CreateError(source.line_number, -777414, "typedef: enuneration definition error");
				source.Finish();
				continue;
			}
			while (source != false && source.lexem == lt_mul)
			{
				type = new pointer_t(type);
				source++;
			}
			if (source == false && source.lexem != lt_word)
			{
				CreateError(source.line_number, -777414, "typedef: expected enuneration type name");
				source.Finish();
				continue;
			}
			name = source.value;
			source++;
			if (source == false)
			{
				CreateError(source.line_number, -777414, "typedef: expected delimiter of union");
				source.Finish();
				continue;
			}
			if (source.lexem == lt_openindex)
			{
				type = ParseIndex(source, type);
				source++;
			}

			typedef_t * def = new typedef_t(name, typetype != nullptr && typetype->GetType() != lt_void ? typetype : type);
			this->CreateType(def, name);
			state = finish_state;
			continue;
		}

		case finish_typedef_state:
			if (source.lexem == lt_semicolon)
			{
				typedef_t * def = new typedef_t(name, typetype != nullptr && typetype->GetType() != lt_void ? typetype : type);
				this->CreateType(def, name);
				source++;
				state = finish_state;
				continue;
			}
			if (source.lexem == lt_openbraket)
			{
				if (this->FindType(name) != nullptr)
				{
					this->CreateError(source.line_number, -7770320, "type '%s' already defined", name.c_str());
					source.Finish();
					continue;
				}
				// use default linkage for typedefs
				linkage_t linkage;
				function_parser * function_ptr = new function_parser(type, name);
				function_overload_parser * overload = new function_overload_parser(function_ptr, &linkage);
				function_ptr->prop = type_t::property_t::funct_ptr_type;
				if(source.sequence->size() > 0)
					overload->ParseArgunentDefinition(this, source.sequence);

				function_ptr->RegisterFunctionOverload(overload);

				if (typetype != nullptr)
				{
					type_t * t = typetype;
					if (typetype->prop == type_t::auto_type)
					{
						fprintf(stderr, "Catch typetype\n");
						typetype = function_ptr;
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
						fprintf(stderr, "Catch typetype\n");
						//pointer_t	*	parent = (pointer_t*)t;
						//parent->parent_type = function_ptr;
						typetype = function_ptr;
					}
					CreateType(new typedef_t(name, typetype), name);
				}
				else
					CreateType(new typedef_t(name, function_ptr), name);
				source++;
				if (source != false && source.lexem == lt_const)
				{
					overload->read_only_members = true;
					source++;
				}
				state = finish_state;
				continue;
				//state = typedef_function_state;
				//continue;
			}
			if (source.lexem == lt_openindex)
			{
				type = ParseIndex(source, type);
				source++;
				continue;
			}

			CreateError(source.line_number, -777315, "typedef: delimiter expected");
			source.Finish();
			continue;

		case finish_state:
			if (source.lexem == lt_semicolon)
			{
				// Ok
				source++;
				continue;
			}
			CreateError(source.line_number, -777315, "typedef: unexpected lexem instead of semicolon ar typedef paring");
			source.Finish();
			continue;

		default:
			throw "namespace_t::ParseTypeDefinition:  FSM error";
		}
	}
	return type;
}
