#if ! defined _PRIMULA_LEXEM_H_
#define	_PRIMULA_LEXEM_H_

typedef enum {
	lt_empty,			// An empty lexeme - it cannot be in source file, ignore it
	lt_word,
	lt_integer,
	lt_unused,			// eliminate hexdecimal lexem, consider as integer now
	lt_floatnumber,
	lt_string,
	lt_openbraket,
	lt_comma,
	lt_closebraket,
	lt_openblock,
	lt_closeblock,
	lt_colon,
	lt_semicolon,
	lt_openindex,
	lt_closeindex,
	lt_logical_and,
	lt_logical_or,
	lt_and,
	lt_and_and_set,
	lt_xor,
	lt_xor_and_set,
	lt_or,
	lt_or_and_set,
	lt_not,
	lt_not_and_set,
	lt_shift_right,
	lt_more,
	lt_more_eq,
	lt_shift_left,
	lt_less_eq,
	lt_less,
	lt_set,
	lt_add,
	lt_add_and_set,
	lt_inc,
	lt_sub,
	lt_sub_and_set,
	lt_dec,
	lt_mul,
	lt_mul_and_set,
	lt_div,
	lt_div_and_set,
	lt_rest,			// we call '%' (remainder of the division) as rest
	lt_rest_and_set,	// This is %= opertaion
	lt_not_eq,
	lt_logical_not,
	lt_quest,
	lt_point_to,
	lt_equally,
	lt_character,
	lt_dot,
	lt_three_dots,
	lt_addrres,			//  &
	lt_indirect,		//  *
	lt_inc_postfix,
	lt_dec_postfix,
	lt_tilde,
	lt_namescope,		//  ::
	lt_direct_pointer_to_member,
	lt_indirect_pointer_to_member,

// Lexical analyzer detect and generates following types
	lt_type_bool = 600,
	lt_type_char,
	lt_type_uchar,
	lt_type_short,
	lt_type_ushort,
	lt_type_int,
	lt_type_uint,
	lt_type_long,
	lt_type_ulong,
	lt_type_llong,
	lt_type_ullong,
	lt_type_double,
	lt_type_float,
	lt_type_ldouble,
	lt_type_int64,

// List of reserved words of Ñ and Ñ++
	lt_asm = 1000,
	lt_auto,
	lt_break,
	lt_case,
	lt_catch,
	lt_class,
	lt_const,
	lt_const_cast,
	lt_continue,
	lt_default,				// 1010
	lt_delete,
	lt_do,
	lt_dynamic_cast,
	lt_else,
	lt_enum,
	lt_explicit,
	lt_export,
	lt_extern,
	lt_false,				// 1020
	lt_for,
	lt_friend,
	lt_goto,
	lt_if,
	lt_inline,
	lt_mutable,
	lt_namespace,
	lt_new,
	lt_operator,			// 1030
	lt_private,
	lt_protected,
	lt_public,
	lt_register,
	lt_reinterpret_cast,
	lt_return,
	lt_sizeof,
	lt_static,
	lt_static_cast,			// 1040
	lt_struct,
	lt_switch,
	lt_template,
	lt_this,
	lt_throw,
	lt_true,
	lt_try,
	lt_typedef,
	lt_typeid,
	lt_typename,			// 1050
	lt_union,
	lt_using,
	lt_virtual,
	lt_void,
	lt_volatile,
	lt_wchar_t,
	lt_while,

// Lexical parser manage these types
	lt_lex_bool = 2000,
	lt_lex_char,
	lt_lex_double,
	lt_lex_float,
	lt_lex_short,
	lt_lex_int,
	lt_lex_long,
	lt_lex_signed,
	lt_lex_unsigned,

// Following lexemes generated on build syntax tree
	lt_typecasting = 3000,
	lt_call,
	lt_call_method,
	lt_pointer,
	lt_expression,
	lt_data,
	lt_variable,
	lt_unary_plus,
	lt_unary_minus,
	lt_array_of_types,
	lt_function_address,
	lt_argument,
    lt_operator_postinc,
    lt_operator_postdec,

// Something wrong
	lt_error = -1
} lexem_type_t;

typedef enum { 
	vs_private, 
	vs_protected, 
	vs_public 
} visibility_t;

typedef struct {
	lexem_type_t		lexem_type;
	char const		*	lexem_value;
	int					line_number;
	char			*	file_name;
	lexem_type_t		previous_lexem;
} lexem_event_t;

#endif