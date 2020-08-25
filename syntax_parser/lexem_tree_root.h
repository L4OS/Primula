#pragma once

#include <list>
#include <stack>

#include "../include/lexem.h"

class Code
{

public:
	struct lexem_node;
	typedef	std::list<struct lexem_node>		lexem_list_t;
	typedef std::list<lexem_list_t>				statement_list_t;

	typedef struct lexem_node
	{
		lexem_type_t	lexem;
		int				line_number;
		//		union 
		//		{
		const char						*	value;
		lexem_list_t					*	sequence;
		statement_list_t				*	statements;
		//		};

		lexem_node()
		{
			lexem = lt_empty;
			value = 0;
			sequence = 0;
			statements = 0;
		}
	} lexem_node_t;

	virtual statement_list_t		CodeThree() = 0;
	virtual void AddLexem(lexem_event_t * ev) = 0;
};

Code * CreateCodeSplitter();
