#include <stdio.h>
#include <string.h>

#include "../include/lexem.h"
#include "defdef.h"

lexem_item_t		*	lexem_head = 0;
lexem_item_t		*	lexem_tail = 0;



void lexem_to_string(lexem_item_t * l, char * txt_buff, int buff_size)
{
	*(unsigned long *)txt_buff = 0;

	switch (l->type)
	{
	case lt_word:			snprintf(txt_buff, buff_size, "%s ", l->lexem);	break;
	case lt_integer:		
	case lt_floatnumber:	snprintf(txt_buff, buff_size, " %s", l->lexem);	break;
	case lt_string:			snprintf(txt_buff, buff_size, " \"%s\" ", l->lexem);	break;
	case lt_openbraket:		txt_buff[0] = '(';						break;
	case lt_comma:			txt_buff[0] = ',';						break;
	case lt_closebraket:	txt_buff[0] = ')';						break;
	case lt_openblock:		txt_buff[0] = '{';						break;
	case lt_closeblock:		txt_buff[0] = '}';						break;
	case lt_colon:			txt_buff[0] = ':';						break;
	case lt_semicolon:		txt_buff[0] = ';';						break;
	case lt_openindex:		txt_buff[0] = '[';						break;
	case lt_closeindex:		txt_buff[0] = ']';						break;
	case lt_logical_and:	snprintf(txt_buff, buff_size, "&&");					break;
	case lt_logical_or:		snprintf(txt_buff, buff_size, "||");					break;
	case lt_and:			txt_buff[0] = '&';						break;
	case lt_or:				txt_buff[0] = '|';						break;
	case lt_shift_right:	snprintf(txt_buff, buff_size, ">>");					break;
	case lt_more_eq:		snprintf(txt_buff, buff_size, ">=");					break;
	case lt_more:			txt_buff[0] = '>';						break;
	case lt_shift_left:		snprintf(txt_buff, buff_size, ">>");					break;
	case lt_less_eq:		snprintf(txt_buff, buff_size, "<=");					break;
	case lt_less:			txt_buff[0] = '<';						break;
	case lt_set:			txt_buff[0] = '=';						break;
	case lt_add:			txt_buff[0] = '+';						break;
	case lt_inc:			snprintf(txt_buff, buff_size, "++");					break;
	case lt_sub:			txt_buff[0] = '-';						break;
	case lt_dec:			snprintf(txt_buff, buff_size, "--");					break;
	case lt_mul:			txt_buff[0] = '*';						break;
	case lt_not_eq:			snprintf(txt_buff, buff_size, "!=");					break;
	case lt_logical_not:	txt_buff[0] = '!';						break;
	case lt_quest:			txt_buff[0] = '?';						break;
	case lt_point_to:		snprintf(txt_buff, buff_size, "->");					break;
	case lt_equally:		snprintf(txt_buff, buff_size, "==");					break;
	case lt_character:		snprintf(txt_buff, buff_size, "\'%s\'", l->lexem);		break;
	case lt_dot:			txt_buff[0] = '.';						break;
	case lt_div:			txt_buff[0] = '/';						break;
	case lt_rest:			txt_buff[0] = '%';						break;
	case lt_tilde:			txt_buff[0] = '~';						break;
	case lt_three_dots:		snprintf(txt_buff, buff_size, "...");					break;
	case lt_mul_and_set:	snprintf(txt_buff, buff_size, "*=");					break;
	case lt_add_and_set:	snprintf(txt_buff, buff_size, "+=");					break;
	case lt_sub_and_set:	snprintf(txt_buff, buff_size, "-=");					break;
	case lt_div_and_set:	snprintf(txt_buff, buff_size, "/=");					break;
	case lt_rest_and_set:	snprintf(txt_buff, buff_size, "%%=");					break;
	case lt_namescope:		snprintf(txt_buff, buff_size, "::");					break;
	case lt_return:			snprintf(txt_buff, buff_size, "return ");				break;
	case lt_do:				snprintf(txt_buff, buff_size, "do");					break;
	case lt_if:				snprintf(txt_buff, buff_size, "if");					break;
	case lt_else:			snprintf(txt_buff, buff_size, "else ");					break;
	case lt_break:			snprintf(txt_buff, buff_size, "break");					break;
	case lt_continue:		snprintf(txt_buff, buff_size, "continue");				break;
	case lt_while:			snprintf(txt_buff, buff_size, "while");					break;
	case lt_for:			snprintf(txt_buff, buff_size, "for");					break;
	case lt_goto:			snprintf(txt_buff, buff_size, "goto");					break;
	case lt_switch:			snprintf(txt_buff, buff_size, "switch");				break;
	case lt_case:			snprintf(txt_buff, buff_size, "case");					break;
	case lt_default:		snprintf(txt_buff, buff_size, "default");				break;
	case lt_throw:			snprintf(txt_buff, buff_size, "throw");					break;

	case lt_static:			snprintf(txt_buff, buff_size, "static ");				break;
	case lt_extern:			snprintf(txt_buff, buff_size, "extern ");				break;
	case lt_const:			snprintf(txt_buff, buff_size, "const ");				break;

	case lt_try:			snprintf(txt_buff, buff_size, "try ");					break;
	case lt_catch:			snprintf(txt_buff, buff_size, "catch ");				break;

	case lt_false:			snprintf(txt_buff, buff_size, "false");					break;
	case lt_true:			snprintf(txt_buff, buff_size, "true");					break;

	case lt_lex_bool:		snprintf(txt_buff, buff_size, "bool ");					break;
	case lt_lex_char:		snprintf(txt_buff, buff_size, "char ");					break;
	case lt_lex_double:		snprintf(txt_buff, buff_size, "double ");				break;
	case lt_lex_float:		snprintf(txt_buff, buff_size, "float ");				break;
	case lt_lex_short:		snprintf(txt_buff, buff_size, "short ");				break;
	case lt_lex_int:		snprintf(txt_buff, buff_size, "int ");					break;
	case lt_lex_long:		snprintf(txt_buff, buff_size, "long ");					break;
	case lt_lex_signed:		snprintf(txt_buff, buff_size, "signed ");				break;
	case lt_lex_unsigned:	snprintf(txt_buff, buff_size, "unsigned ");				break;

	case lt_type_bool:		snprintf(txt_buff, buff_size, "bool ");					break;
	case lt_type_char:		snprintf(txt_buff, buff_size, "char ");					break;
	case lt_type_uchar:		snprintf(txt_buff, buff_size, "unsgned char ");			break;
	case lt_type_short:		snprintf(txt_buff, buff_size, "short ");				break;
	case lt_type_ushort:	snprintf(txt_buff, buff_size, "unsigned short ");		break;
	case lt_type_int:		snprintf(txt_buff, buff_size, "int ");					break;
	case lt_type_uint:		snprintf(txt_buff, buff_size, "unsigned int ");			break;
	case lt_type_long:		snprintf(txt_buff, buff_size, "long ");					break;
	case lt_type_ulong:		snprintf(txt_buff, buff_size, "unsigned long ");		break;
	case lt_type_llong:		snprintf(txt_buff, buff_size, "long long ");			break;
	case lt_type_ullong:	snprintf(txt_buff, buff_size, "unsigned long long ");	break;
	case lt_type_double:	snprintf(txt_buff, buff_size, "double ");				break;
	case lt_type_float:		snprintf(txt_buff, buff_size, "float ");				break;
	case lt_wchar_t:		snprintf(txt_buff, buff_size, "wchar_t ");				break;
	case lt_sizeof:			snprintf(txt_buff, buff_size, "sizeof");				break;

	case lt_void:		strncpy(txt_buff, "void ", buff_size); break;
	case lt_enum:		strncpy(txt_buff, "enum ", buff_size); break;
	case lt_union:		strncpy(txt_buff, "union ", buff_size); break;
	case lt_struct:		strncpy(txt_buff, "struct ", buff_size); break;
	case lt_typedef:	strncpy(txt_buff, "typedef ", buff_size); break;
	case lt_register:	strncpy(txt_buff, "register ", buff_size); break;

	default:
		throw "Type description failed";
	}

}

