#if ! defined DEFDEF
#define DEFDEF

#include "lexem.h"

	typedef enum {
		om_debug,
		om_test,
		om_lexem,
		om_no_such_key
	} output_mode_t;

	typedef enum {
		gs_expression,
		gs_lexem,
		gs_inside_lexem,
		gs_delimiter,
		gs_numeric,
		gs_string,
		gs_parse_slash,
		gs_parse_rest,
		gs_linear_comment,
		gs_caret_feed,
		gs_new_line,
		gs_do_preprocessor,
		gs_do_screen_preprocessor,
		gs_skip_preprocessor,
		gs_is_preprocessor_comment,
		gs_prep_line_comment,
		gs_prep_block_comment,
		gs_prep_check_comment,
		gs_and,
		gs_xor,
		gs_or,
		gs_more,
		gs_less,
		gs_plus,
		gs_check_ptr_to_member,
		gs_minus,
		gs_dot,
		gs_two_dots,
		gs_exclamation,
		gs_screening,
		gs_equally,
		gs_mul,
		gs_colon,
		gs_apost_begin,
		gs_apost_end,
		gs_inside_comment,
		gs_checking_comment,
		gs_enter_def_expression,
		gs_parse_def_expression,
		// 20180422
		gs_try_concatenate_strings
	} global_states_t;

	typedef struct {
		global_states_t		global_state;
		FILE			*	f;
		FILE			*	of;
		output_mode_t		mode;
		int					line_number;
		char				filename[512];
	} flow_control_t;

	typedef struct {
		char		loong_buffer[232];
		char	*	args;
		char	*	payload;
	} def_pair_t;

	typedef struct lexem_item_t {
		lexem_item_t	*	next;
		char			*	lexem;
		lexem_type_t		type;
	} lexem_item_t;


extern int parse_definition( char * define_payload );
extern int remove_definition( char * define_payload );
extern def_pair_t * check_for_preprocessor_define( char * name );
extern int parse_preprocessor_if( char * payload );


#endif