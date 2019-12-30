#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

// Local project files
#include "../include/lexem.h"
#include "defdef.h"

#pragma region Config area
int		autoname = 0;	// Generate automatical name for unnamed structures. Not need at all
#pragma endregion


extern int substitute_def_expression(def_pair_t * def, char * str);
extern void lexem_to_string(lexem_item_t * l, char * txt_buff, int buff_size);

typedef enum {
	nt_decimal,
	nt_hexdecimal,
	nt_float,
} mumber_type_t;

static char				preprocessor_buffer[8192];
static int				preprocessor_counter;
static char				current_lexem[512];
static int				current_lexem_size;
static mumber_type_t	number_type = nt_decimal;
static def_pair_t	*	def = NULL;
static int				def_indirection = 0;

static FILE				*	of = NULL;
static output_mode_t		mode = om_test; // om_lexem; // om_debug; // 
static int					st_line_number = 1;
//static int					debug_line = 1;


int parse_input(flow_control_t	*	flow);


extern lexem_type_t translate_lexem(char const * w);
extern void create_hash(void);
extern int parse_preprocessor_line(flow_control_t	* flow, char * line);
extern int substitute_def_expression(def_pair_t * def, char * str);

static int count = 0;

static int global_state_machine(unsigned char ch, flow_control_t	*	flow);
static char * generate_typename(lexem_type_t type);
lexem_type_t translate_builtin_types();
void append_lexem(lexem_type_t type);

int parse_input(flow_control_t	*	flow)
{
    int error_code;
    int marker = true;

    ::of = flow->of;
    ::mode = flow->mode;

    while (true)
    {
        char ch = (char)fgetc(flow->f);
        if (feof(flow->f))
        {
            error_code = global_state_machine(0x0a, flow);
            fclose(flow->f);
            break;
        }
        if (marker && !isascii(ch))
            continue;
        marker = false;
        if (ch == 0xa)
        {
            if (flow->line_number == 2867)
                printf("Debug here\n");
            if (flow->line_number == 2909)
                printf("Debug here\n");
        }
        try
        {
            error_code = global_state_machine(ch, flow);
            if (0 > error_code)
                break;
        }
        catch (char * s)
        {
            fprintf(stderr, "Internal parser error: %s\n", s);
            error_code = -1;
            break;
        }
    }
    if (0 > error_code)
        fprintf(stderr, "Stop on error %d on %s at line %d\n", error_code, flow->filename, flow->line_number);
    else
        fprintf(stderr, "Preprocessor pass %d lines of '%s'\n", flow->line_number, flow->filename);
    return error_code;
}

static lexem_type_t		lex_queue[9];
static int				lex_counter = 0;

extern bool IsInativeCode();