int restore_code(FILE * inp, FILE * of)
{
	int			        status = -1;
	char		    *	eof;
	lexem_type_t		id;
	lexem_item_t		l;
	char			    buffer[1024];
	char			    arg[1024];
	int			        tabs = 0;
	bool			    crlf = false;
	int			        prev_line = 1;
	int			        line_number = 0;
	while (true)
	{
		eof = fgets(buffer, sizeof(buffer) - 1, inp);
		if (!eof) 
            break; 
		if (buffer[0] == '/')
		{
			fputs(buffer, of);
			prev_line++;
		}
		else if (buffer[0] == '@')
		{
			sscanf(&buffer[1], "%u", &line_number);
			fprintf(of, "\n");
			crlf = true;
			prev_line = line_number;
		}
		else
		{
			status = sscanf(buffer, "%d", (int*) &id);
			char * value = strchr(buffer, ' ');
			if(value)
			{
				value++;
				char * term;
				term = strchr(value, '\r');
				if (term)
					*term = '\0';
				term = strchr(value, '\n');
				if (term)
					*term = '\0';
				strncpy(arg, value, sizeof(arg));
			}

			l.type = id;
			l.lexem = arg;

			try 
			{
				lexem_to_string(&l, buffer, sizeof(buffer));
			}
			catch (const char * err)
			{
				fprintf(stderr, "Line %d: Exception: %s\n", line_number, err);
			}

			if (id == lt_closeblock)
				tabs--;

			if (crlf)
			{
				for (int i = 0; i < tabs; i++) 
                    fputs("  ", of);
				crlf = false;
			}
			fprintf(of, "%s", buffer);

			if (id == lt_openblock)
				tabs++;

		}
	}
	fputc('\n', of);
	return status;
}
