#include "../include/lexem.h"

typedef struct {
	lexem_type_t	type;
	int				priority;
} lexem_desc_t;

lexem_desc_t	fast_option[] =
{
	{ lt_empty, -1 },
	{ lt_word, 1000 },
	{ lt_integer, 1000 },
	{ lt_error, -1},
	{ lt_floatnumber, 1000},
	{ lt_string, 1000},
	{ lt_openbraket, 5000},
	{ lt_comma, 170},
	{ lt_closebraket, },
	{ lt_openblock, },
	{ lt_closeblock, },
	{ lt_colon, 150 },
	{ lt_semicolon, },
	{ lt_openindex, 20},
	{ lt_closeindex, },
	{ lt_logical_and, 130},
	{ lt_logical_or, 140},
	{ lt_and, 100},
	{ lt_and_and_set, 150},
	{ lt_xor, 110},
	{ lt_xor_and_set, 150},
	{ lt_or, 120},
	{ lt_or_and_set, 150},
	{ lt_not, 30},
	{ lt_not_and_set, 150},
	{ lt_shift_right, 70},
	{ lt_more, 80},
	{ lt_more_eq, 80},
	{ lt_shift_left, 70},
	{ lt_less_eq, 80},
	{ lt_less, 80},
	{ lt_set, 150},
	{ lt_add, 60},
	{ lt_add_and_set, 150},
	{ lt_inc, 30},
	{ lt_sub, 60},
	{ lt_sub_and_set, 150},
	{ lt_dec, 30},
	{ lt_mul, 50},
	{ lt_mul_and_set, 150},
	{ lt_div, 50},
	{ lt_div_and_set, 150},
	{ lt_rest, 50},				// remainder of division '%'
	{ lt_rest_and_set, 150},	// %=
	{ lt_not_eq, 90},
	{ lt_logical_not, 30},
	{ lt_quest, 150},
	{ lt_point_to, 20},
	{ lt_equally, 90},
	{ lt_character, 1000},
	{ lt_dot, 20},
	{ lt_three_dots, },
	{ lt_addrres, 30},			// &
	{ lt_indirect, 30},			// *
	{ lt_inc_postfix, 20},
	{ lt_dec_postfix, 20},
	{ lt_tilde, 30},
	{ lt_namescope, 10},		// ::
	{lt_direct_pointer_to_member, /* standatd is 40 */ 20},
	{lt_indirect_pointer_to_member, /* standard is 40 */ 20}
};

extern int GetLexemPriority(lexem_type_t  lex)
{
	int prio = -1;
	if (lex < (sizeof(fast_option) / sizeof(fast_option[0])))
		prio = fast_option[lex].priority;
	else if (lex >= lt_type_bool && lex <= lt_type_int64)
	{
		prio = 2000;
	}
	else
		switch (lex)
		{
		case lt_typecasting:
		case lt_new:
		case lt_unary_plus:
		case lt_unary_minus:
			prio = 30;
			break;
		case lt_direct_pointer_to_member:
		case lt_indirect_pointer_to_member:
			prio = 40;
			break;
        case lt_operator_postinc:
        case lt_operator_postdec:
            prio = 20;
            break;
		case lt_void:
			prio = 2000;
			break;
		case lt_true:
		case lt_false:
		case lt_this:
		case lt_variable:
			prio = 1000; // 'this' word as operand
			break;
		case lt_sizeof:
			prio = 30;
			break;
		case lt_call:
		case lt_call_method:
        case lt_argument:
			prio = 2000;
			break;
		case lt_function_address:
			prio = 2000;
			break;
		default:
//			__debugbreak();
			throw "Lexem priority not defined";
		}
	return prio;
}