static int global_state_machine(unsigned char ch, flow_control_t	*	flow)
{
	st_line_number = flow->line_number;
	//if(flow->line_number == 636 && strstr(flow->filename, "stdlib.h") )
	//printf("line_number == \n");
	switch (flow->global_state) 
    {
	case gs_expression:
		if (ch == '/') 
        {
			flow->global_state = gs_parse_slash; 
            return 0;
		}
		if (ch == 0x0a) 
        {
			flow->line_number++; 
            return 0;
		}
		if (ch == '#') 
        {
			flow->global_state = gs_do_preprocessor;
			preprocessor_counter = 0;
			return 0;
		}
		if ( /*isspace(ch)*/ ch == ' ' || ch == '\t') 
            return 0;
		if (IsInativeCode())
			return 0;

	case gs_lexem:
	reparse:
		if (isalpha(ch) || ch == '_') 
        {
			current_lexem_size = 0;
			current_lexem[current_lexem_size++] = ch;
			flow->global_state = gs_inside_lexem; // gs_lexem;
			return 0;
		}
		if (isdigit(ch)) 
        {
			number_type = nt_decimal;
			current_lexem[current_lexem_size++] = ch;
			flow->global_state = gs_numeric;
			return 0;
		}

	case gs_inside_lexem:
		if (isalnum(ch) || ch == '_') 
        {
			current_lexem[current_lexem_size++] = ch;
			return 0;
		}
		current_lexem[current_lexem_size] = '\0';
		if (current_lexem_size) 
        {
			//if( def != NULL )
			//	throw "Expression too complicated";
			// Look for #define
			def = check_for_preprocessor_define(current_lexem);
			if (def) 
            {
				// #define can have arguments
				if (def->args) 
                {
					goto enter_def_expression;
				}
				flow->global_state = gs_lexem;
				char * ptr = def->payload;
				if (ptr) 
                {
					current_lexem_size = 0;
					// printf("-- Parse define '%s' as '%s'\n", def->loong_buffer, ptr );
				    // recursion here
					while (*ptr) 
                    {
						global_state_machine(*ptr++, flow);
					}
				}
				else 
                {
					current_lexem_size = 0;
					// printf("Skip empty defininition '%s'\n", def->loong_buffer );
				}
				current_lexem[current_lexem_size] = 0;
				static int recursion = 0;
				recursion++;
				if (recursion >= 50) 
                {
					printf("Recursion in define too deep\n");
					// Not so good trick to catch something like #define FOO FOO
					append_lexem(lt_word);
					current_lexem_size = 0;
				}
				global_state_machine(ch, flow);
				recursion--;
				return 0;
			}
			else 
            {
				//if(flow->line_number == 339)
				//printf("line_number == 339\n");
				append_lexem(lt_word);
			}
		}
		flow->global_state = gs_delimiter;


	case gs_delimiter:
		if (isspace(ch)) 
        {
			if (ch == 0x0a) 
            {
				flow->line_number++;
				flow->global_state = gs_expression;
			}
			return 0;
		}
		flow->global_state = gs_lexem;
		if (isalpha(ch) || ch == '_') 
            goto reparse;
		if (ch == '{') 
        { 
            append_lexem(lt_openblock);	
            return 0; 
        }
		if (ch == '}') 
        { 
            append_lexem(lt_closeblock);	
            return 0; 
        }
		if (ch == '(') 
        { 
            append_lexem(lt_openbraket);	
            return 0; 
        }
		if (ch == ')') 
        { 
            append_lexem(lt_closebraket);	
            return 0; 
        }
		if (ch == ';') 
        { 
            append_lexem(lt_semicolon);	
            return 0; 
        }
		if (ch == '[') 
        { 
            append_lexem(lt_openindex);	
            return 0; 
        }
		if (ch == ']') 
        { 
            append_lexem(lt_closeindex);	
            return 0; 
        }
		if (ch == ',') 
        { 
            append_lexem(lt_comma);		
            return 0; 
        }
		if (ch == '?') 
        { 
            append_lexem(lt_quest);		
            return 0; 
        }
		if (ch == '~') 
        { 
            append_lexem(lt_tilde);		
            return 0; 
        }
		if (isdigit(ch)) 
        {
			number_type = nt_decimal;
			current_lexem[current_lexem_size++] = ch;
			flow->global_state = gs_numeric;
			return 0;
		}
		if (ch == ':') 
        { 
            flow->global_state = gs_colon;			
            return 0; 
        }
		if (ch == '*') 
        { 
            flow->global_state = gs_mul;			
            return 0; 
        }
		if (ch == '=') 
        { 
            flow->global_state = gs_equally;		
            return 0; 
        }
		if (ch == '+') 
        { 
            flow->global_state = gs_plus;			
            return 0; 
        }
		if (ch == '-') 
        { 
            flow->global_state = gs_minus;			
            return 0; 
        }
		if (ch == '"') 
        { 
            flow->global_state = gs_string;			
            return 0; 
        }
		if (ch == '&') 
        { 
            flow->global_state = gs_and;			
            return 0; 
        }
		if (ch == '^') 
        { 
            flow->global_state = gs_xor;			
            return 0; 
        }
		if (ch == '|') 
        { 
            flow->global_state = gs_or;				
            return 0; 
        }
		if (ch == '>') 
        { 
            flow->global_state = gs_more;			
            return 0; 
        }
		if (ch == '<') 
        { 
            flow->global_state = gs_less;			
            return 0; 
        }
		if (ch == '/') 
        { 
            flow->global_state = gs_parse_slash; 	
            return 0; 
        }
		if (ch == '%') 
        { 
            flow->global_state = gs_parse_rest; 	
            return 0; 
        }
		if (ch == '!') 
        { 
            flow->global_state = gs_exclamation; 	
            return 0; 
        }
		if (ch == '\'') 
        { 
            flow->global_state = gs_apost_begin; 	
            return 0; 
        }
		if (ch == '.') 
        { 
            flow->global_state = gs_dot;			
            return 0; 
        }
		if (ch == '@') 
        { 
            flow->global_state = gs_linear_comment; 
            return 0; 
        }
		fprintf(stderr, " So we get unparsed character - %c on delimiter state", ch);
		throw "Got an unparsed character";
		break;

	case gs_dot:
		if (ch == '.') 
        {
			flow->global_state = gs_two_dots;
			return 0;
		}
		if (ch == '*') 
        {
			append_lexem(lt_direct_pointer_to_member);
			flow->global_state = gs_lexem;
			return 0;
		}
		append_lexem(lt_dot);
		flow->global_state = gs_lexem;
		goto reparse;

	case gs_two_dots:
		if (ch == '.') 
        {
			append_lexem(lt_three_dots);
			flow->global_state = gs_lexem;
			return 0;
		}
		return -9;

	case gs_apost_begin:
		if (ch == '\'' && !current_lexem_size) 
            return -7; // empty character constant			
		if (ch == '\\' && !current_lexem_size) 
        {
			current_lexem[current_lexem_size++] = ch;
			return 0;
		}
		if (current_lexem[0] == '\\') 
        {
			current_lexem[1] = ch;
			current_lexem_size = 2;
		}
		else 
        {
			current_lexem[0] = ch;
			current_lexem_size = 1;
		}
		flow->global_state = gs_apost_end;
		return 0;

	case gs_apost_end:
		if (ch != '\'') 
            return -8; // too many characters in constant
		append_lexem(lt_character);
		flow->global_state = gs_lexem;
		return 0;

	case gs_string:
		if (ch != '"') 
        {
			if (ch == '\\') flow->global_state = gs_screening;
			current_lexem[current_lexem_size++] = ch;
			return 0;
		}
		current_lexem[current_lexem_size] = '\0';
#if 0
		append_lexem(lt_string);
		flow->global_state = gs_lexem;
#else
		//append_lexem_queue(lt_string);
		flow->global_state = gs_try_concatenate_strings;
#endif
		return 0;

	case gs_screening:
		current_lexem[current_lexem_size++] = ch;
		flow->global_state = gs_string;
		return 0;

	case gs_equally:
		flow->global_state = gs_lexem;
		if (ch != '=') 
        { 
            append_lexem(lt_set);	
            goto reparse; 
        }
		append_lexem(lt_equally);
		return 0;

	case gs_mul:
		flow->global_state = gs_lexem;
		if (ch != '=') 
        { 
            append_lexem(lt_mul);		
            goto reparse; 
        }
		append_lexem(lt_mul_and_set);
		return 0;

	case gs_colon:
		flow->global_state = gs_lexem;
		if (ch != ':') 
        { 
            append_lexem(lt_colon);		
            goto reparse; 
        }
		append_lexem(lt_namescope);
		return 0;

	case gs_exclamation:
		flow->global_state = gs_lexem;
		if (ch != '=') 
        { 
            append_lexem(lt_logical_not);		
            goto reparse; 
        }
		append_lexem(lt_not_eq);
		return 0;

	case gs_plus:
		flow->global_state = gs_lexem;
		switch (ch) 
        {
		case '+': 
            append_lexem(lt_inc); 
            break;
		case '=': 
            append_lexem(lt_add_and_set); 
            break;
		default:  
            append_lexem(lt_add); 
            goto reparse;
		}
		return 0;
	case gs_minus:
		flow->global_state = gs_lexem;
		switch (ch) 
        {
		case '-': 
            append_lexem(lt_dec); 
            break;
		case '=': 
            append_lexem(lt_sub_and_set); 
            break;
		case '>': 
            flow->global_state = gs_check_ptr_to_member; 
            break;
		default:  
            append_lexem(lt_sub); 
            goto reparse;
		}
		return 0;
	case gs_check_ptr_to_member:
		flow->global_state = gs_lexem;
		if (ch == '*')
		{
			append_lexem(lt_indirect_pointer_to_member);
			return 0;
		}
		append_lexem(lt_point_to);
		goto reparse;

	case gs_more:
		flow->global_state = gs_lexem;
		switch (ch) 
        {
		case '>':	
            append_lexem(lt_shift_right); 
            break;
		case '=':	
            append_lexem(lt_more_eq);		
            break;
		default:	
            append_lexem(lt_more);		
            goto reparse;
		}
		return 0;

	case gs_less:
		flow->global_state = gs_lexem;
		switch (ch) 
        {
		case '<':	
            append_lexem(lt_shift_left);	
            break;
		case '=':	
            append_lexem(lt_less_eq);		
            break;
		default:	
            append_lexem(lt_less);		
            goto reparse;
		}
		return 0;

	case gs_and:
		flow->global_state = gs_lexem;
		if (ch == '&') 
        {
			append_lexem(lt_logical_and);
			return 0;
		}
		if (ch == '=') 
        {
			append_lexem(lt_and_and_set);
			return 0;
		}
		append_lexem(lt_and);
		goto reparse;

	case gs_xor:
		flow->global_state = gs_lexem;
		if (ch == '=') 
        {
			append_lexem(lt_xor_and_set);
			return 0;
		}
		append_lexem(lt_xor);
		goto reparse;

	case gs_or:
		flow->global_state = gs_lexem;
		if (ch == '|') 
        {
			append_lexem(lt_logical_or);
			return 0;
		}
		if (ch == '=') 
        {
			append_lexem(lt_or_and_set);
			return 0;
		}
		append_lexem(lt_or);
		goto reparse;

	case gs_parse_rest:
		if (ch == '=') 
        {
			flow->global_state = gs_lexem;
			append_lexem(lt_rest_and_set);
			return 0;
		}
		append_lexem(lt_rest);
		goto reparse;

	case gs_numeric:
	{
		static int U_counter = 0;
		static int L_counter = 0;
		if (isdigit(ch)) 
        {
			current_lexem[current_lexem_size++] = ch;
			return 0;
		}
		if (ch == 'x' && current_lexem_size == 1) 
        {
			current_lexem[current_lexem_size++] = ch;
			number_type = nt_hexdecimal;
			return 0;
		}
		if (ch == '.' && number_type == nt_decimal) 
        {
			current_lexem[current_lexem_size++] = ch;
            number_type = nt_float;
			return 0;
		}
		if (number_type == nt_hexdecimal &&
			(ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) 
        {
			current_lexem[current_lexem_size++] = ch;
			return 0;
		}
		if (toupper(ch) == 'U')
		{
			if (U_counter != 0)
				throw "Unsigned suffix allowed only once";
			current_lexem[current_lexem_size++] = ch;
			++U_counter;
			return 0;
		}
		if (toupper(ch) == 'L')
		{
			if (L_counter >= 2)
				throw "The long suffux is too long";
			current_lexem[current_lexem_size++] = ch;
			L_counter++;
			return 0;
		}
#ifdef CPP14
		if (ch == '\'')
		{
			// Optional single quotes(') may be inserted between the digits as a separator. 
			// They are ignored by the compiler.
			return 0;
		}
#endif
		flow->global_state = gs_lexem;
		current_lexem[current_lexem_size++] = 0;
		U_counter = L_counter = 0;

		switch (number_type)
		{
		case nt_decimal:		
            append_lexem(lt_integer);	
            break;
		case nt_hexdecimal:		
            append_lexem(lt_integer);	
            break;
		case nt_float:			
            append_lexem(lt_floatnumber);	
            break;
		}
	}
	goto reparse;

	case gs_parse_slash:
		switch (ch)
		{
		case '=':
			append_lexem(lt_div_and_set);
			flow->global_state = gs_lexem;
			return 0;
		case '/':
			flow->global_state = gs_linear_comment;
			return 0;
		case '*':
			flow->global_state = gs_inside_comment;
			return 0;
		default:	
            append_lexem(lt_div);
		}
		flow->global_state = gs_lexem;
		goto reparse;

	case gs_inside_comment:
		if (ch == 0x0a)
			flow->line_number++;
		else if (ch == '*')
			flow->global_state = gs_checking_comment;
		break;

	case gs_checking_comment:
		switch (ch) 
        {
		case '/':	
            flow->global_state = gs_lexem;	
            break;
		case 0x0a:	
            flow->line_number++; // Здесь break ни в коем случе не нужен
		case '*':	
            break;
		default:	
            flow->global_state = gs_inside_comment;
		}
		break;

	case gs_linear_comment:
		if (ch == 0x0d) 
        {
			flow->global_state = gs_caret_feed; 
            return 0;
		}
		if (ch == 0x0a) 
        {
			flow->line_number++; 
            flow->global_state = gs_expression; 
            return 0;
		}
		break;

	case gs_caret_feed:
		if (ch == 0x0a) 
        {
			flow->line_number++; 
            flow->global_state = gs_expression; 
            return 0;
		}
		printf("something going on end of line %d!!!\n", flow->line_number);
		break;

	do_preprocessor:
		flow->global_state = gs_do_preprocessor;
	case gs_do_preprocessor:
		if (ch == '/') 
        {
			flow->global_state = gs_is_preprocessor_comment;
			break;
		}
		if (ch == '\\') 
        {
			flow->global_state = gs_do_screen_preprocessor;
			break;
		}
		if (ch == 0x0a) 
        {
			preprocessor_buffer[preprocessor_counter] = '\0';
			int r = parse_preprocessor_line(flow, preprocessor_buffer);
			preprocessor_counter = 0;
			if (r > 0)
				flow->global_state = gs_expression;
			else if (r == 0)
				flow->global_state = gs_skip_preprocessor;
			else
				return r;
			flow->line_number++;
			break;
		}
		if (preprocessor_counter < sizeof(preprocessor_buffer))
			preprocessor_buffer[preprocessor_counter++] = ch;
		else
			throw "Not enough buffer space";
		break;

	case gs_is_preprocessor_comment:
		if (ch == '/') 
        {
			flow->global_state = gs_prep_line_comment;
		}
        else
        {
            if (ch == '*')
            {
                flow->global_state = gs_prep_block_comment;
            }
            else
            {
                preprocessor_buffer[preprocessor_counter++] = '/';
                goto do_preprocessor;
            }
        }
		break;

	case gs_prep_line_comment:
		if (ch == 0x0a) 
        {
			goto  do_preprocessor;
		}
		break;

	case gs_prep_block_comment:
		if (ch == 0x0a)
			flow->line_number++;
		else if (ch == '*')
			flow->global_state = gs_prep_check_comment;
		break;

	case gs_prep_check_comment:
		switch (ch) 
        {
		case '/':	
            flow->global_state = gs_do_preprocessor;	
            break;
		case 0x0a:	
            flow->line_number++; // Здесь break ни в коем случе не нужен
		case '*':	
            break;
		default:	
            flow->global_state = gs_prep_block_comment;
		}
		break;

	case gs_do_screen_preprocessor:
		if (ch == 0xd) 
        {
			// Do nothing - just skip
			break;
		}
		else if (ch == 0xa) 
        {
			flow->line_number++;
		}
		else 
        {
			preprocessor_buffer[preprocessor_counter++] = '\\';
			preprocessor_buffer[preprocessor_counter++] = ch;
		}
		flow->global_state = gs_do_preprocessor;
		break;

	case gs_skip_preprocessor:
		// Keep parser consistent on # inside comments
	{
		static enum {
			normal_skip, is_comment, line_comment, block_comment, term_comment
		}  status_quo = normal_skip;

		if (ch == 0x0a) 
            flow->line_number++;
		if (status_quo == normal_skip) 
        {
			if (ch == '#') 
            {
				flow->global_state = gs_do_preprocessor;
				break;
			}
			if (ch == '/') 
                status_quo = is_comment;
			break;
		}
		if (status_quo == is_comment) 
        {
			switch (ch) 
            {
			case '/':	
                status_quo = line_comment;	
                break;
			case '*':	
                status_quo = block_comment;	
                break;
			default:	
                status_quo = normal_skip;	
                break;
			}
			break;
		}
		if (status_quo == line_comment) 
        {
			if (ch == 0x0a) 
                status_quo = normal_skip;
			break;
		}
		if (status_quo == block_comment) 
        {
			if (ch == '*') 
                status_quo = term_comment;
			break;
		}
		if (status_quo == term_comment) 
        {
			if (ch == '/') 
                status_quo = normal_skip;
			else 
                status_quo = block_comment;
		}
	}
	break;

enter_def_expression:
	flow->global_state = gs_enter_def_expression;
	case gs_enter_def_expression:
		if (isspace(ch)) 
            return 0;
		if (ch != '(')
			return -10;
		flow->global_state = gs_parse_def_expression;
		current_lexem_size = 0;
		break;

	case gs_parse_def_expression:

		//if( flow->line_number== 28 && strstr(flow->filename, "sigset.h"))
		//printf("\\\\\\ %s %d\n", flow->filename, flow->line_number );

		if (ch == '(') 
            ++def_indirection;

		if (ch != ')') 
        {
			current_lexem[current_lexem_size++] = ch;
			return 0;
		}
		//if(strstr(current_lexem, "_DeclSpec") != 0) {
		//	printf("Catch something\n");
		//}
		if (def_indirection) 
        {
			--def_indirection;
			current_lexem[current_lexem_size++] = ch;
			return 0;
		}
		current_lexem[current_lexem_size] = '\0';
		if (def->payload) 
        {
			// 
			int		size;
			char	ibuff[512];
			strcpy(ibuff, current_lexem);
			// Added 20131229 for calculating preprocessor expressions
			current_lexem_size = 0;
			current_lexem[current_lexem_size] = '\0';
			//if( flow->line_number== 202 && strstr(flow->filename, "io.h"))
			//printf("\\\\\\ %s %d\n", flow->filename, flow->line_number );
			size = substitute_def_expression(def, ibuff);
			if (size < 0) 
                return size;

			flow->global_state = gs_lexem;
			char * ptr = ibuff;
			while (*ptr) 
            {
				size = global_state_machine(*ptr++, flow);
				if (size < 0)
					return size;
			}
		}
		else 
        {
			//printf("SKIP #define %s(%s) <= %s\n", 
			//	def->loong_buffer, 
			//	def->args, 
			//	current_lexem );
			current_lexem_size = 0;
			current_lexem[current_lexem_size] = '\0';
		}
		def = 0;
		flow->global_state = gs_lexem;
		return 0;

	case gs_try_concatenate_strings:
		if (isspace(ch))
			return 0;
		//	throw "queue synchronization error";
		if (ch != '"')
		{
			append_lexem(lt_string);
			flow->global_state = gs_lexem;
			goto reparse;
		}
		else
		{
			// concatenate strings
			flow->global_state = gs_string;
		}
		break;

	default:
		printf("global state machine unparsed state: %d!!!\n", flow->global_state);
		return -1; // Compilator internal error
	}

	return 0;
}

static char * generate_typename(lexem_type_t type)
{
    static int i_struct = 0;
    static int i_class = 0;
    static int i_enum = 0;
    static int i_union = 0;
    static char name[40];

    switch (type)
    {
    case lt_struct:
        snprintf(name, 40, "__stru_%03d", ++i_struct);
        break;
    case lt_class:
        snprintf(name, 40, "__clas_%03d", ++i_class);
        break;
    case lt_enum:
        snprintf(name, 40, "__enu_%03d", ++i_enum);
        break;
    case lt_union:
        snprintf(name, 40, "__uni_%03d", ++i_union);
        break;
    default:
        throw "This type cannot be unnamed";
    }
    return name;
}

lexem_type_t translate_builtin_types()
{
    int		check_error;
    int		c_signed = 0;
    int		c_unsigned = 0;
    int		c_int = 0;
    int		c_long = 0;
    int		c_short = 0;
    int		c_char = 0;
    int		c_float = 0;
    int		c_double = 0;

    for (int k = 0; k < lex_counter; k++)
        switch (lex_queue[k])
        {
        case lt_lex_bool:
            return (lex_counter == 1) ? lt_type_bool : (lexem_type_t)-2000;
        case lt_lex_char:
            c_char++;
            break;
        case lt_lex_double:
            c_double++;
            break;
        case lt_lex_float:
            c_float++;
            break;
        case lt_lex_short:
            c_short++;
            break;
        case lt_lex_int:
            c_int++;
            break;
        case lt_lex_long:
            c_long++;
            break;
        case lt_lex_signed:
            c_signed++;
            break;
        case lt_lex_unsigned:
            c_unsigned++;
            break;
        }

    if (c_signed != 0 && c_unsigned != 0)
    {
        return (lexem_type_t)-2001;
    }
    if (c_int > 1)
    {
        return (lexem_type_t)-2008;
    }
    if (c_double > 0)
    {
        check_error = c_signed + c_unsigned + c_char + c_short + c_int + c_float;
        if (check_error || c_double > 1 || c_long > 1)
            return (lexem_type_t)-2002;
        return lt_type_double;
    }
    else if (c_float > 0)
    {
        check_error = c_signed + c_unsigned + c_char + c_short + c_int + c_long;
        if (check_error || c_float > 1)
            return (lexem_type_t)-2003;
        return lt_type_float;
    }
    else if (c_char > 0)
    {
        check_error = c_short + c_int + c_long;
        if (check_error || c_char > 1)
            return (lexem_type_t)-2004;
        return c_unsigned > 0 ? lt_type_uchar : lt_type_char;
    }
    else if (c_short > 0)
    {
        if (c_long || c_short > 1)
            return (lexem_type_t)-2005;
        return c_unsigned > 0 ? lt_type_ushort : lt_type_short;
    }
    else if (c_long > 0)
    {
        if (c_long == 1)
            return c_unsigned > 0 ? lt_type_ulong : lt_type_long;
        if (c_long == 2)
            return c_unsigned > 0 ? lt_type_ullong : lt_type_llong;
        return (lexem_type_t)-2006;
    }
    else
        if (c_int > 1)
            return (lexem_type_t)-2007;
    return c_unsigned > 0 ? lt_type_uint : lt_type_int;
}

void append_lexem(lexem_type_t type)
{
    static int				tcount = 0;
    static lexem_type_t		prev_lexem = lt_empty;
    lexem_item_t		*	l;

    if (count > 10000)
        printf("Debug memory\n");
    if (type == 0 && current_lexem_size == 0)
        throw "something wrong"; // return;
    {
        static lexem_item_t		lexem_buff;
        static char				lexem_name[512];
        l = &lexem_buff;
        l->type = type;
        if (current_lexem_size) {
            l->lexem = (char*)lexem_name;
            strcpy(l->lexem, current_lexem);
            tcount++;
        }
        else
            l->lexem = 0;
        l->next = 0;
    }
    current_lexem_size = 0;
    memset(current_lexem, 0, sizeof(current_lexem));

    if (mode == om_debug) 
    {
        char * s = l->lexem;
        if (type == lt_word && !l->lexem)
            s = (char*) "_STATE_ERROR_";
        printf("+++ line %d lexem type '%d' \tvalue '%s'\n", st_line_number, type,
            (type == lt_word || type == lt_string || type == lt_integer ||
                type == lt_floatnumber) ? s :
                (type == lt_openbraket) ? "(" :
            (type == lt_comma) ? "," :
            (type == lt_closebraket) ? ")" :
            (type == lt_openblock) ? "{" :
            (type == lt_closeblock) ? "}" :
            (type == lt_colon) ? ":" :
            (type == lt_semicolon) ? ";" :
            (type == lt_openindex) ? "[" :
            (type == lt_closeindex) ? "]" :
            (type == lt_logical_and) ? "&&" :
            (type == lt_logical_or) ? "||" :
            (type == lt_and) ? "&" :
            (type == lt_or) ? "|" :
            (type == lt_shift_right) ? ">>" :
            (type == lt_more_eq) ? ">=" :
            (type == lt_more) ? ">" :
            (type == lt_shift_left) ? "<<" :
            (type == lt_less_eq) ? "<=" :
            (type == lt_less) ? "<" :
            (type == lt_set) ? "=" :
            (type == lt_add) ? "+" :
            (type == lt_inc) ? "++" :
            (type == lt_sub) ? "-" :
            (type == lt_dec) ? "--" :
            (type == lt_mul) ? "*" :
            (type == lt_not_eq) ? "!=" :
            (type == lt_logical_not) ? "!" :
            (type == lt_quest) ? "?" :
            (type == lt_point_to) ? "->" :
            (type == lt_equally) ? "==" :
            (type == lt_dot) ? "." :
            (type == lt_div) ? "/" :
            (type == lt_rest) ? "%" :
            (type == lt_character) ? s :
            "_NOT_PARSED_");
    }
    //else if( mode == om_test ) 
    //{
    //	char tmp_buff[256];
    //	lexem_to_string(l, tmp_buff, sizeof(tmp_buff));
    //	fprintf(of, tmp_buff );
    //} 
    else if (mode == om_lexem)
    {
        static int current_line = 0;
        if (current_line != st_line_number)
        {
            current_line = st_line_number;
            fprintf(of, "@%d\n", current_line);
        }
        char * s = l->lexem;

        if (lex_counter != 0)
        {
            static bool postfix_const = false;
            lexem_type_t new_type = lt_word;

            if (type == lt_word)
            {
                new_type = translate_lexem(s);
            }

            switch (new_type)
            {
            case lt_lex_bool:
            case lt_lex_char:
            case lt_lex_double:
            case lt_lex_float:
            case lt_lex_short:
            case lt_lex_int:
            case lt_lex_long:
            case lt_lex_signed:
            case lt_lex_unsigned:
                lex_queue[lex_counter++] = new_type;
                break;
            case lt_const:
                postfix_const = true;
                break;
            default:
                new_type = translate_builtin_types();
                if (new_type < 0)
                {
                    int len = 0;
                    lexem_item_t	lex;
                    char tmp_buff[80];
                    tmp_buff[0] = 0;
                    lex.lexem = (char*) "";
                    for (int i = 0; i < lex_counter; i++)
                    {
                        len = strlen(tmp_buff);
                        lex.type = lex_queue[i];
                        lexem_to_string(&lex, &tmp_buff[len], 79 - len);
                    }
                    fprintf(stderr, "Error in line %d - type combination is not parsed: %s\n", st_line_number, tmp_buff);
                    lex_counter = 0;
                    throw "Type combination is not parsed";
                }
                fprintf(of, "%d\n", new_type);
                if (postfix_const)
                {
                    fprintf(of, "%d\n", lt_const);
                    postfix_const = false;
                }

                if (type == lt_word)
                    fprintf(of, "%d %s\n", type, s);
                else
                    fprintf(of, "%d\n", type);

                lex_counter = 0;
                break;
            }
        }
        else if (
            prev_lexem == lt_struct ||
            prev_lexem == lt_class ||
            prev_lexem == lt_enum ||
            prev_lexem == lt_union)
        {
            if (type == lt_word)
                fprintf(of, "%d %s\n", prev_lexem, s);
            else if (type == lt_openblock)
                fprintf(of, "%d %s\n%d\n", prev_lexem, autoname ? generate_typename(prev_lexem) : "", lt_openblock);
            else
            {
                fprintf(stderr, "Must be name of type or open brace\n");
            }

        }
        else
        {
            if (type == lt_word)
            {
                if (!s)
                    throw "Empty lexem detected";
                type = translate_lexem(s);
                if (type == lt_empty)
                {
                    fprintf(of, "%d %s\n", lt_word, s);
                    return;
                }
            }
            switch (type)
            {
            case lt_word:
                throw "Lexem cannot be here";
                break;
            case lt_integer:
            case lt_floatnumber:
            case lt_character:
                if (!s) throw "Empty lexem detected";
                fprintf(of, "%d %s\n", type, s);
                break;
            case lt_string:
                fprintf(of, "%d %s\n", type, s ? s : "");
                break;
                /* Added 20170823 */
            case lt_struct:
            case lt_class:
            case lt_enum:
            case lt_union:
                break;
                /* Разбор встроенных типов */
            case lt_lex_bool:
            case lt_lex_char:
            case lt_lex_double:
            case lt_lex_float:
            case lt_lex_short:
            case lt_lex_int:
            case lt_lex_long:
            case lt_lex_signed:
            case lt_lex_unsigned:
                lex_queue[lex_counter++] = type;
                break;
                /* End of 20170823 */

            default:
                fprintf(of, "%d\n", type);
                break;
            }
        }
        prev_lexem = type;
    }
    else
        throw "Unknown parsing mode";
}

