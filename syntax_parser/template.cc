#include "namespace.h"

static namespace_t * expression_space = nullptr;

expression_node_t::expression_node_t(const expression_node_t * node)
{
	this->lexem = node->lexem;
	this->postfix = node->postfix;
	this->is_constant = node->is_constant;
	if (node->type)
		this->type = expression_space->TryTranslateType(node->type);
	if (node->left)
		this->left = new expression_node_t(node->left);
	if (node->right)
		this->right = new expression_node_t(node->right);
	switch (node->lexem)
	{
	case lt_variable:
		this->variable = expression_space->TranslateVariable(expression_space, node->variable);
		break;
	default:
		break;
	}
}

expression_t * namespace_t::TranslateExpression(namespace_t * space, expression_t * exp)
{
	expression_space = this;
	expression_node_t * node = new expression_node_t(*exp->root);
	return new expression_t(node);
}

type_t * namespace_t::TryTranslateType(type_t * type)
{
	switch (type->prop)
	{
	case type_t::property_t::constant_type:
	{
		const_t * t = (const_t*)type;
		type = TryTranslateType(t->parent_type);
		break;
	}
	case type_t::property_t::pointer_type:
	{
		pointer_t * t = (pointer_t*)type;
		type = TryTranslateType(t->parent_type);
		break;
	}
	case type_t::property_t::template_type:
	case type_t::property_t::compound_type:
	{
		auto pair = instance_types.find(type->name);
		if (pair != instance_types.end())
		{
			type = pair->second;
			break;
		}
		if (this->parent == nullptr)
		{
			type = nullptr;
			break;
		}
		type = parent->TryTranslateType(type);
		break;
	}
	case type_t::property_t::dimension_type:
	{
		array_t * array = (array_t*)type;
		type = TryTranslateType(array->child_type);
		type = new array_t(type, array->items_count);
		break;
	}
	default:
		printf("Type not translated\n");
	}
	return type;
}

variable_base_t *  namespace_t::TranslateVariable(namespace_t * space, variable_base_t * v)
{
	variable_base_t * var = nullptr;
	type_t * t = TryTranslateType(v->type);
	if (t == nullptr)
		CreateError(-7775102, "Cannot find variaable type", 0);
	else
	{
		var = new variable_base_t(this, t, v->name, v->storage);
		if (v->declaration != nullptr)
		{
			var->declaration = v->declaration;
			//			throw "Translate variable declaration";
		}
	}
	return var;
}

void namespace_t::TranslateNamespace(namespace_t * space, int line_num)
{
	type_t * t = nullptr;

	for (auto parent_function : space->function_list)
	{
		function_parser * func = CreateFunctionInstance(parent_function, line_num);
		RegisterFunction(func->name, func, true);
	}

	for (auto statement : space->space_code)
	{
		switch (statement->type)
		{
		case statement_t::_expression:
			this->space_code.push_back(TranslateExpression(this, (expression_t *)statement));
			break;

		case statement_t::_return:
		{
			return_t * ret = new return_t;
			return_t * base = (return_t*)statement;
			if (base->return_value != nullptr)
			{
				ret->return_value = TranslateExpression(this, base->return_value);
			}
			this->space_code.push_back(ret);
			break;
		}

		case statement_t::_variable:
		{
			variable_base_t * var = TranslateVariable(space, (variable_base_t*)statement);
			this->space_code.push_back(var);
			break;
		}

		case statement_t::_call:
		{
			call_t * call = (call_t*)statement;
			this->space_code.push_back(call);
			break;
		}

		case statement_t::_if:
		{
			operator_IF * op = (operator_IF*)statement;
			this->space_code.push_back(op);
			break;
		}

		default:
			throw "template.cc: Add or skip in TranslateNamespace";
		}
	}
}

