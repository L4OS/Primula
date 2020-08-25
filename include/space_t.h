#pragma once

#include "statement_t.h"

#if MODERN_COMPILER
struct space_t
#else
class space_t
#endif
{
public:
    typedef enum spacetype_t
    {
        global_space,
        name_space,
        function_space,
        structure_space,
        switch_space,
        continue_space,
        codeblock_space,
        exception_handler_space,
        template_space
    } spacetype_t;

    typedef std::list<structure_t*>		parent_spaces_t;

    spacetype_t		                    type;
    std::string		                    name;
    namespace_t                     *   parent;
    parent_spaces_t						inherited_from;

    std::list<type_t*>                  space_types_list;
    std::list<variable_base_t*>         space_variables_list;
    std::list<statement_t *>            space_code;
    std::list<class function_parser*>   function_list;
};
