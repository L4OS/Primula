// Parser of #define
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "../include/defdef.h"

static def_pair_t	defintions[2048];
static int			def_count = 0;

//#define ITEM	defintions[def_count]

void create_definition()
{
	const static char	defdef[] =  "_WIN32";
	strcpy( defintions[def_count].loong_buffer, defdef );
	defintions[def_count].payload = 0;
	def_count++;

	strcpy(defintions[def_count].loong_buffer, (char*) "__STDC__");
	defintions[def_count].payload = 0;
	def_count++;

	strcpy(defintions[def_count].loong_buffer, (char*) "_M_IX86");
	defintions[def_count].payload = (char*) "600";
	def_count++;

	strcpy(defintions[def_count].loong_buffer, (char*) "_W64");
	defintions[def_count].payload = 0;
	def_count++;

}

// Добавляет #define 
// Нужно оптимизировать через хэши
int parse_definition( char * define_payload )
{
	size_t	deflen, arglen, valen;
	char	*	va;
	char	*	p;	// First delimiter in string
	char	*	b;	// First parenthesis in string
	char	*	e;	// Parenthesis after last argument

	if(def_count < ((sizeof(defintions)/sizeof(defintions[0]))-2) ) 
	{
		if( define_payload ) 
			while( isspace(*define_payload) ) 
				define_payload++;
// -------------------------------------
		arglen = 0;
		b = strchr( define_payload, '(' );
		p = strpbrk( define_payload, " \t" );
		e = 0;
		if(b) {
			if( (b&&p&&b<p) || (b&&!p) ) { //
//				printf("Found function substitution\n");
				*b++ = '\0';
				while( isspace(*b) ) *b++ = '\0';
				e = strchr( b, ')' );
				if(!e) return -948;
				*e++ = '\0';
				p = e[0] ? e : 0;
				arglen = strlen(b);
			}
			else b = 0;
		} 
// -------------------------------------
		if( p ) while( isspace(*p) ) *p++ = '\0';
		deflen = strlen(define_payload);
		if( arglen + deflen + 2 >= sizeof(defintions[def_count].loong_buffer) ) {
			printf("Not enough memory\n");
			return -942;
		}

		def_pair_t * def = check_for_preprocessor_define(define_payload);
		if( !def ) {
			def = &defintions[def_count];
			def_count++;
		} else
			printf("%s already defined\n",define_payload );

		strcpy( def->loong_buffer, define_payload );
		if( b ) 
		{
			e = def->loong_buffer + deflen + 1;	// reusing varianle e
			strcpy( e, b );
			def->args = e;
			deflen += arglen + 1;
		} else def->args = 0;

		if(p) 
		{
			// Check storage size, if it have enougnt space - use it, otherwise allocate memory
			valen = strlen(p);
			if( (sizeof(def->loong_buffer) - 2 - deflen) > valen ) {
				va = def->loong_buffer + deflen + 1;
			} else {
				va = (char*) malloc( valen );
				if(!va) {
					printf("Not enough memory\n");
					return -942;
				}
			}
			strcpy( va, p );
			def->payload = va;
		}

//		printf("#define %s %s\n", ITEM.loong_buffer, ITEM.payload );
	} else {
		printf("Not enough memory\n");
		return -942;
	}
	return 1;
}

// Where is check for preprocessors?: # #@ ##


// Substitutions of expressions in #define
int substitute_def_expression(def_pair_t * def, char * str) 
{
	char	args[16][64];
	int		argc = 0;
	int		i,j;
	bool	sc = false;
	int		bracket_counter = 0;


	// Split arguments to array
	for( i=j=0; str[i]; i++ ) {
		if( str[i] == '"' ) 
			sc = !sc;
		// 20131230 brackets workwaourb
		if( str[i] == '(') bracket_counter++;
		else if( str[i] == ')') bracket_counter--;

		if( bracket_counter || (sc || str[i] != ',') ) { 
				args[argc][j++] = str[i];
			continue;
		}
		if( !isspace(str[i]) ) {
			args[argc++][j] = 0;
			j=0;
		}
	}
	args[argc++][j] = 0;

	char	parm[16][16];
	int		cnt = 0;
	for( i=j=0; def->args[i]; i++ ) {
		if( isspace(def->args[i]) )
			continue;
		if( def->args[i] != ',' ) {
			parm[cnt][j++] = def->args[i];
			continue;
		}
		parm[cnt++][j] = 0;
		j = 0;
	}
	parm[cnt++][j] = 0;

	if(argc != cnt) 
		return -950;
	
	// Find words which will be replaced by arguments 
	j = 0;
	str[0] = 0;
	char	buff[2048];
	bool	stop = false;
	typedef enum { no, txt, chr, merg } state_t;

	state_t next_state = no;
	state_t prev_state = no;

	for(i=0; !stop; i++) {
		switch( def->payload[i] ) {
			case '\0':
				stop = true;
				goto entry;
			case '#':
				if(def->payload[i+1] == '#')
					next_state = merg;
				else if(def->payload[i+1] == '@')	
					next_state = chr;
				else							
					next_state = txt; 
			case '\t':
			case '(':
			case ')':
			case ',':
			case ' ':
entry:
				buff[j] = 0;
				// Пошло сравнение
				for(int k=0; k<cnt; k++) {
					if( 0==strcmp( buff, parm[k] ) ) {
						if( next_state==txt ) {
							buff[0] = '\"';
							strcpy( &buff[1], args[k] );
							strcat( &buff[1], "\"" );
							i++;
						} else if(next_state==chr) {
							buff[0] = '\"';
							buff[1] = args[k][0];
							buff[2] = '\"';
							buff[3] = '\0';
							i += 2;
						} else {
							strcpy( buff, args[k] );
							if( next_state==merg ) i+=2;
						}
						prev_state = next_state;
						next_state = no;
						j = (int) strlen(buff);
						break;
					}
				}
				if( next_state==no || next_state==merg )
					buff[j++] = def->payload[i];
				buff[j] = 0;
				strcat( str, buff);
				j = 0;
				break;
			default:
				buff[j++] = def->payload[i];
		}
	}

	return (int) strlen(str);;
}

int remove_definition( char * define_payload )
{
	def_pair_t * pair = check_for_preprocessor_define(define_payload);
	if(pair) {
		memset( pair, 0, sizeof(def_pair_t) );
	}
	return 1;
}

def_pair_t * check_for_preprocessor_define( char * name )
{
//if(strcmp(name,"__nonnull")==0)
//printf("Debug!\n");
	for( int i=0; i<sizeof(defintions)/sizeof(defintions[0]); i++  ) {
		if( strcmp(name, defintions[i].loong_buffer) == 0 ) {
			return &defintions[i];
		}
	}
	return 0;
}

