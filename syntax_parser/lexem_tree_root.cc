#include "../include/lexem.h"
#include "lexem_tree_root.h"
#include "../include/type_t.h"
#include <stdio.h>

class CodeSplitter : public Code
{
	typedef std::stack<lexem_list_t*>		node_stack_t;
	typedef std::stack<statement_list_t*>	statement_stack_t;

	statement_list_t			statement_list;
	lexem_list_t				lexem_list;
	lexem_list_t			*	current_node;
	statement_list_t		*	current_statement;
	node_stack_t				stack;
	statement_stack_t			statement_stack;
	std::stack<lexem_type_t>	mode_stack;

	void FixThree(statement_list_t * list)
	{
		statement_list_t::iterator	statement_src = list->begin();
		statement_list_t::iterator	statement_eof = list->end();
		statement_list_t::iterator	prev = statement_eof;
		lexem_list_t	*	fix = nullptr;
		bool last_result = false;
        bool    fix_dowhile = false;
        while (statement_src != statement_eof)
		{

			lexem_list_t::iterator  node = statement_src->begin();
			lexem_list_t::iterator  end = statement_src->end();
			bool	check_block = false;
            bool    keep_prev = false;

			if (fix != nullptr)
			{
#if MODERN_COMPILER
				for (auto add : *statement_src)
                {
                    fix->push_back(add);
                }
#else
                lexem_list_t::iterator  add;
                for(add = node; add != end; add++)
                {
                    fix->push_back(*add);
                }
#endif
				statement_list_t::iterator	doomed = statement_src;
				statement_src++;
				list->erase(doomed);
				fix = nullptr;
				continue;
			}
			else while (node != end)
			{
				switch (node->lexem)
				{
				case lt_do:
                    check_block = true;
                    fix_dowhile = true;
                    fix = &*statement_src;
                    break;

                case lt_while:
                    if (fix_dowhile)
                    {
                        fix_dowhile = false;
                    }
                    break;

				case lt_struct:
				case lt_union:
				case lt_enum:
				case lt_class:
                case lt_try:
                    //printf("Found that\n");
					check_block = true;
					break;

				case lt_openblock:
					FixThree(node->statements);
					if (check_block)
					{
						fix = &*statement_src;
						check_block = false;
                        fix_dowhile = false;
					}
					break;

				case lt_else:
					while (node != end)
					{
                        if (node->lexem == lt_openblock)
                            FixThree(node->statements);
						prev->push_back(*node);
						node++;
					}
                    {
                        keep_prev = true;
                        statement_list_t::iterator	remove = statement_src;
                        statement_src++;
                        list->erase(remove);
                        check_block = false;
                        if (statement_src == statement_eof)
                            return;
                    }
					continue;

                default:
					check_block = false;
					break;
				}
				node++;
			}
            if (keep_prev)
                keep_prev = false;
            else
            {
                prev = statement_src;
                if (statement_src == statement_eof)
                    break;
                statement_src++;
            }
		}
	}

public:
	statement_list_t		CodeThree()
	{
		FixThree(&statement_list);
		return statement_list;
	}

	CodeSplitter()
	{
		current_node = &lexem_list;
		current_statement = &statement_list;
	}

	void AddLexem(lexem_event_t * ev)
	{
		lexem_node_t	lex;
		lex.lexem = ev->lexem_type;
		lex.line_number = ev->line_number;
		lex.value = ev->lexem_value;

		if (mode_stack.empty()) switch (ev->lexem_type)
		{
		case lt_openblock:
			lexem_list.push_back(lex);
			current_statement->push_back(lexem_list);
			lexem_list.clear();

			statement_stack.push(current_statement);
			current_statement = new statement_list_t; // (statement_list_t*) lex.statements;
			break;

		case lt_closeblock:
		{
			if (!lexem_list.empty())
			{
				current_statement->push_back(lexem_list);
				lexem_list.clear();
			}

			statement_list_t *  prev_statement = current_statement;
			current_statement = statement_stack.top();
			statement_stack.pop();

			lexem_list_t * tail = &current_statement->back();
			lexem_node_t * node = &tail->back();
			node->statements = prev_statement;
		}
		break;

		case lt_openbraket:
		case lt_openindex:
			lex.sequence = new lexem_list_t;
			current_node->push_back(lex);
			stack.push(current_node);
			current_node = lex.sequence;
			mode_stack.push(lex.lexem);
			break;

		case lt_semicolon:
			lexem_list.push_back(lex);
			current_statement->push_back(lexem_list);
			lexem_list.clear();
			break;

		default:
			current_node->push_back(lex);
		}
		else switch (ev->lexem_type)
		{
		case lt_openbraket:
		case lt_openindex:
			lex.sequence = new lexem_list_t;
			current_node->push_back(lex);
			stack.push(current_node);
			current_node = lex.sequence;
			mode_stack.push(ev->lexem_type);
			break;

		case lt_closebraket:
		case lt_closeindex:
		{
			lexem_type_t prev = lt_empty;

			if (mode_stack.size() > 0)
			{
				prev = mode_stack.top();
				mode_stack.pop();
			}
			switch (prev)
			{
			case lt_openbraket:
				if (lt_closebraket != ev->lexem_type)
					prev = lt_empty;
				break;
			case lt_openindex:
				if (lt_closeindex != ev->lexem_type)
					prev = lt_empty;
				break;
			case lt_empty:
				break;
			default:
				throw "TODO: Fix first stage of syntax parser";
			}
			if (prev == lt_empty)
			{
				if (lt_closebraket == ev->lexem_type)
					printf("ERROR: ')' is not preceded by the opening parenthesis.");
				else
					printf("ERROR: ']' is not preceded by the opening square bracket.");
			}
			else
			{
				current_node = stack.top();
				stack.pop();
			}
			break;
		}

		default:
            current_node->push_back(lex);
		}
	}
};

Code * CreateCodeSplitter()
{
	return new CodeSplitter;
}
