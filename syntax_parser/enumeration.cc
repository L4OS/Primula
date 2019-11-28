#include "namespace.h"

variable_base_t * namespace_t::TryEnumeration(std::string name, bool self_space)
{
	variable_base_t * result = nullptr;
	for (type_t * space_type : this->space_types_list)
	{
		type_t * type = space_type;
		while (true)
		{
			switch (type->prop)
			{
			case type_t::constant_type:
			{
				type = ((const_t*)type)->parent_type;
				continue;
			}
			case type_t::typedef_type:
				type = ((typedef_t*)type)->type;
				continue;
			}
			break;
		}
		if (type->prop == type_t::enumerated_type)
		{
			enumeration_t * en = (enumeration_t*)type;
			auto pair = en->enumeration.find(name);
			if (pair == en->enumeration.end())
				continue;

			result = new variable_base_t(this, type, name, linkage_t::sc_default);
			result->static_data = new static_data(pair->second);
			return result;
		}
	}

	return parent && !self_space ? parent->TryEnumeration(name, false) : result;
}


static bool TryRegisterEnumeration(namespace_t * space, std::string name, int value, int line_number)
{
	bool success = space->enum_map.count(name) == 0;
	if (success)
	{
		auto pair = std::make_pair(name, value);
		space->enum_map.insert(pair);
	}
	else
	{
		space->CreateError(-7777789, "%s redefinition", line_number, name.c_str());
	}
	return success;
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
			{
				if (item.lexem == lt_set)
				{
					state = chageindex_state;
					continue;
				}
				if (item.lexem != lt_comma)
				{
					CreateError(-7777789, "expected comma", item.line_number);
					return result;
				}
				bool success = TryRegisterEnumeration(this, item_name, item_index, item.line_number);
				if (!success)
				{
					return result;
				}
				// Add new value to enumeration (does it need for anything except restore source code?)
				auto pair = std::make_pair(item_name, item_index);
				result->enumeration.insert(pair);
				item_name.clear();
				item_index++;
				state = check_name_state;
				continue;
			}

			case chageindex_state:
				if (item.lexem == lt_integer)
				{
					item_index = std::stoi(item.value);
					state = next_state;
					continue;
				}
				if (item.lexem == lt_sub)
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
	if (item_name.size() > 0)
	{
		result->enumeration.insert(std::make_pair(item_name, item_index));
	}
	return result;
}

