#include <stdio.h>
#include <string.h>
#include "../include/lexem.h"
#include "defdef.h"

extern int parse_input( flow_control_t	*	flow );
extern void create_hash(void);
extern void create_definition(void);
extern int restore_code(FILE * inp, FILE * of);

typedef struct {
	const char * const  		szKey;
	int							ID;
	const char * const 			szHelpString;
} typeKeys;

typeKeys cmdKeys[] = 
{
	{ "PARSE",	 om_lexem,		"- generate lexem from source code"},
	{ "REVERSE", om_test,		"- generate source code from lexem"},
	{ "DEBUG",   om_debug,		"- trace lexem state machine"},
	{ 0,  0,   "Nothing here" },
};


static void help_screen()
{
	fprintf(stderr, "Use case: syntax.exe [key] filename.c [outfile.lexem]\nFollowing keys are supported:\n");
	for (typeKeys * key = cmdKeys; key->szKey; key++) {
		fprintf(stderr, "\t-%s     \t\"%s\"\n", key->szKey, key->szHelpString);
	}
}

int main(int argc, char * argv[])
{
	int						error_code = 0;
	char					lexem_name[128];
	FILE					*f = NULL, *of = NULL;
	output_mode_t			mode = om_lexem;


	do {

		if(argc < 2) 
		{
			help_screen();
			break;
		}

		f = NULL;

		char	*	out_name;
		mode = om_lexem;
		int			numkeys = 0;
		for (int idx = --argc; idx; --idx)
		{
			if (*argv[idx] == '-') 
			{
				mode = om_no_such_key;
				for (typeKeys * key = cmdKeys; key->szKey; key++)
					if (!strcmp(&argv[idx][1], key->szKey)) 
					{
						mode = (output_mode_t)key->ID;
						numkeys++;
						break;
					}
				if (mode == om_no_such_key)
				{
					fprintf(stderr, "Command line key '%s' not supported\n", argv[idx]);
					help_screen();
					error_code = -80;
					break;
				}
			}
			else 
				if (f) 
				{ 
					perror("File already set");  
					break;
				}
				else 
					if (!(f = fopen(argv[idx], "rt"))) 
					{
						perror(argv[idx]);
						error_code = -1;
						break;
					}
					else 
						out_name = argv[idx];
		}

		if( error_code ) 
			break;

		of = NULL;

		{
			switch (mode) 
			{
			case om_debug:	
				of = stdout; 
				break;
			case om_test:
				strcpy(lexem_name, out_name);
				strcat(lexem_name, ".cc");
				break;
			case om_lexem:
				strcpy(lexem_name, out_name);
				strcat(lexem_name, ".lexem");
				break;
			default: 
				throw "Extended modes not supported";
			}
		}
	
		if (of != stdout)
			of = fopen(lexem_name, "wt"); 

		flow_control_t		flow;
		strncpy( flow.filename, out_name, sizeof(flow.filename) );
		flow.global_state = gs_expression;
		flow.line_number = 1;
		flow.f = f;
		flow.of = of;
		flow.mode = mode;

		create_hash();
		create_definition();

		fprintf( of, "// $ %s\n", flow.filename );
		if(mode == om_test)
			error_code = restore_code(f, of);
		else
			error_code = parse_input( &flow );

		if( error_code >=0 ) 
			fprintf(stderr, "Lexical analysis has been successfully completed.\n");
		else 
			fprintf(stderr, "Lexical analysis status %d. Process aborted!\n", error_code );
		fclose(of);
	} while(false);

	if(f) 
		fclose(f);

	return 0;
}
