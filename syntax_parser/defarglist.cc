#if false

include "defarglist.h"
#include "lexem_tree_root.h"
#include "namespace.h"

arg_definintion_list_t		*	namespace_t::ParseArgunentDefinition(SourcePtr source)
{
	arg_definintion_list_t	*	result = new arg_definintion_list_t;
	type_t					*	type = nullptr;
	variable_base_t			*	argument = nullptr;
	std::string					name;

	enum {
		wait_type,
		wait_name,
		wait_delimiter,
		wait_closeindex,
		got_three_dots
	} state = wait_type;

	for (auto node = source; node != false; node++ )
	{ 
		switch (state)
		{
		case wait_type:
			if (node.lexem == lt_three_dots)
			{
				// We use at this point void type to emulate three dots behaviour
				type = GetBuiltinType(lt_void);
				argument = new variable_base_t(this, type, "...");
				result->push_back(*argument);
				state = got_three_dots;
				break;
			}
			type = GetBuiltinType(node.lexem);
			if (type == nullptr)
				return nullptr;
			state = wait_name;
			break;

		case wait_name:
			if (node.lexem == lt_mul)
			{
				type = new pointer_t(type);
				break;
			}
			if(node.lexem != lt_word)
				return nullptr;
			name = node.value;
			state = wait_delimiter;
			break;

		case wait_delimiter:
			if (node.lexem == lt_openindex)
			{
				state = wait_closeindex;
				break;
			}
			if(node.lexem != lt_comma)
				return nullptr;
			argument = new variable_base_t(this, type, name);
			result->push_back(*argument);
			state = wait_type;
			break;

		case got_three_dots:
			return nullptr;

		case wait_closeindex:
			if (node.lexem != lt_closeindex)
				throw "Does standard allow to use index in argument definition?";
			type = new pointer_t(type);
			state = wait_delimiter;
			break;
		}
	}
	if (state == wait_delimiter)
	{
		argument = new variable_base_t(this, type, name);
		result->push_back(*argument);
	}

	return result;
}

#endif