variable_base_t * namespace_t::CreateObjectFromTemplate(type_t * type, linkage_t * linkage, SourcePtr & source)
{
	enum {
		check_type,
		wait_delimiter,
		create_instance,
		check_arguments_exist,
		check_terminator,
		finish
	} state = check_type;

	variable_base_t	*	instance = nullptr;
	type_t			*	t = nullptr;

	structure_t	* template_structure = nullptr;
	structure_t	* instance_structure = nullptr;

	for (; source != false; source++)
	{
		switch (state)
		{
		case check_type:
			t = this->TryLexenForType(source);
			if (t == nullptr)
			{
				CreateError(-7775132, "Unable get type in template ", source.line_number);
				source.Finish();
				continue;
			}
			state = wait_delimiter;
			continue;

		case wait_delimiter:
			switch (source.lexem)
			{
			case lt_comma:
				printf("Found class template type - %s %s\n", name.c_str(), t->name.c_str());
				instance_types.insert(std::make_pair(t->name, t));
				state = check_type;
				continue;
			case lt_more:
				printf("Found class template type - %s %s\n", name.c_str(), t->name.c_str());
				instance_types.insert(std::make_pair(t->name.c_str(), t));
				state = create_instance;
				continue;
			default:
				CreateError(-7775132, "Wrong lexem on instance template", source.line_number);
				source.Finish();
				continue;
			}

		case create_instance:
			if (source.lexem != lt_word)
			{
				CreateError(-7735132, "instance object name exprected", source.line_number);
				source.Finish();
				continue;
			}
			if (type->prop == type_t::compound_type)
			{
				template_structure = (structure_t*)type;
				instance_structure = new structure_t(template_structure->kind, type->name + "_tm");
				instance_structure->space = new namespace_t(this, namespace_t::spacetype_t::structure_space, instance_structure->name);
				instance_structure->space->TranslateNamespace(template_structure->space, source.line_number);

				instance = new variable_base_t(instance_structure->space, instance_structure, source.value, linkage->storage_class);

				space_types_map.insert(std::make_pair(instance->type->name, instance->type));
				space_types_list.push_back(instance->type);
				state = check_arguments_exist;
			}
			else
				throw "State not parsed";
			continue;

		case check_arguments_exist:
			// TODO: Select approperiare constructor
			if (source.lexem == lt_openbraket)
			{
				call_t	* call_constructor;
				function_parser * constructor = instance_structure->space->FindFunction(instance_structure->name);
				function_overload_parser * overload = new function_overload_parser(constructor, linkage);
				if (constructor != nullptr)
				{
					call_constructor = new call_t(this, constructor->name, overload);
				}

				state = check_terminator;
				SourcePtr ptr(source.sequence);
				while (ptr != false)
				{
					expression_t * expr = ParseExpression(ptr);
					if (constructor == nullptr)
					{
						CreateError(-7775136, "object has no appropriate constructor", source.line_number);
						break;
					}
					call_constructor->arguments.push_back(expr);
					if (ptr == true && ptr.lexem != lt_comma)
					{
						CreateError(-7775136, "Wrong argument format", source.line_number);
						break;;
					}
					ptr++;
				}
				instance->declaration = call_constructor;
				continue;
			}
		case lt_semicolon:
		{
			linkage_t linkage; // Need take from template!!!
			CreateVariable(instance->type, instance->name, &linkage);
			printf("Declaration in instance of variable '%s%s' completed?\n", name.c_str(), instance->name.c_str());
			continue;
		}
		throw "Check if constructor without arguments";
		break;

		case check_terminator:
			if (source.lexem != lt_semicolon)
				throw "Semicolon expercted on declaration of template's instance";
			continue;
		}
	}

	return instance;
}

function_parser	* namespace_t::CreateFunctionInstance(function_parser * func, int line_number)
{
	function_parser	*	instance = nullptr;
	type_t		*	t;

	t = TryTranslateType(func->type);
	if (!t)
	{
		CreateError(-7775102, "Cannot find type", line_number);
	}
	else
	{
		instance = new function_parser(t, func->name + "_tm");
		instance->method_type = func->method_type;
		for (auto & overload : func->overload_list)
		{
			function_overload_parser * instance_overload = new function_overload_parser(instance, &overload->linkage);
			instance_overload->space = new namespace_t(this, namespace_t::function_space, func->name);
			for (auto & a : overload->arguments)
			{
				t = TryTranslateType(a.type);
				if (!t)
				{
					CreateError(-7775102, "Cannot find type", line_number);
					delete instance;
					instance = nullptr;
					break;
				}
				farg_t		arg(a);
				arg.type = t;
				instance_overload->arguments.push_back(arg);
			}
			instance_overload->MangleArguments();
			instance->overload_list.push_back(instance_overload);
			if (instance != nullptr)
				instance_overload->space->TranslateNamespace(overload->space, line_number);
		}
	}
	return instance;
}

