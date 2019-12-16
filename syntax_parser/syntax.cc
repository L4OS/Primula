#include <stdio.h>
#include <string.h>
#include "../include/lexem.h"
#include "lexem_tree_root.h"

Code::statement_list_t  parse_file_stream(FILE * f)
{
	char					buffer[512];
	char					filename[512];
	int						line_number = 0;
	lexem_type_t			lexem_type;
	int						count;
	char				*	lexem_value_ptr;
	lexem_event_t			ev;					// Текущая лексема 
	static int				status;

	Code * code = CreateCodeSplitter();


	while (!feof(f))
	{
		if (fgets(buffer, sizeof(buffer), f) == NULL)
			break;
		if (buffer[0] == '@') {
			sscanf(buffer + 1, "%d\n", &line_number);
			continue;
		}
		if (buffer[0] == '$') {
			strncpy(filename, buffer + 1, sizeof(filename) - 1);
			continue;
		}
		if (buffer[0] == '/') {
			strncpy(filename, buffer + 5, sizeof(filename) - 1);
			continue;
		}
		lexem_value_ptr = strchr(buffer, ' ');
		if (lexem_value_ptr) {
			*lexem_value_ptr++ = 0;
			char * s = strchr(lexem_value_ptr, '\n');
			if (s) *s = 0;
		}
		count = sscanf(buffer, "%d\n", &lexem_type);

		ev.lexem_type = lexem_type;
#if true
		if (lexem_value_ptr != 0)
		{
			int sz = strlen(lexem_value_ptr) + 1;
			ev.lexem_value = new char[sz];
			strcpy((char*)ev.lexem_value, lexem_value_ptr);
		}
		else
#endif
		ev.lexem_value = lexem_value_ptr;
		ev.line_number = line_number;
		ev.file_name = filename;

#if TEST_TEST
		static int prev_num = 0;
		if (prev_num != line_number)
		{
			printf("Line %d\n", line_number);
			prev_num = line_number;
		}
#endif

		code->AddLexem(&ev);
	}

	Code::statement_list_t result = code->CodeThree();
	delete code;
	return result;
}

#include <string>

#include "namespace.h"
#include "../codegen/restore_source_t.h"
#include "../codegen/Pascal/pascal_generator.h"

#if true
restore_source_t			global_space;
#else
PascalGenerator             global_space;
#endif

extern void InitialNamespace(namespace_t * space);

int main(int argc, char * argv[])
{
	FILE * f;

	if (argc < 2) {
		puts("use case: syntax.exe filename.c.lexem");
		return 1;
	}

	f = fopen(argv[1], "rt");
	if (!f) {
		perror(argv[1]);
		return -1;
	}

	Code::statement_list_t result = parse_file_stream(f);

	fclose(f);

	fprintf(stderr, "------------- Syntax parsing started------------------\n");

	InitialNamespace(&global_space);
	global_space.Parse(result);

    fprintf(stderr, "------------- Syntax parsing complete ------------------\n");

	global_space.GenerateCode(&global_space);

	return 0;
}
