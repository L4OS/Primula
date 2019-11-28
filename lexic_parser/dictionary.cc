#include "../include/lexem.h"

typedef enum 
{
	v_c,
	v_cp,
	v_cpp
} version_t;

typedef struct	
{
	char const 		*	name;
	lexem_type_t		identify;
	version_t			plusplus;
} reserved_word_t;

/////////////////////////////////////////////////////////////////////////////
//
// All reserved word of C++
//
const reserved_word_t	operators[] = {
	{	"asm",					lt_asm,					v_c		},
	{	"auto",					lt_auto,				v_cpp	},
	{	"bool",					lt_lex_bool,			v_cp	},
	{	"break",				lt_break,				v_c		},
	{	"case",					lt_case,				v_c		},
	{	"catch",				lt_catch,				v_cpp	},
	{	"char",					lt_lex_char,			v_c		},
	{	"class",				lt_class,				v_cp	},
	{	"const",				lt_const,				v_c		},
	{	"const_cast",			lt_const_cast,			v_cpp	},
	{	"continue",				lt_continue,			v_c		},
	{	"default",				lt_default,				v_c		},
	{	"delete",				lt_delete,				v_cp	},
	{	"do",					lt_do,					v_c		},
	{	"double",				lt_lex_double,			v_c		},
	{	"dynamic_cast",			lt_dynamic_cast,		v_cpp	},
	{	"else",					lt_else,				v_c		},
	{	"enum",					lt_enum,				v_c		},
	{	"explicit",				lt_explicit,			v_cpp	},
	{	"export",				lt_export,				v_c		},
	{	"extern",				lt_extern,				v_c		},
	{	"false",				lt_false,				v_cp	},
	{	"float",				lt_lex_float,			v_c		},
	{	"for",					lt_for,					v_c		},
	{	"friend",				lt_friend,				v_cpp	},
	{	"goto",					lt_goto,				v_c		},
	{	"if",					lt_if,					v_c		},
	{	"inline",				lt_inline,				v_cp	},
	{	"int",					lt_lex_int,				v_c		},
	{	"long",					lt_lex_long,			v_c		},
	{	"mutable",				lt_mutable,				v_cpp	},
	{	"namespace",			lt_namespace,			v_cpp	},
	{	"new",					lt_new,					v_cp	},
	{	"operator",				lt_operator,			v_cpp	},
	{	"private",				lt_private,				v_cp	},
	{	"protected",			lt_protected,			v_cp	},
	{	"public",				lt_public,				v_cp	},
	{	"register",				lt_register,			v_c		},
	{	"reinterpret_cast",		lt_reinterpret_cast,	v_cpp	},
	{	"return",				lt_return,				v_c		},
	{	"short",				lt_lex_short,			v_c		},
	{	"signed",				lt_lex_signed,			v_c		},
	{	"sizeof",				lt_sizeof,				v_c		},
	{	"static",				lt_static,				v_c		},
	{	"static_cast",			lt_static_cast,			v_cpp	},
	{	"struct",				lt_struct,				v_c		},
	{	"switch",				lt_switch,				v_c		},
	{	"template",				lt_template,			v_cpp	},
	{	"this",					lt_this,				v_cp	},
	{	"throw",				lt_throw,				v_cpp	},
	{	"true",					lt_true,				v_cp	},
	{	"try",					lt_try,					v_cpp	},
	{	"typedef",				lt_typedef,				v_c		},
	{	"typeid",				lt_typeid,				v_cpp	},
	{	"typename",				lt_typename,			v_cpp	},
	{	"union",				lt_union,				v_c		},
	{	"unsigned",				lt_lex_unsigned,		v_c		},
	{	"using",				lt_using,				v_cpp	},
	{	"virtual",				lt_virtual,				v_cp	},
	{	"void",					lt_void,				v_c		},
	{	"volatile",				lt_volatile,			v_c		},
	{	"wchar_t",				lt_wchar_t,				v_cpp	},
	{	"while",				lt_while,				v_c		}
};

#define WORDS_COUNT		sizeof(operators)/sizeof(operators[0])
#define HASH_SIZE		((WORDS_COUNT<<1)-1)

/////////////////////////////////////////////////////////////////////////////
// Hash of all reserved word
reserved_word_t	const	*	words_hash[HASH_SIZE];

unsigned int get_hash_value( const char * w)
{
	unsigned int	seed = 339;
	unsigned int	hash = 0;

	for( int i=0; w[i]; i++ ) 
	{
		unsigned int c = w[i] - 'a';
		hash = hash * seed + c + i;
	}
	return hash % HASH_SIZE;
}

// Avoid external includes
static int strcmp(const char *w1, const char *w2)
{
	int res;
	while (true)
	{
		register int result = *w1 - *w2;
		if (result || *w1 == 0)
			return result;
		++w1;
		++w2;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Returns pointer to reserved word descriptor by its lexeme
static reserved_word_t const * find_reserved_word( int id ) 
{
	reserved_word_t const * result = 0;
	if( id >= 1000 && id <= 1062) 
		result = &operators[id - 1000];
	return result;
}

reserved_word_t const * find_reserved_word(char const * w) 
{
	reserved_word_t const * result = 0;
	unsigned int	hash;
	hash = get_hash_value(w);
	for (int i = 0; i < 10; i++)
	{
		if (words_hash[hash] == 0)
			break;
		if (strcmp(words_hash[hash]->name, w) == 0)
		{
			result = words_hash[hash];
			break;
		}
		hash++;
		hash %= HASH_SIZE;
	}
	return result;
}

void create_hash(void) 
{
	int collision = 0;
	unsigned int	hash;
	for (int i = 0; i < sizeof(words_hash) / sizeof(words_hash[0]); i++)
		words_hash[i] = 0;
	for(int i=0; i<WORDS_COUNT; i++) 
	{
		char const *	w = operators[i].name;
		hash = get_hash_value( w );
		if( ! words_hash[hash] ) 
		{
			words_hash[hash] = &operators[i];
			collision++;
		} 
		else for( int j=0; j<10; j++ ) 
		{
			hash++;
			hash %= HASH_SIZE;
			if( words_hash[hash] ) 	
			{	
				collision++; 
				continue; 
			}
			if(j >= 10) 
				throw "No hash space available";
			words_hash[hash] = &operators[i];
			break;
		}
	}
}

lexem_type_t translate_lexem( char const * w )
{
	reserved_word_t const * prw = find_reserved_word(w);
	if( ! prw ) 
		return lt_empty;
	return prw->identify;
}

char const * get_word_by_lexem( int identify )
{
	reserved_word_t const * prw = find_reserved_word( identify );
	if( ! prw ) 
		return 0;
	return prw->name;
}