function_parser	* namespace_t::CreateFunctionFromTemplate(function_parser * func, SourcePtr & source)
{
	function_parser	*	instance = nullptr;

	enum {
		check_type,
		wait_delimiter,
		create_instance,
		finish
	} state = check_type;

	type_t * t = nullptr;

	// We need here select best fit function from list of templated functions
	// But now we just take first

	std::list<farg_t>::iterator arg_ptr;
	for (auto temp_func : func->overload_list)
	{
		arg_ptr = temp_func->arguments.begin();
		printf("%s  %x\n", typeid(arg_ptr).name(), typeid(arg_ptr).hash_code());
		break;
	}

	instance_types.clear();

	for (; source != false; source++)
	{
		switch (state)
		{
		case check_type:
			t = this->TryLexenForType(source);
			if (t == nullptr)
			{
				CreateError(-7775132, "Unable get type in template ", source.line_number);
				source.Finish();
				continue;
			}
			state = wait_delimiter;
			continue;

		case wait_delimiter:
			switch (source.lexem)
			{
			case lt_comma:
				printf("Found template type - %s %s %s\n", arg_ptr->type->name.c_str(), arg_ptr->name.c_str(), t->name.c_str());
				instance_types.insert(std::make_pair(arg_ptr->type->name, t));
				++arg_ptr;
				state = check_type;
				continue;
			case lt_more:
				printf("Found template type - %s %s %s\n", arg_ptr->type->name.c_str(), arg_ptr->name.c_str(), t->name.c_str());
				instance_types.insert(std::make_pair(arg_ptr->type->name, t));
				++arg_ptr;
				state = create_instance;
				continue;
			default:
				CreateError(-7775132, "Wrong lexem on instance template", source.line_number);
				source.Finish();
				continue;
			}
			break;

		case create_instance:
			instance = CreateFunctionInstance(func, source.line_number);
			printf("Create function instance form template\n");
			state = finish;
			break;

		case finish:
			printf("This is function template instance finish\n");
			break;
		}

		if (state == finish)
			break;
	}
	return instance;
}

void namespace_t::ParseTemplate(SourcePtr & source)
{
	enum {
		entry_template,
		parameter_type,
		wait_delimiter,
		build_template
	} state = entry_template;

	lexem_type_t		type_id = lt_empty;
	type_t			*	type;
	std::string			type_name;

	template_t	*	x = new template_t("...to be overriten template name");
	x->space = new namespace_t(this, namespace_t::spacetype_t::template_space, "...to be overriten template space name");

	for (; source != false; source != false ? source++ : source)
	{
		switch (state)
		{
		case entry_template:
			if (source.lexem == lt_less)
			{
				state = parameter_type;
				continue;
			}
			throw "TODO: another forms of template or syntax error?";
			;
		case parameter_type:
			if (source.lexem == lt_class || source.lexem == lt_struct)
			{
				type_id = source.lexem;
				type_name = source.value;
				state = wait_delimiter;
				continue;
			}
			throw "TODO: another type of template"
				;
		case wait_delimiter:
		{
			structure_t	*	structure = nullptr;
			if (source.lexem == lt_more || source.lexem == lt_comma)
			{
				switch (type_id)
				{
				case lt_class:
				case lt_struct:
					structure = new structure_t(type_id, type_name);
					structure->space = x->space->CreateSpace(spacetype_t::structure_space, type_name + "::" + name);
					x->space->CreateType(structure, type_name);
					state = source.lexem == lt_more ? build_template : parameter_type;
					continue;
				}
				throw "type is not known in parser template.cc";
			}
			CreateError(-7775132, "Wrong delimiter in template arguments", source.line_number);
			source.Finish();
			continue;
		}
		break
			;
		case build_template:
			if (source.lexem != lt_struct && source.lexem != lt_class)
				//{
				//	throw "Wrong state at template parser";
				//}
				x->space->name = source.value;
			x->space->ParseStatement(source);
#if true
			printf("TODO store template\n");

			for (auto t : x->space->space_types_list)
				this->template_types_map.insert(std::make_pair(t->name, t));
			for (auto f : x->space->function_list)
				this->template_function_map.insert(std::make_pair(f->name, f));
#endif
			printf("Parse templated object\n");
			break;
		}
	}
}

