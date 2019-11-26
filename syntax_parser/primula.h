#pragma once

#include "lexem_tree_root.h"

class SourcePtr
{
public:
	Code::lexem_list_t::iterator				index;
	Code::lexem_list_t::iterator				eof;
	lexem_type_t								lexem;
	lexem_type_t								previous_lexem;
	int											line_number;
	Code::lexem_list_t						*	sequence;
	Code::statement_list_t					*	statements;
	std::string									value;


	SourcePtr& operator++()
	{
		// actual increment takes place here
		previous_lexem = lexem;
		*this->index++;
		if (index != eof)
		{
			lexem = index->lexem;
			line_number = index->line_number;
			sequence = index->sequence;
			statements = index->statements;
			if (index->value)
				value = index->value;
		}
		return *this;
	}

	// postfix
	SourcePtr operator++(int)
	{
		SourcePtr tmp(*this);
		operator++(); // prefix-increment this instance
		return tmp;   // return value before increment
	}

	friend bool operator != (const SourcePtr &source, const SourcePtr &eof)
	{
		return source.index != eof.eof;
	}

	friend bool operator == (const SourcePtr &source, const SourcePtr &eof)
	{
		return source.index == eof.eof;
	}

	friend bool operator == (const SourcePtr &source, const bool val)
	{
		return (source.index != source.eof) == val;
	}

	friend bool operator != (const SourcePtr &source, const bool val)
	{
		return (source.index != source.eof) != val;
	}

	void Finish(void)
	{
		index = eof;
	}

	void SetLength(SourcePtr end)
	{
		eof = end.index;
	}

	bool IsFinished()
	{
		return index == eof;
	}

	SourcePtr(SourcePtr source, lexem_type_t delimiter, bool mandatory)
	{
		index = source.index;
		if (source == true)
		{
			for (eof = index; eof != source.eof; eof++)
				if (eof->lexem == delimiter)
				{
					lexem = index->lexem;
					line_number = index->line_number;
					sequence = index->sequence;
					statements = index->statements;
					if (index->value)
						value = index->value;
				}
			if (!mandatory &&  eof == source.eof)
				eof = index;
		}
		else
			eof = index;
	}

	SourcePtr(Code::lexem_list_t * items)
	{
		index = items->begin();
		eof = items->end();
		if (items->size())
		{
			lexem = index->lexem;
			line_number = index->line_number;
			sequence = index->sequence;
			statements = index->statements;
			if (index->value)
				value = index->value;
		}
	}

};

typedef struct shunting_yard
{
	lexem_type_t								lexem;
	bool										unary;
	int											line_number;
	std::string									text;
	struct shunting_yard					*	left;
	struct shunting_yard					*	right;
	class type_t							*	type;
	lexem_type_t								postfix;

	shunting_yard(SourcePtr &source)
	{
		lexem = source.lexem;
		line_number = source.line_number;
		text = source.value.data();
		left = right = nullptr;
		unary = IsUnary(lexem);
		type = nullptr;
		postfix = lt_empty;
	}

	shunting_yard(const shunting_yard & yard)
	{
		lexem = yard.lexem;
		line_number = yard.line_number;
		text = yard.text;
		left = yard.left;
		right = yard.right;
		unary = yard.unary;
		type = yard.type;
		postfix = yard.postfix;
	}

	bool IsUnary(lexem_type_t lexem)
	{
		switch (lexem)
		{
		case lt_namescope:
		case lt_inc:
		case lt_dec:
		case lt_new:
		case lt_unary_plus:
		case lt_unary_minus:
		case lt_logical_not:
		case lt_not:
			return true;
		default:
			return false;
		}
	}
} shunting_yard_t;

