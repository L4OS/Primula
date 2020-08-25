#include "namespace.h"

#include <map>

bool pragma_pack = false;

static inline void CalculateStructureSize(lexem_type_t kind, structure_t * structure)
{
#if MODERN_COMPILER
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
#else
    std::map<std::string, variable_base_t*>::iterator pair;
    for(
        pair = structure->space->space_variables_map.begin();
        pair != structure->space->space_variables_map.end();
        ++pair)
    {
        variable_base_t * var = pair->second;
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
#endif
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
#if MODERN_COMPILER
	structure->space = CreateSpace(spacetype_t::structure_space, parent_name + "::");
#else
    structure->space = CreateSpace(structure_space, parent_name + "::");
#endif

	if (parent_name.size() > 0)
		this->CreateType(structure, parent_name);
	//else
	//	printf("debug: found unnamed struct\n");

	bool previous_state = structure->space->no_parse_methods_body;
	structure->space->no_parse_methods_body = true;
	structure->space->Parse(*statements);
	structure->space->no_parse_methods_body = previous_state;
#if MODERN_COMPILER
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
#else
    std::list<class function_parser*>::iterator   function;
    function_overload_list_t::iterator overload;
    for (function = structure->space->function_list.begin(); function != structure->space->function_list.end(); ++function)
    {
        for (overload = (*function)->overload_list.begin(); overload != (*function)->overload_list.end(); ++overload)
        {
            if ((*overload)->linkage.storage_class != linkage_t::sc_abstract)
            {
                if ((*overload)->source_code != nullptr)
                {
                    if ((*overload)->space != nullptr)
                    {
                        (*overload)->space->Parse(*(*overload)->source_code);
                        (*overload)->source_code = nullptr;
                    }
                    else
                        throw "FSM error: methods space lost";
                }
            }
        }
    }
#endif
	CalculateStructureSize(kind, structure);
	return structure;
}



function_overload_t * namespace_t::CheckCostructorDestructor(linkage_t * linkage, type_t * type, std::string name, SourcePtr &source)
{
	function_overload_t * function;
	do
	{
		function = this->CreateFunction(type, name, source.sequence, linkage);
		if (function->function->type != type)
		{
			CreateError(source.line_number, -7777111, "namespace_t::CheckCostructorDestructor:: type mismatch");
			source.Finish();
			continue;
		}
		if (function->space != nullptr)
		{
			CreateError(source.line_number, -7777111, "Method already defined");
			source.Finish();
			continue;
		}
		source++;
		if (source == false)
		{
			CreateError(source.line_number, -7778911, "Non terminated function detected");
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
			function->Parse(source.line_number, this, source.statements);
			source++;
			continue;
		}
	} while (false);
	return function;
}

