#include "namespace.h"

expression_node_t * namespace_t::TryEnumeration(std::string name)
{
    expression_node_t * node = nullptr;
    if (this->enum_map.count(name) > 0)
    {
        auto pair = this->enum_map.find(name);
        int val = pair->second;

        node = new expression_node_t(lt_integer);
        node->type = this->GetBuiltinType(lt_type_int);
        node->is_constant = true;
        node->constant = new constant_node_t(val);
    }
    else if (parent != nullptr)
        return parent->TryEnumeration(name);
    return node;
}

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
		space->CreateError(line_number, -7777789, "%s redefinition", name.c_str());
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
    int             linenum = 0;
	auto  enumeration = statements->begin();
	if (enumeration != statements->end())
		for (auto item : *enumeration)
		{
			switch (state)
			{
			case check_name_state:
				if (item.lexem != lt_word)
				{
					CreateError(item.line_number, -7777789, "wrong enumeration item name");
					return result;
				}
				item_name = item.value;
				state = next_state;
                linenum = item.line_number;
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
					CreateError(item.line_number, -7777789, "expected comma");
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
				CreateError(item.line_number, -7777689, "assign expression to enumerated item value not implemented");
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
        bool success = TryRegisterEnumeration(this, item_name, item_index, linenum);
        if (!success)
        {
            return result;
        }
        result->enumeration.insert(std::make_pair(item_name, item_index));
	}
	return result;
}

