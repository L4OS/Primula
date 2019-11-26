#include "namespace.h"

#include <map>

bool pragma_pack = false;

static inline void CalculateStructureSize(lexem_type_t kind, structure_t * structure)
{
	for (auto pair : structure->space->space_variables_map)
	{
		auto var = pair.second;
		int bitsize = var->type->bitsize;
		if (pragma_pack == false)
		{
			bitsize = ((((bitsize >> 3) + 3) / 4) * 4) << 3;
		}
		if (kind == lt_union)
		{
			if (structure->bitsize < bitsize)
				structure->bitsize = bitsize;
		}
		else
			structure->bitsize += bitsize;
	}
}

type_t * namespace_t::ParseCompoundDefinition(std::string parent_name, lexem_type_t kind, Code::statement_list_t *	statements)
{
	switch (kind)
	{
	case lt_struct:
	case lt_class:
	case lt_union:
		break;
	default:
		throw "namespace_t::ParseCompoundDefinition got wrong kind of comound type";
	}
	structure_t	*	structure = new structure_t(kind, parent_name);
	structure->space = CreateSpace(spacetype_t::structure_space, parent_name + "::");

	if (parent_name.size() > 0)
		this->CreateType(structure, parent_name);
	//else
	//	printf("debug: found unnamed struct\n");

	bool previous_state = structure->space->no_parse_methods_body;
	structure->space->no_parse_methods_body = true;
	structure->space->Parse(*statements);
	structure->space->no_parse_methods_body = previous_state;

	for (auto function : structure->space->function_list)
	{
		for (auto overload : function->overload_list)
		{
			if (overload->source_code != nullptr)
			{
				overload->space->Parse(*overload->source_code);
				//overload->source_code = nullptr;
			}
		}
	}

	CalculateStructureSize(kind, structure);
	return structure;
}

type_t * namespace_t::ParseEnumeration(std::string parent_name, Code::statement_list_t *	statements)
{
	enumeration_t * result = new enumeration_t(parent_name);

	enum {
		check_name_state,
		next_state,
		chageindex_state,
		negative_value_state
	} state = check_name_state;

	std::string		item_name;
	int				item_index = 0;
	auto  enumeration = statements->begin();
	if (enumeration != statements->end())
		for (auto item : *enumeration)
		{
			switch (state)
			{
			case check_name_state:
				if (item.lexem != lt_word)
				{
					CreateError(-7777789, "wrong enumeration item name", item.line_number);
					return result;
				}
				item_name = item.value;
				state = next_state;
				continue;

			case next_state:
				if (item.lexem == lt_set)
				{
					state = chageindex_state;
					continue;
				}
				// Add new value to enumeration
				result->enumeration.insert(std::make_pair(item_name, item_index));
				item_name.clear();
				item_index++;
				state = check_name_state;
				continue;

			case chageindex_state:
				if (item.lexem == lt_integer)
				{
					item_index = std::stoi(item.value);
					state = next_state;
					continue;
				}
				if(item.lexem == lt_sub)
				{
					state = negative_value_state;
					continue;
				}
on_error:
				CreateError(-7777689, "assign expression to enumerated item value not implemented", item.line_number);
				return result;

			case negative_value_state:
				if (item.lexem == lt_integer)
				{
					item_index = -std::stoi(item.value);
					state = next_state;
					continue;
				}
				goto on_error;

			default:
				throw "Wrong state in enumeration parser";
			}
		}
	// Add last item to enumeration
	if(item_name.size() > 0)
		result->enumeration.insert(std::make_pair(item_name, item_index));
	return result;
}

function_overload_t * namespace_t::CheckCostructorDestructor(linkage_t * linkage, type_t * type, std::string name, SourcePtr &source)
{
	function_overload_t * function;
	do
	{
		function = this->CreateFunction(type, name, source.sequence, linkage);
		if (function->function->type != type)
		{
			CreateError(-7777111, "namespace_t::CheckCostructorDestructor:: type mismatch", source.line_number);
			source.Finish();
			continue;
		}
		if (function->space != nullptr)
		{
			CreateError(-7777111, "Method already defined", source.line_number);
			source.Finish();
			continue;
		}
		source++;
		if (source == false)
		{
			CreateError(-7778911, "Non terminated function detected", source.line_number);
			source.Finish();
			continue;
		}
		if (source.lexem == lt_semicolon)
		{
			source++;
			continue;
		}
		if (source.lexem == lt_openblock)
		{
			function->Parse(this, source.statements);
			source++;
			continue;
		}
	} while (false);
	return function;
}

